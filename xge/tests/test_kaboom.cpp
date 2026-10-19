// test_kaboom.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/descriptions/kaboom.xml, played frame by frame by a real
// xge::Game with no window. The Mad Bomber paces the rooftop, turning when a
// <timer> with a random wait says so, and drops bombs from a pool on another
// timer, from under himself (<facing>down</facing>). A bomb caught goes back
// into the pool; one on the ground costs a bucket and puts every bomb of the
// wave back at once (<reset object>). The eight waves are the scenes of the
// playing state, each taking the bomber and his pool from the kit with its own
// pace, drop, fall and points, and the bucket stack (three looks) is the state's own.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "play/executor.h"
#include "engine/engine.h"
#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <tuple>
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

	// The path of a wave's piece: playing.wave2.bomber.
	std::string of(const std::string& wave, const std::string& piece) { return "playing." + wave + "." + piece; }

	struct Table
	{
		Game game{ "descriptions/kaboom.xml" };
		CommandExecutor executor{ game };

		explicit Table(const std::string& wave = "wave1")
		{
			measure(game);
			game.setCurrentState(0);
			game.pushState("playing");
			if (wave != "wave1") { game.enterScene("playing." + wave); }
		}

		Object& bucket() { return game.getObject("bucket"); }
		float score() { return bucket().variable["score"]; }
		float buckets() { return bucket().variable["buckets"]; }
		std::string state() { return game.getCurrentState().name; }
		std::string wave() { return game.currentScene("playing"); }

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

		// The bombs of a wave's pool that are falling.
		std::vector<Object*> falling(const std::string& wave = "wave1")
		{
			std::vector<Object*> found;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.groupName == of(wave, "bombs") && object.isVisible) { found.push_back(&object); }
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

TEST_CASE("kaboom.xml loads a stack of buckets, and a bomber and a pool of ten bombs in each of eight waves", "[kaboom]")
{
	Table table;

	CHECK(table.bucket().name == "playing.bucket");
	CHECK(table.wave() == "playing.wave1");

	for (const char* wave : { "wave1", "wave2", "wave3", "wave4", "wave5", "wave6", "wave7", "wave8" })
	{
		const Object& object = table.game.getObject(of(wave, "bomber"));
		CHECK(object.hasFacing);
		CHECK(object.facing == Direction::Down);
		CHECK(object.timers.size() == 2);
		CHECK(object.position.y + object.size.y <= 104.0f + 1.0f);

		int members = 0;
		for (const auto& bomb : table.game.getCurrentObjects())
		{
			if (bomb.groupName != of(wave, "bombs")) { continue; }
			++members;
			CHECK_FALSE(bomb.isVisible);
			CHECK_FALSE(bomb.collisionData.enabled);
		}
		CHECK(members == 10);
	}

	CHECK(table.buckets() == 3);
	CHECK(table.bucket().lookName() == "three");
	CHECK(table.score() == 0);
}

TEST_CASE("the bomber drops a bomb from under himself every so often, and it falls straight down", "[kaboom]")
{
	Table table;
	const Object& bomber = table.game.getObject(of("wave1", "bomber"));

	table.frames(static_cast<int>(0.7f * kFramerate) - 1);
	CHECK(table.falling().empty());
	table.frames(1);

	const auto first = table.falling();
	REQUIRE(first.size() == 1);
	const Object& bomb = *first[0];
	CHECK(bomb.velocity.x == Approx(0));
	CHECK(bomb.velocity.y == Approx(2.5));

	// Under the middle of the bomber, below the rooftop.
	CHECK(std::abs((bomb.position.x + bomb.size.x / 2) - (bomber.position.x + bomber.size.x / 2)) < 8.0f);
	CHECK(bomb.position.y >= bomber.position.y + bomber.size.y);

	// Three more, while the bucket (wherever the bomber's wanderings put the
	// bombs) catches none of them.
	table.bucket().collisionData.enabled = false;
	table.frames(static_cast<int>(0.7f * kFramerate) * 3 + 1);
	CHECK(table.falling().size() == 4);
}

TEST_CASE("the bomber stays on the roof, turns at the sides, and changes his mind in between", "[kaboom][random]")
{
	Table table;
	table.keepPlaying();
	Object& bomber = table.game.getObject(of("wave1", "bomber"));

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

TEST_CASE("a caught bomb scores one to eight by the wave, and goes back into the pool", "[kaboom]")
{
	for (const auto& [wave, points] : { std::tuple{ "wave1", 1.0f }, std::tuple{ "wave2", 2.0f }, std::tuple{ "wave3", 3.0f }, std::tuple{ "wave8", 8.0f } })
	{
		Table table(wave);
		table.keepPlaying();

		// Wait for the first bomb, then put the bucket under it.
		while (table.falling(wave).empty()) { table.frames(1); }
		Object& bomb = *table.falling(wave)[0];
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
	table.frames(static_cast<int>(0.7f * kFramerate) * 3 + 2);
	auto falling = table.falling();
	REQUIRE(falling.size() == 3);

	// The bucket out of the way, and the lowest bomb a frame from the ground.
	table.bucket().position.x = 0;
	Object& lowest = *falling[0];
	lowest.position = { 700.0f, kWindowHeight - lowest.size.y - 1.0f };
	table.frames(2); // it lands on the second

	CHECK(table.buckets() == 2);
	CHECK(table.bucket().lookName() == "two"); // one bucket off the stack
	CHECK(table.falling().empty());
	CHECK(table.score() == 0);

	// The bomber goes on dropping.
	table.frames(static_cast<int>(0.7f * kFramerate) + 1);
	CHECK(table.falling().size() == 1);
}

TEST_CASE("the third bucket lost ends the game", "[kaboom]")
{
	Table table;
	table.bucket().position.x = 0;

	for (int miss = 0; miss < 3; ++miss)
	{
		while (table.falling().empty()) { table.frames(1); }
		Object& bomb = *table.falling()[0];
		bomb.position = { 700.0f, kWindowHeight - bomb.size.y - 1.0f };
		table.frames(2);
	}

	CHECK(table.buckets() == 0);
	CHECK(table.bucket().lookName() == "one");
	table.frames(1);
	CHECK(table.state() == "gameover");
}

TEST_CASE("the score moves the game on from wave to wave and to a win", "[kaboom]")
{
	Table table;

	const float ends[] = { 10, 50, 140, 300, 550, 1000, 1700 };
	for (int wave = 0; wave < 7; ++wave)
	{
		table.bucket().variable["score"] = ends[wave];
		table.frames(1);
		CHECK(table.state() == "playing");
		CHECK(table.wave() == "playing.wave" + std::to_string(wave + 2));
	}

	table.bucket().variable["score"] = 2900;
	table.frames(1);
	CHECK(table.state() == "youwin");
}

TEST_CASE("a wave left is away: its bomber stops where he was and drops nothing more", "[kaboom][scenes]")
{
	Table table;
	table.keepPlaying();
	table.frames(static_cast<int>(0.7f * kFramerate) * 2 + 1);
	REQUIRE(table.falling().size() == 2);

	table.bucket().variable["score"] = 10;
	table.frames(1);
	REQUIRE(table.wave() == "playing.wave2");

	Object& oldBomber = table.game.getObject(of("wave1", "bomber"));
	const float x = oldBomber.position.x;
	table.frames(3 * kFramerate);

	CHECK(table.game.isAway(oldBomber));
	CHECK_FALSE(table.game.isShown(oldBomber));
	CHECK(oldBomber.position.x == Approx(x));
	CHECK(table.falling("wave1").empty()); // put away as the wave ended
	CHECK_FALSE(table.falling("wave2").empty());
}

TEST_CASE("each wave's bomber is quicker and drops faster bombs more often", "[kaboom]")
{
	Table table;
	const float pace1 = std::abs(table.game.getObject(of("wave1", "bomber")).velocity.x);
	const float pace2 = std::abs(table.game.getObject(of("wave2", "bomber")).velocity.x);
	const float pace3 = std::abs(table.game.getObject(of("wave3", "bomber")).velocity.x);
	CHECK(pace1 == Approx(2.5));
	CHECK(pace2 == Approx(3));
	CHECK(pace3 == Approx(3.5));
	CHECK(std::abs(table.game.getObject(of("wave8", "bomber")).velocity.x) == Approx(6));

	const float fall1 = table.game.getObject(of("wave1", "bombs.1")).velocity.y;
	const float fall2 = table.game.getObject(of("wave2", "bombs.1")).velocity.y;
	const float fall3 = table.game.getObject(of("wave3", "bombs.1")).velocity.y;
	CHECK(fall1 == Approx(2.5));
	CHECK(fall2 == Approx(3));
	CHECK(fall3 == Approx(3.5));

	CHECK(table.game.getVariable("playing.wave2.drop") == Approx(0.6));

	// The last wave drops a bomb every 0.24 seconds.
	Table last("wave8");
	last.keepPlaying();
	last.frames(85); // bombs at about 0.24, 0.48, 0.72, 0.96 and 1.2 seconds
	CHECK(last.falling("wave8").size() >= 5);
}

TEST_CASE("space starts a new game from the end screen with everything back", "[kaboom]")
{
	Game game{ "descriptions/kaboom.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");

	game.getObject("bucket").variable["score"] = 10;
	for (int i = 0; i < 90; ++i) { engine.step(); }
	REQUIRE(game.currentScene("playing") == "playing.wave2");
	game.getObject("bucket").variable["buckets"] = 0;
	engine.step();
	REQUIRE(game.getCurrentState().name == "gameover");

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "mainmenu");
	CHECK(game.currentScene("playing") == "playing.wave1");
	CHECK(game.getObject("bucket").variable["buckets"] == 3);
	CHECK(game.getObject("bucket").variable["score"] == 0);
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "bombs") { CHECK_FALSE(object.isVisible); }
	}
}

TEST_CASE("the keys carry on from wave to wave, and P or Space pauses over the game drawn still", "[kaboom][engine_input]")
{
	Game game{ "descriptions/kaboom.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");

	const float startX = bucket.position.x;
	engine.handleKeyPressed(KeyCode::D);
	game.updateObjects();
	CHECK(bucket.position.x == Approx(startX + 9));

	// The next wave is the same state: the held key carries on.
	bucket.variable["score"] = 10;
	game.updateObjects();
	REQUIRE(game.currentScene("playing") == "playing.wave2");
	CHECK(bucket.position.x == Approx(startX + 18));
	engine.handleKeyReleased(KeyCode::D);

	engine.handleKeyPressed(KeyCode::Left);
	game.updateObjects();
	CHECK(bucket.position.x == Approx(startX + 9));
	engine.handleKeyReleased(KeyCode::Left);

	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");

	// The wave being played is drawn still underneath; the one left is not.
	Object& bomber = game.getObject(of("wave2", "bomber"));
	CHECK(game.isStill(bucket));
	CHECK(game.isStill(bomber));
	CHECK(game.isStill(game.getObject("score")));
	CHECK_FALSE(game.isStill(game.getObject(of("wave1", "bomber"))));
	const float x = bomber.position.x;
	for (int i = 0; i < 30; ++i) { engine.step(); }
	CHECK(bomber.position.x == Approx(x));

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");
	CHECK(game.currentScene("playing") == "playing.wave2");
}

TEST_CASE("the first wave can be played: a quick player catches nearly every bomb", "[kaboom][play]")
{
	Table table;

	int frames = 0;
	for (; frames < 40 * kFramerate && table.wave() == "playing.wave1"; ++frames)
	{
		table.trackLowestBomb();
		table.frames(1);
	}

	INFO("score " << table.score() << ", buckets " << table.buckets() << ", " << frames / kFramerate << " seconds");
	CHECK(table.buckets() >= 2);
	CHECK(table.wave() == "playing.wave2");
}

TEST_CASE("the last wave is harder: the same player loses buckets in it", "[kaboom][play]")
{
	// Not a promise about any one game (the bomber's path is random), but over
	// a minute of the last wave the bombs come faster than a bucket can always
	// reach them, so it is not the walkover the first wave is for this player.
	Table table("wave8");
	table.bucket().variable["buckets"] = 1000.0f;

	for (int frame = 0; frame < 60 * kFramerate && table.state() == "playing"; ++frame)
	{
		table.trackLowestBomb();
		table.frames(1);
	}

	CHECK(table.score() > 0);
}
