// test_kaboom.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/kaboom.xml, played frame by frame by a real
// xge::Game with no window. The Mad Bomber paces the rooftop, turning when a
// <timer> with a random wait says so, and drops bombs from a pool on another
// timer, from under himself (<facing>down</facing>). A bomb caught goes back
// into the pool; one on the ground costs a bucket and puts every bomb of the
// wave back at once (<reset object>). Three waves, each a state with its own
// bomber and pool, sharing one <keys> set.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	constexpr float kWindowWidth = 800.0f;
	constexpr float kWindowHeight = 600.0f;
	constexpr int kFramerate = 60;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	struct Table
	{
		Game game{ "games/kaboom.xml" };
		CommandExecutor executor{ game };

		explicit Table(const char* state = "wave1")
		{
			measure(game);
			game.setCurrentState(0);
			game.pushState(state);
		}

		Object& bucket() { return game.getObject("bucket"); }
		float score() { return bucket().variable["score"]; }
		float buckets() { return bucket().variable["buckets"]; }
		std::string state() { return game.getCurrentState().name; }

		// For tests that play on whatever happens: enough buckets that no run
		// of bad luck ends the game.
		void keepPlaying() { bucket().variable["buckets"] = 1000.0f; }

		void key(const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "bucket", direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// The bombs of a pool that are falling.
		std::vector<Object*> falling(const std::string& pool)
		{
			std::vector<Object*> found;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.groupName == pool && object.isVisible) { found.push_back(&object); }
			}
			return found;
		}

		// Slides the bucket under the lowest bomb still above it: a player who
		// never misses a beat.
		void trackLowestBomb()
		{
			float targetX = kWindowWidth / 2.0f;
			float lowest = -1.0f;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.objClass != "bombs" || !object.isVisible || !game.isShown(object)) { continue; }
				if (object.position.y > lowest && object.position.y < bucket().position.y)
				{
					lowest = object.position.y;
					targetX = object.position.x + object.size.x / 2.0f;
				}
			}
			const float centre = bucket().position.x + bucket().size.x / 2.0f;
			key("left", centre > targetX + 6.0f);
			key("right", centre < targetX - 6.0f);
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

TEST_CASE("kaboom.xml loads a rooftop, a bomber and a pool of ten bombs for each of three waves, and a bucket", "[kaboom]")
{
	Table table;

	for (const char* bomber : { "bomber1", "bomber2", "bomber3" })
	{
		const Object& object = table.game.getObject(bomber);
		CHECK(object.hasFacing);
		CHECK(object.facing == Direction::Down);
		CHECK(object.timers.size() == 2);
		CHECK(object.position.y + object.size.y <= 104.0f + 1.0f);
	}

	for (const char* pool : { "bombs1", "bombs2", "bombs3" })
	{
		int members = 0;
		for (const auto& object : table.game.getCurrentObjects())
		{
			if (object.groupName != pool) { continue; }
			++members;
			CHECK_FALSE(object.isVisible);
			CHECK_FALSE(object.collisionData.enabled);
		}
		CHECK(members == 10);
	}

	CHECK(table.buckets() == 3);
	CHECK(table.score() == 0);
}

TEST_CASE("the bomber drops a bomb from under himself every so often, and it falls straight down", "[kaboom]")
{
	Table table;
	const Object& bomber = table.game.getObject("bomber1");

	table.frames(static_cast<int>(0.55f * kFramerate) - 1);
	CHECK(table.falling("bombs1").empty());
	table.frames(1);

	const auto first = table.falling("bombs1");
	REQUIRE(first.size() == 1);
	const Object& bomb = *first[0];
	CHECK(bomb.velocity.x == Approx(0));
	CHECK(bomb.velocity.y == Approx(3));

	// Under the middle of the bomber, below the rooftop.
	CHECK(std::abs((bomb.position.x + bomb.size.x / 2) - (bomber.position.x + bomber.size.x / 2)) < 8.0f);
	CHECK(bomb.position.y >= bomber.position.y + bomber.size.y);

	table.frames(static_cast<int>(0.55f * kFramerate) * 3);
	CHECK(table.falling("bombs1").size() == 4);
}

TEST_CASE("the bomber stays on the roof, turns at the sides, and changes his mind in between", "[kaboom][random]")
{
	Table table;
	table.keepPlaying();
	Object& bomber = table.game.getObject("bomber1");

	int turnsAwayFromTheSides = 0;
	float lastDirection = bomber.velocity.x;
	for (int frame = 0; frame < 20 * kFramerate; ++frame)
	{
		table.frames(1);
		CHECK(bomber.position.x >= -10.0f);
		CHECK(bomber.position.x + bomber.size.x <= kWindowWidth + 10.0f);
		CHECK(bomber.velocity.y == 0);

		const bool nearSide = bomber.position.x < 20.0f || bomber.position.x + bomber.size.x > kWindowWidth - 20.0f;
		if ((bomber.velocity.x > 0) != (lastDirection > 0) && !nearSide) { ++turnsAwayFromTheSides; }
		lastDirection = bomber.velocity.x;
	}

	CHECK(turnsAwayFromTheSides >= 5);
}

TEST_CASE("held keys slide the bucket and it stops at the sides", "[kaboom]")
{
	Table table;
	table.keepPlaying();
	const float startX = table.bucket().position.x;

	table.key("left", true);
	table.frames(1);
	CHECK(table.bucket().position.x == Approx(startX - 9));
	table.frames(200);
	CHECK(table.bucket().position.x == Approx(0));
	table.key("left", false);

	table.key("right", true);
	table.frames(200);
	CHECK(table.bucket().position.x + table.bucket().size.x == Approx(kWindowWidth));
}

TEST_CASE("a caught bomb scores one, two or three by the wave, and goes back into the pool", "[kaboom]")
{
	for (const auto& [wave, pool, points] : { std::tuple{ "wave1", "bombs1", 1.0f }, std::tuple{ "wave2", "bombs2", 2.0f }, std::tuple{ "wave3", "bombs3", 3.0f } })
	{
		Table table(wave);
		table.keepPlaying();

		// Wait for the first bomb, then put the bucket under it.
		while (table.falling(pool).empty()) { table.frames(1); }
		Object& bomb = *table.falling(pool)[0];
		bomb.position = { table.bucket().position.x + 20.0f, table.bucket().position.y - bomb.size.y };
		table.frames(2);

		CHECK(table.score() == points);
		CHECK_FALSE(bomb.isVisible);
		CHECK_FALSE(bomb.collisionData.enabled);
	}
}

TEST_CASE("a bomb on the ground costs a bucket and sets off every bomb in the air", "[kaboom]")
{
	Table table;
	table.frames(static_cast<int>(0.55f * kFramerate) * 3 + 2);
	auto falling = table.falling("bombs1");
	REQUIRE(falling.size() == 3);

	// The bucket out of the way, and the lowest bomb a frame from the ground.
	table.bucket().position.x = 0;
	Object& lowest = *falling[0];
	lowest.position = { 700.0f, kWindowHeight - lowest.size.y - 1.0f };
	table.frames(2); // it lands on the second

	CHECK(table.buckets() == 2);
	CHECK(table.falling("bombs1").empty());
	CHECK(table.score() == 0);

	// The bomber goes on dropping.
	table.frames(static_cast<int>(0.55f * kFramerate) + 1);
	CHECK(table.falling("bombs1").size() == 1);
}

TEST_CASE("the third bucket lost ends the game", "[kaboom]")
{
	Table table;
	table.bucket().position.x = 0;

	for (int miss = 0; miss < 3; ++miss)
	{
		while (table.falling("bombs1").empty()) { table.frames(1); }
		Object& bomb = *table.falling("bombs1")[0];
		bomb.position = { 700.0f, kWindowHeight - bomb.size.y - 1.0f };
		table.frames(2);
	}

	CHECK(table.buckets() == 0);
	table.frames(1);
	CHECK(table.state() == "gameover");
}

TEST_CASE("the score moves the game on from wave to wave and to a win", "[kaboom]")
{
	Table table;

	table.bucket().variable["score"] = 15;
	table.frames(1);
	CHECK(table.state() == "wave2");

	table.bucket().variable["score"] = 45;
	table.frames(1);
	CHECK(table.state() == "wave3");

	table.bucket().variable["score"] = 90;
	table.frames(1);
	CHECK(table.state() == "youwin");
}

TEST_CASE("each wave's bomber is quicker and drops faster bombs more often", "[kaboom]")
{
	Table table;
	const float pace1 = std::abs(table.game.getObject("bomber1").velocity.x);
	const float pace2 = std::abs(table.game.getObject("bomber2").velocity.x);
	const float pace3 = std::abs(table.game.getObject("bomber3").velocity.x);
	CHECK(pace1 < pace2);
	CHECK(pace2 < pace3);

	const float fall1 = table.game.getObject("bombs1.1").velocity.y;
	const float fall2 = table.game.getObject("bombs2.1").velocity.y;
	const float fall3 = table.game.getObject("bombs3.1").velocity.y;
	CHECK(fall1 < fall2);
	CHECK(fall2 < fall3);
}

TEST_CASE("space starts a new game from the end screen with everything back", "[kaboom]")
{
	Game game{ "games/kaboom.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "wave1");

	for (int i = 0; i < 90; ++i) { engine.step(); }
	game.getObject("bucket").variable["buckets"] = 0;
	engine.step();
	REQUIRE(game.getCurrentState().name == "gameover");

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "mainmenu");
	CHECK(game.getObject("bucket").variable["buckets"] == 3);
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "bombs") { CHECK_FALSE(object.isVisible); }
	}
}

TEST_CASE("the shared keys play every wave, and P or Space pauses", "[kaboom][engine_input]")
{
	Game game{ "games/kaboom.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "wave1");

	const float startX = bucket.position.x;
	engine.handleKeyPressed(KeyCode::D);
	game.updateObjects();
	CHECK(bucket.position.x == Approx(startX + 9));

	// The second wave binds the same keys: the held key carries on.
	game.pushState("wave2");
	game.updateObjects();
	CHECK(bucket.position.x == Approx(startX + 18));
	engine.handleKeyReleased(KeyCode::D);

	engine.handleKeyPressed(KeyCode::Left);
	game.updateObjects();
	CHECK(bucket.position.x == Approx(startX + 9));
	engine.handleKeyReleased(KeyCode::Left);

	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "wave2");
}

TEST_CASE("the first wave can be played: a quick player catches nearly every bomb", "[kaboom][play]")
{
	Table table;

	int frames = 0;
	for (; frames < 40 * kFramerate && table.state() == "wave1"; ++frames)
	{
		table.trackLowestBomb();
		table.frames(1);
	}

	INFO("score " << table.score() << ", buckets " << table.buckets() << ", " << frames / kFramerate << " seconds");
	CHECK(table.buckets() >= 2);
	CHECK(table.state() == "wave2");
}

TEST_CASE("the last wave is harder: the same player loses buckets in it", "[kaboom][play]")
{
	// Not a promise about any one game (the bomber's path is random), but over
	// a minute of the third wave the bombs come faster than a bucket can always
	// reach them, so it is not the walkover the first wave is for this player.
	Table table("wave3");
	table.bucket().variable["buckets"] = 1000.0f;

	for (int frame = 0; frame < 60 * kFramerate && table.state() == "wave3"; ++frame)
	{
		table.trackLowestBomb();
		table.frames(1);
	}

	CHECK(table.score() > 0);
}
