// test_demonattack.cpp
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026
//
// Catch2 tests for games/descriptions/demonattack.xml: three waves of demons (flapping
// ones, ones that split in two when shot, and big ones that take two hits),
// the bombs they drop, and the cannon's three lives. The waves are the scenes
// of the playing state, and every demon is of the class demon. It is played frame by
// frame through a real xge::Engine with no window, with the keys a player
// would press: Space starts, A and D (or the arrows) move, Space fires.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
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

	struct Play
	{
		Game game;
		Engine engine;

		explicit Play(const char* file)
			: game(file), engine(game, std::make_unique<FakeWindow>())
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		std::string state() { return game.getCurrentState().name; }
		std::string wave() { return game.currentScene("playing"); }
		Object& object(const std::string& name) { return game.getObject(name); }

		// The demons of a wave (playing.wave2.), not hidden or dead.
		int alive(const std::string& wave)
		{
			int n = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (inWave(object, wave) && object.isVisible) { ++n; }
			}
			return n;
		}

		static bool inWave(const Object& object, const std::string& wave)
		{
			return object.objClass == "demon" && object.name.rfind("playing." + wave + ".", 0) == 0;
		}

		// Holds still every demon of a wave, so a test is not about a lane
		// drifting into the thing it checks.
		void freeze(const std::string& wave)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (inWave(object, wave)) { object.velocity = { 0.0f, 0.0f }; }
			}
		}
	};
}

// -------------------------------------------------------------- Demon Attack

namespace
{
	// Takes every demon of a wave out, as shooting them all would, and lets the
	// condition that sees none left move the game on.
	void clearWave(Play& play, const std::string& wave)
	{
		for (auto& object : play.game.getCurrentObjects())
		{
			if (Play::inWave(object, wave)) { object.isVisible = false; object.collisionData.enabled = false; }
		}
		play.frames(2);
	}

	// The cannon under a demon, a shot fired and given time to arrive.
	void shoot(Play& play, const std::string& demon)
	{
		play.object("player").position.x = play.object(demon).position.x;
		play.tap(KeyCode::Space);
		play.frames(80);
	}
}

TEST_CASE("demonattack.xml loads a cannon, a shot and three waves of demons", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");

	CHECK(play.state() == "title");
	CHECK(play.alive("wave1") == 6);
	CHECK(play.alive("wave2") == 4);
	CHECK(play.alive("wave3") == 3);
	CHECK_FALSE(play.object("laser").isVisible);
	CHECK(play.object("player").variable["lives"] == 3.0f);
}

TEST_CASE("the cannon slides on A and D and the arrows", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");
	CHECK(play.wave() == "playing.wave1");

	const float x = play.object("player").position.x;
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.object("player").position.x > x);

	const float after = play.object("player").position.x;
	play.engine.handleKeyPressed(KeyCode::Left);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::Left);
	CHECK(play.object("player").position.x < after);
}

TEST_CASE("a shot kills a flapping demon and scores thirty, and the last one moves on to the second wave", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	play.freeze("wave1");

	shoot(play, "playing.wave1.demons.1");
	CHECK_FALSE(play.object("playing.wave1.demons.1").isVisible);
	CHECK(play.object("player").variable["score"] == 30.0f);
	CHECK(play.alive("wave1") == 5);

	clearWave(play, "wave1");
	CHECK(play.wave() == "playing.wave2");
	CHECK(play.object("player").variable["lives"] == 4.0f);
}

TEST_CASE("the shot follows the cannon as it flies", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	play.freeze("wave1");

	Object& player = play.object("player");
	Object& laser = play.object("laser");
	play.tap(KeyCode::Space);
	play.frames(2);
	REQUIRE(laser.isVisible);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(10);
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(1);
	CHECK(laser.position.x == Approx(player.position.x + 18.0f));
}

TEST_CASE("a demon of the second wave splits in two small ones, which score when shot", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	clearWave(play, "wave1");
	REQUIRE(play.wave() == "playing.wave2");

	// the demons stand still; the small ones, which are of the class too, are not yet in play
	for (const char* name : { "playing.wave2.demons.1", "playing.wave2.demons.2", "playing.wave2.demons.3", "playing.wave2.demons.4" }) { play.object(name).velocity = { 0.0f, 0.0f }; }

	shoot(play, "playing.wave2.demons.1");
	CHECK_FALSE(play.object("playing.wave2.demons.1").isVisible);
	CHECK(play.object("player").variable["score"] == 50.0f);

	// three demons and the two small ones where it was
	CHECK(play.alive("wave2") == 5);
	CHECK(play.object("playing.wave2.halves.1.1").isVisible);
	CHECK(play.object("playing.wave2.halves.2.1").isVisible);
	CHECK(play.object("playing.wave2.halves.1.1").velocity.y > 0.0f);

	// the wave is not over until the small ones are gone too
	for (const char* name : { "playing.wave2.demons.2", "playing.wave2.demons.3", "playing.wave2.demons.4" }) { play.object(name).isVisible = false; play.object(name).collisionData.enabled = false; }
	play.frames(2);
	CHECK(play.wave() == "playing.wave2");

	// they dive to the bottom of the window and are put away
	play.frames(400);
	CHECK(play.wave() == "playing.wave3");
}

TEST_CASE("a big demon of the third wave takes two shots", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	clearWave(play, "wave1");
	clearWave(play, "wave2");
	REQUIRE(play.wave() == "playing.wave3");
	play.freeze("wave3");

	shoot(play, "playing.wave3.demons.1");
	CHECK(play.object("playing.wave3.demons.1").isVisible);
	CHECK(play.object("player").variable["score"] == 20.0f);

	shoot(play, "playing.wave3.demons.1");
	CHECK_FALSE(play.object("playing.wave3.demons.1").isVisible);
	CHECK(play.object("player").variable["score"] == 120.0f);

	clearWave(play, "wave3");
	CHECK(play.state() == "youwin");
}

TEST_CASE("a demon on the cannon costs a life, and three is game over", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	play.freeze("wave1");

	Object& player = play.object("player");
	Object& demon = play.object("playing.wave1.demons.4");
	player.position = { demon.position.x - 40.0f, demon.position.y };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(8);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(player.variable["lives"] == 2.0f);

	player.variable["lives"] = 0.0f;
	play.frames(2);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(player.variable["lives"] == 3.0f);
}

TEST_CASE("the demons drop bombs on the cannon, and a hit costs a life", "[demonattack]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	Object& player = play.object("player");

	// Wait for a bomb, then put the cannon under it.
	Object* bomb = nullptr;
	for (int frame = 0; frame < 900 && !bomb; ++frame)
	{
		play.frames(1);
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.objClass == "bomb" && object.isVisible) { bomb = &object; break; }
		}
	}
	REQUIRE(bomb);
	CHECK(bomb->velocity.y > 0);

	const float lives = player.variable["lives"];
	player.position.x = bomb->position.x - 15.0f;
	play.frames(150);
	CHECK(player.variable["lives"] < lives);
}

TEST_CASE("only the wave being played runs: the later waves wait where they start, at their own speeds", "[demonattack][scenes]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);

	Object& waiting = play.object("playing.wave2.demons.1");
	const float x = waiting.position.x;
	play.frames(120);
	CHECK(play.game.isAway(waiting));
	CHECK(waiting.position.x == Approx(x));

	CHECK(std::abs(play.object("playing.wave1.demons.1").velocity.x) == Approx(2));
	CHECK(std::abs(waiting.velocity.x) == Approx(2.5));
	CHECK(std::abs(play.object("playing.wave3.demons.1").velocity.x) == Approx(3));

	// A new game starts at the first wave again, everything as it was.
	clearWave(play, "wave1");
	REQUIRE(play.wave() == "playing.wave2");
	play.object("player").variable["lives"] = 0.0f;
	play.frames(2);
	REQUIRE(play.state() == "gameover");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.wave() == "playing.wave1");
	CHECK(play.alive("wave1") == 6);
}

TEST_CASE("paused, the wave is drawn still underneath and nothing moves", "[demonattack][scenes]")
{
	Play play("descriptions/demonattack.xml");
	play.tap(KeyCode::Space);
	play.frames(30);

	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	Object& demon = play.object("playing.wave1.demons.1");
	const float x = demon.position.x;
	CHECK(play.game.isStill(demon));
	CHECK(play.game.isStill(play.object("player")));
	CHECK(play.game.isStill(play.object("score")));
	CHECK_FALSE(play.game.isStill(play.object("playing.wave2.demons.1")));
	play.frames(60);
	CHECK(demon.position.x == Approx(x));

	play.tap(KeyCode::Escape);
	CHECK(play.state() == "playing");
	play.frames(5);
	CHECK(demon.position.x != Approx(x));
}
