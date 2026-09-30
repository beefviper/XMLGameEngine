// test_freeway.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/freeway.xml, played frame by frame by a real
// xge::Game with no window. Freeway is written with verbs the language already
// had (hop, wrap, reset, inc, an atleast condition), so these tests are about
// the game those make up: two chickens crossing ten lanes of traffic, the first
// to five crossings wins.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "engine.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	constexpr float kCell = 50.0f;
	constexpr float kInset = 8.0f;
	constexpr float kWindowWidth = 800.0f;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	// The top of a chicken standing in a row.
	float rowY(int row) { return static_cast<float>(row) * kCell + kInset; }

	struct Table
	{
		Game game{ "games/freeway.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& chicken(int player) { return game.getObject("chicken" + std::to_string(player)); }
		float score(int player) { return chicken(player).variable["score"]; }

		void hop(int player, const char* direction)
		{
			const std::string name = "chicken" + std::to_string(player);
			executor.executeInput(Command{ CmdTriggerAction{ name, direction } }, true);
			executor.executeInput(Command{ CmdTriggerAction{ name, direction } }, false);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Every car and truck parked far off to the left, so a test about the
		// chickens is not about the traffic.
		void clearTraffic()
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "traffic")
				{
					object.position.x = -5000.0f;
					object.velocity = { 0.0f, 0.0f };
				}
			}
		}

		bool atStart(int player)
		{
			return chicken(player).position.x == chicken(player).positionOriginal.x
				&& chicken(player).position.y == chicken(player).positionOriginal.y;
		}
	};

	// Just enough of a Window for Engine's constructor.
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};
}

TEST_CASE("freeway.xml loads two chickens at the bottom and ten lanes of traffic", "[freeway]")
{
	Table table;

	CHECK(table.score(1) == 0.0f);
	CHECK(table.score(2) == 0.0f);
	CHECK(table.chicken(1).position.y == rowY(11));
	CHECK(table.chicken(2).position.y == rowY(11));
	// Each in its own half of the road.
	CHECK(table.chicken(1).position.x + table.chicken(1).size.x / 2.0f == kWindowWidth / 4.0f);
	CHECK(table.chicken(2).position.x + table.chicken(2).size.x / 2.0f == kWindowWidth * 3.0f / 4.0f);

	int traffic = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "traffic") { ++traffic; }
	}
	CHECK(traffic == 4 * 2 + 6 * 3); // four lanes of two trucks, six of three cars
}

TEST_CASE("each lane sits in its own row, and neighbouring lanes go opposite ways", "[freeway]")
{
	Table table;

	float previousSpeed = 0.0f;
	for (int row = 1; row <= 10; ++row)
	{
		const std::string lane = "lane" + std::to_string(row);
		const Object& first = table.game.getObject(lane + ".1");
		INFO(lane);
		CHECK(first.position.y == rowY(row));
		CHECK(first.velocity.y == 0.0f);
		CHECK(first.velocity.x != 0.0f);
		if (previousSpeed != 0.0f)
		{
			CHECK((first.velocity.x > 0.0f) != (previousSpeed > 0.0f));
		}
		previousSpeed = first.velocity.x;

		// Everything in a lane shares its row and speed.
		for (int member = 2; member <= 3; ++member)
		{
			const std::string name = lane + "." + std::to_string(member);
			bool found = false;
			for (const auto& object : table.game.getCurrentObjects())
			{
				if (object.name == name) { found = true; }
			}
			if (!found) { continue; }
			const Object& other = table.game.getObject(name);
			CHECK(other.position.y == first.position.y);
			CHECK(other.velocity.x == first.velocity.x);
		}
	}
}

TEST_CASE("traffic loops: a car that has left one side comes in from the other", "[freeway][wrap]")
{
	Table table;

	// Lane 2 heads right, lane 1 heads left.
	Object& rightward = table.game.getObject("lane2.1");
	rightward.position.x = kWindowWidth + 1.0f; // completely off the right side
	Object& leftward = table.game.getObject("lane1.1");
	leftward.position.x = -leftward.size.x - 1.0f; // completely off the left side
	table.frames(1);

	CHECK(rightward.position.x < 0.0f);
	CHECK(leftward.position.x > kWindowWidth - leftward.size.x - 10.0f);
}

TEST_CASE("a hop is one lane, once, and the bottom edge refuses it", "[freeway][hop]")
{
	Table table;
	table.clearTraffic();

	table.hop(1, "up");
	table.frames(1);
	CHECK(table.chicken(1).position.y == rowY(10));
	table.frames(10);
	CHECK(table.chicken(1).position.y == rowY(10)); // nothing keeps it going

	table.hop(1, "down");
	table.frames(1);
	CHECK(table.chicken(1).position.y == rowY(11));

	table.hop(1, "down"); // nothing below the start
	table.frames(1);
	CHECK(table.chicken(1).position.y == rowY(11));
}

TEST_CASE("the chickens are driven separately", "[freeway][hop]")
{
	Table table;
	table.clearTraffic();

	table.hop(2, "up");
	table.frames(1);
	CHECK(table.chicken(2).position.y == rowY(10));
	CHECK(table.chicken(1).position.y == rowY(11));

	table.hop(1, "up");
	table.hop(1, "up");
	table.frames(1); // two hops in one frame keep the later one
	CHECK(table.chicken(1).position.y == rowY(10));
	CHECK(table.chicken(2).position.y == rowY(10));
}

TEST_CASE("crossing scores a point and sends the chicken back to the start", "[freeway]")
{
	Table table;
	table.clearTraffic();

	table.chicken(1).position.y = rowY(1);
	table.hop(1, "up");
	table.frames(1);

	CHECK(table.score(1) == 1.0f);
	CHECK(table.score(2) == 0.0f);
	CHECK(table.atStart(1));
	CHECK(table.game.getObject("score1").spriteParams.at(1) == "1");
	CHECK(table.game.getObject("score2").spriteParams.at(1) == "0");
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("being hit puts the chicken back at the start and costs nothing else", "[freeway]")
{
	Table table;
	table.clearTraffic();
	table.chicken(2).variable["score"] = 3.0f;

	// Halfway across, with a car of lane 5 on top of it.
	table.chicken(2).position.y = rowY(5);
	// (A pair in which nothing moves is not looked at, so the car keeps its lane's speed.)
	Object& car = table.game.getObject("lane5.1");
	car.position.x = table.chicken(2).position.x;
	car.velocity.x = -3.0f;
	table.frames(1);

	CHECK(table.atStart(2));
	CHECK(table.score(2) == 3.0f);
	CHECK(table.score(1) == 0.0f);
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("a hop onto a lane where a car is waiting is a hit", "[freeway][hop]")
{
	Table table;
	table.clearTraffic();

	Object& car = table.game.getObject("lane10.1");
	car.position.x = table.chicken(1).position.x - 5.0f; // over the chicken's column
	car.velocity = { 0.0f, 0.0f };
	table.hop(1, "up");
	table.frames(1);

	CHECK(table.atStart(1)); // it landed in the car's lane and was put straight back
}

TEST_CASE("the two chickens do not get in each other's way", "[freeway]")
{
	Table table;
	table.clearTraffic();

	table.chicken(2).position = table.chicken(1).position;
	table.frames(5);

	CHECK(table.chicken(1).position.y == rowY(11));
	CHECK(table.chicken(2).position.y == rowY(11));
	CHECK(table.score(1) == 0.0f);
	CHECK(table.score(2) == 0.0f);
}

TEST_CASE("five crossings win, and four are not enough", "[freeway]")
{
	{
		Table table;
		table.clearTraffic();
		table.chicken(1).variable["score"] = 3.0f;
		table.chicken(1).position.y = rowY(1);
		table.hop(1, "up");
		table.frames(1);
		CHECK(table.score(1) == 4.0f);
		CHECK(table.game.getCurrentState().name == "playing");
	}
	{
		Table table;
		table.clearTraffic();
		table.chicken(1).variable["score"] = 4.0f;
		table.chicken(1).position.y = rowY(1);
		table.hop(1, "up");
		table.frames(1);
		CHECK(table.score(1) == 5.0f);
		CHECK(table.game.getCurrentState().name == "p1wins");
	}
	{
		Table table;
		table.clearTraffic();
		table.chicken(2).variable["score"] = 4.0f;
		table.chicken(2).position.y = rowY(1);
		table.hop(2, "up");
		table.frames(1);
		CHECK(table.game.getCurrentState().name == "p2wins");
	}
}

TEST_CASE("space starts a new game from either win screen, with both scores back to nothing", "[freeway]")
{
	Table table;
	table.clearTraffic();
	table.chicken(2).variable["score"] = 4.0f;
	table.chicken(2).position.y = rowY(1);
	table.hop(2, "up");
	table.frames(1);
	table.chicken(1).variable["score"] = 2.0f;
	REQUIRE(table.game.getCurrentState().name == "p2wins");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.score(1) == 0.0f);
	CHECK(table.score(2) == 0.0f);
	CHECK(table.game.getObject("score1").spriteParams.at(1) == "0");
	CHECK(table.game.getObject("score2").spriteParams.at(1) == "0");
	CHECK(table.atStart(2));
}

TEST_CASE("the keys drive the right chicken through the whole game", "[freeway][engine_input]")
{
	Game game{ "games/freeway.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());

	engine.handleKeyPressed(KeyCode::Space); // mainmenu -> playing
	REQUIRE(game.getCurrentState().name == "playing");
	for (auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "traffic") { object.position.x = -5000.0f; object.velocity = { 0.0f, 0.0f }; }
	}

	Object& one = game.getObject("chicken1");
	Object& two = game.getObject("chicken2");

	engine.handleKeyPressed(KeyCode::W);
	engine.handleKeyReleased(KeyCode::W);
	game.updateObjects();
	CHECK(one.position.y == rowY(10));
	CHECK(two.position.y == rowY(11));

	engine.handleKeyPressed(KeyCode::Up);
	engine.handleKeyReleased(KeyCode::Up);
	game.updateObjects();
	CHECK(two.position.y == rowY(10));
	CHECK(one.position.y == rowY(10));

	engine.handleKeyPressed(KeyCode::S);
	engine.handleKeyReleased(KeyCode::S);
	engine.handleKeyPressed(KeyCode::Down);
	engine.handleKeyReleased(KeyCode::Down);
	game.updateObjects();
	CHECK(one.position.y == rowY(11));
	CHECK(two.position.y == rowY(11));

	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("a chicken that just keeps hopping up is hit by the traffic", "[freeway][play]")
{
	// Not a skilled player: hop up five times a second for a minute. Nothing
	// waits for a gap, so the traffic must put it back at the start at least
	// once, or the lanes are empty.
	Table table;
	const float startY = table.chicken(1).position.y;
	int hits = 0;
	int crossings = 0;
	for (int frame = 0; frame < 60 * 60; ++frame)
	{
		const float previousY = table.chicken(1).position.y;
		const float previousScore = table.score(1);
		if (frame % 12 == 0) { table.hop(1, "up"); }
		table.frames(1);
		if (table.score(1) > previousScore) { ++crossings; }
		else if (table.chicken(1).position.y == startY && previousY < startY) { ++hits; }
		if (table.game.getCurrentState().name != "playing") { break; }
	}

	CHECK(hits > 0);
	CHECK(hits + crossings > 0);
}
