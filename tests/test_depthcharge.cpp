// test_depthcharge.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/depthcharge.xml, played frame by frame by a real
// xge::Game with no window. Depth Charge is written with verbs the language
// already had: <fire /> for the charge, <die /> when it hits a submarine or the
// sea floor, <dec /> on the floor rule so that only a miss costs anything,
// <wrap /> for the lanes of submarines, and a remaining condition to win.
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
	constexpr float kWindowWidth = 800.0f;
	constexpr float kWindowHeight = 600.0f;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	struct Table
	{
		Game game{ "games/depthcharge.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& ship() { return game.getObject("ship"); }
		Object& charge() { return game.getObject("charge"); }
		float charges() { return ship().variable["charges"]; }
		float sunk() { return ship().variable["sunk"]; }

		// A charge in the water: shown and taking part.
		bool falling() { return charge().isVisible && charge().collisionData.enabled; }

		void press(const char* action)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "ship", action } }, true);
		}

		void key(const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "ship", direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		std::vector<Object*> subs()
		{
			std::vector<Object*> found;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "subs") { found.push_back(&object); }
			}
			return found;
		}

		// Holds every submarine still and puts the ones in `except` back, so a
		// test about a charge is not about a lane drifting into it.
		void freezeSubs()
		{
			for (Object* sub : subs()) { sub->velocity = { 0.0f, 0.0f }; }
		}

		// Sinks all but the named submarine, as if they had been hit earlier.
		void sinkAllBut(const std::string& survivor)
		{
			for (Object* sub : subs())
			{
				if (sub->name == survivor) { continue; }
				sub->isVisible = false;
				sub->collisionData.enabled = false;
			}
		}

		// Drops a charge and puts it right on top of a submarine.
		void dropOnto(const std::string& name)
		{
			press("drop");
			Object& sub = game.getObject(name);
			charge().position = { sub.position.x + sub.size.x / 2.0f, sub.position.y - 2.0f };
		}

		// Drops a charge and puts it a frame away from the sea floor, in a gap
		// between lanes, so that it misses.
		void dropAndMiss()
		{
			press("drop");
			charge().position = { 5.0f, kWindowHeight - 8.0f };
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

TEST_CASE("depthcharge.xml loads a ship, nine submarines in three lanes and eight charges", "[depthcharge]")
{
	Table table;

	CHECK(table.subs().size() == 9);
	CHECK(table.charges() == 8.0f);
	CHECK(table.sunk() == 0.0f);
	CHECK(!table.falling());
	CHECK(table.ship().position.x + table.ship().size.x / 2.0f == kWindowWidth / 2.0f);

	// Three lanes, three submarines each, each lane its own speed and row.
	float speeds[3] = { 2.0f, -3.0f, 4.0f };
	float rows[3] = { 190.0f, 300.0f, 410.0f };
	for (int lane = 1; lane <= 3; ++lane)
	{
		for (int member = 1; member <= 3; ++member)
		{
			const Object& sub = table.game.getObject("subs" + std::to_string(lane) + "." + std::to_string(member));
			INFO("subs" << lane << "." << member);
			CHECK(sub.velocity.x == speeds[lane - 1]);
			CHECK(sub.position.y == rows[lane - 1]);
		}
	}
}

TEST_CASE("held keys slide the ship and it stops at the sides", "[depthcharge]")
{
	Table table;
	const float startX = table.ship().position.x;

	table.key("right", true);
	table.frames(10);
	CHECK(table.ship().position.x == startX + 60.0f);

	table.key("right", false);
	table.frames(10);
	CHECK(table.ship().position.x == startX + 60.0f);

	table.key("left", true);
	table.frames(200);
	CHECK(table.ship().position.x == 0.0f);

	table.key("left", false);
	table.key("right", true);
	table.frames(200);
	CHECK(table.ship().position.x + table.ship().size.x == kWindowWidth);
}

TEST_CASE("dropping a charge launches it from the ship, falling", "[depthcharge]")
{
	Table table;
	table.key("right", true);
	table.frames(20);
	table.key("right", false);

	table.press("drop");

	CHECK(table.falling());
	CHECK(table.charge().position.x == table.ship().position.x + table.ship().size.x / 2.0f);
	CHECK(table.charge().position.y == table.ship().position.y);
	CHECK(table.charge().velocity.x == 0.0f);
	CHECK(table.charge().velocity.y == 5.0f);

	// Keeps falling, straight, while the ship moves on.
	const float x = table.charge().position.x;
	const float y = table.charge().position.y;
	table.key("left", true);
	table.frames(4);
	CHECK(table.charge().position.x == x);
	CHECK(table.charge().position.y == y + 20.0f);
}

TEST_CASE("only one charge can be falling at a time", "[depthcharge]")
{
	Table table;
	table.press("drop");
	table.frames(10);
	const float y = table.charge().position.y;

	table.key("right", true);
	table.frames(5);
	table.key("right", false);
	table.press("drop"); // ignored: the first is still in the water

	CHECK(table.charge().position.y == y + 25.0f);
	CHECK(table.charge().position.x != table.ship().position.x + table.ship().size.x / 2.0f);
}

TEST_CASE("a charge that hits a submarine sinks it and costs nothing", "[depthcharge]")
{
	Table table;
	table.freezeSubs();

	table.dropOnto("subs2.2");
	table.frames(1);

	CHECK(table.sunk() == 1.0f);
	CHECK(table.charges() == 8.0f);
	CHECK(!table.game.getObject("subs2.2").isVisible);
	CHECK(!table.falling()); // the charge is used up: it can be dropped again
	CHECK(table.game.getObject("sunkvalue").spriteParams.at(1) == "1");
	CHECK(table.game.getCurrentState().name == "playing");

	// The other submarines are untouched.
	int alive = 0;
	for (Object* sub : table.subs()) { if (sub->isVisible) { ++alive; } }
	CHECK(alive == 8);

	table.press("drop");
	CHECK(table.falling());
}

TEST_CASE("a sunk submarine no longer stops anything", "[depthcharge]")
{
	Table table;
	table.freezeSubs();
	table.dropOnto("subs1.1");
	table.frames(1);
	REQUIRE(!table.game.getObject("subs1.1").isVisible);

	table.dropOnto("subs1.1");
	table.frames(1);

	CHECK(table.sunk() == 1.0f); // nothing to hit the second time
	CHECK(table.falling());
}

TEST_CASE("a charge that reaches the sea floor is a miss", "[depthcharge]")
{
	Table table;
	table.freezeSubs();

	table.dropAndMiss();
	table.frames(3);

	CHECK(table.charges() == 7.0f);
	CHECK(table.sunk() == 0.0f);
	CHECK(!table.falling());
	CHECK(table.game.getObject("chargesvalue").spriteParams.at(1) == "7");
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("a charge that falls through an empty lane keeps going", "[depthcharge]")
{
	Table table;
	table.freezeSubs();
	table.press("drop");
	table.charge().position = { 190.0f, 150.0f }; // above the first lane, with a gap in every lane at that column
	table.frames(60);

	CHECK(table.charge().position.y > 400.0f);
	CHECK(table.falling());
	CHECK(table.charges() == 8.0f);
}

TEST_CASE("the eighth miss ends the game, and a hit in between changes nothing", "[depthcharge]")
{
	Table table;
	table.freezeSubs();

	for (int i = 0; i < 6; ++i)
	{
		table.dropAndMiss();
		table.frames(3);
	}
	table.dropOnto("subs3.1");
	table.frames(1);
	CHECK(table.charges() == 2.0f);
	CHECK(table.sunk() == 1.0f);
	CHECK(table.game.getCurrentState().name == "playing");

	table.dropAndMiss();
	table.frames(3);
	CHECK(table.charges() == 1.0f);
	CHECK(table.game.getCurrentState().name == "playing");

	table.dropAndMiss();
	table.frames(3);
	CHECK(table.charges() == 0.0f);
	CHECK(table.game.getCurrentState().name == "gameover");
}

TEST_CASE("sinking the last submarine wins, and eight are not enough", "[depthcharge]")
{
	{
		Table table;
		table.freezeSubs();
		table.sinkAllBut("subs3.3");
		table.dropOnto("subs3.3");
		table.frames(1);
		CHECK(table.game.getObject("subs3.3").isVisible == false);
		CHECK(table.game.getCurrentState().name == "youwin");
	}
	{
		Table table;
		table.freezeSubs();
		table.sinkAllBut("subs3.3");
		table.game.getObject("subs1.1").isVisible = true;
		table.game.getObject("subs1.1").collisionData.enabled = true;
		table.dropOnto("subs3.3");
		table.frames(1);
		CHECK(table.game.getCurrentState().name == "playing");
	}
}

TEST_CASE("the lanes loop: a submarine that has left one side comes in from the other", "[depthcharge][wrap]")
{
	Table table;

	Object& rightward = table.game.getObject("subs1.1"); // lane 1 heads right
	rightward.position.x = kWindowWidth + 1.0f;
	Object& leftward = table.game.getObject("subs2.1");  // lane 2 heads left
	leftward.position.x = -leftward.size.x - 1.0f;
	table.frames(1);

	CHECK(rightward.position.x < 0.0f);
	CHECK(leftward.position.x > kWindowWidth - leftward.size.x - 10.0f);
}

TEST_CASE("space starts a new game from either end screen with everything back", "[depthcharge]")
{
	Table table;
	table.freezeSubs();
	table.dropOnto("subs1.2");
	table.frames(1);
	table.ship().variable["charges"] = 1.0f;
	table.dropAndMiss();
	table.frames(3);
	REQUIRE(table.game.getCurrentState().name == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.charges() == 8.0f);
	CHECK(table.sunk() == 0.0f);
	CHECK(table.game.getObject("subs1.2").isVisible);
	CHECK(!table.falling());
	CHECK(table.game.getObject("chargesvalue").spriteParams.at(1) == "8");
}

TEST_CASE("the keys play the game through the engine", "[depthcharge][engine_input]")
{
	Game game{ "games/depthcharge.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& ship = game.getObject("ship");
	Object& charge = game.getObject("charge");

	CHECK(game.getCurrentState().name == "mainmenu");
	engine.handleKeyPressed(KeyCode::Space); // space starts the game
	REQUIRE(game.getCurrentState().name == "playing");
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(!charge.isVisible);

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(charge.isVisible);
	CHECK(charge.collisionData.enabled);

	const float startX = ship.position.x;
	engine.handleKeyPressed(KeyCode::D);
	game.updateObjects();
	CHECK(ship.position.x == startX + 6.0f);
	engine.handleKeyReleased(KeyCode::D);
	engine.handleKeyPressed(KeyCode::Left);
	game.updateObjects();
	game.updateObjects();
	CHECK(ship.position.x == startX - 6.0f);
	engine.handleKeyReleased(KeyCode::Left);

	engine.handleKeyPressed(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::Escape);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("a crude player who fires when a submarine is overhead sinks some", "[depthcharge][play]")
{
	// Fire whenever a charge dropped now, falling straight down, would meet a
	// submarine: the ship sits still and drops on whatever is about to pass under
	// it. In a minute of that, at least one submarine must go down.
	Table table;
	for (int frame = 0; frame < 60 * 60; ++frame)
	{
		if (!table.falling())
		{
			const float chargeX = table.ship().position.x + table.ship().size.x / 2.0f;
			for (Object* sub : table.subs())
			{
				if (!sub->isVisible) { continue; }
				const float travel = (sub->position.y - table.ship().position.y) / 5.0f; // frames to fall to its row
				const float subX = sub->position.x + sub->velocity.x * travel + sub->size.x / 2.0f;
				if (subX > chargeX - 20.0f && subX < chargeX + 20.0f)
				{
					table.press("drop");
					break;
				}
			}
		}
		table.frames(1);
		if (table.game.getCurrentState().name != "playing") { break; }
	}

	CHECK(table.sunk() > 0.0f);
}
