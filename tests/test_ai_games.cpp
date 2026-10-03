// test_ai_games.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for games/berserk.xml and games/demonattack.xml, two of three
// games written by another AI from the schema alone to see whether the
// language was expressive enough (the third, Frostbite, has been rewritten
// since and is tested in test_frostbite.cpp). Each is played frame by
// frame through a real xge::Engine with no window, with the keys a player
// would press: Space starts, W, A, S, D (or the arrows) move, Space fires.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

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
		Object& object(const char* name) { return game.getObject(name); }

		int alive(const char* objClass)
		{
			int n = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && object.isVisible) { ++n; }
			}
			return n;
		}

		// Holds still every object of a class, so a test is not about a lane
		// drifting into the thing it checks.
		void freeze(const char* objClass)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass) { object.velocity = { 0.0f, 0.0f }; }
			}
		}
	};
}

// ------------------------------------------------------------------ Berserk

TEST_CASE("berserk.xml loads a player, a bullet, a maze and four robots", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");

	CHECK(play.state() == "title");
	CHECK(play.object("player").isVisible);
	CHECK_FALSE(play.object("bullet").isVisible);
	CHECK(play.object("walls").size.x > 0.0f);
	CHECK(play.alive("robot") == 4);
	CHECK(play.object("player").variable["lives"] == 3.0f);
}

TEST_CASE("Space starts Berserk and the keys move the player", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "room1");

	const float x = play.object("player").position.x;
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::A);
	CHECK(play.object("player").position.x < x);

	const float y = play.object("player").position.y;
	play.engine.handleKeyPressed(KeyCode::Up);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::Up);
	CHECK(play.object("player").position.y < y);
}

TEST_CASE("a shot kills the robot it hits and scores ten", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");
	play.tap(KeyCode::Space);
	play.freeze("robot");

	Object& player = play.object("player");
	Object& robot = play.object("r4");
	player.position = { robot.position.x, 170.0f };

	play.tap(KeyCode::Space); // fire
	CHECK(play.object("bullet").isVisible);
	play.frames(60);

	CHECK_FALSE(robot.isVisible);
	CHECK(play.alive("robot") == 3);
	CHECK(play.object("player").variable["score"] == 10.0f);
}

TEST_CASE("a robot touching the player costs a life and sends the player back", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");
	play.tap(KeyCode::Space);
	play.freeze("robot");

	Object& player = play.object("player");
	const Vector2f start = player.position;
	Object& robot = play.object("r4");
	player.position = { robot.position.x - 14.0f, robot.position.y };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(3);
	play.engine.handleKeyReleased(KeyCode::D);

	CHECK(player.variable["lives"] == 2.0f);
	CHECK(player.position.x == Catch::Approx(start.x).margin(8.0));
	CHECK(player.position.y == start.y);
}

TEST_CASE("killing every robot moves to the next room, and no lives is game over", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");
	play.tap(KeyCode::Space);

	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "robot") { object.isVisible = false; object.collisionData.enabled = false; }
	}
	play.frames(2);
	CHECK(play.state() == "room2");

	play.object("player").variable["lives"] = 0.0f;
	play.frames(2);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.object("player").variable["lives"] == 3.0f);
	CHECK(play.alive("robot") == 4);
}

// -------------------------------------------------------------- Demon Attack

TEST_CASE("demonattack.xml loads a cannon, a shot and seven demons", "[ai_games][demonattack]")
{
	Play play("games/demonattack.xml");

	CHECK(play.state() == "title");
	CHECK(play.alive("alien") == 7);
	CHECK_FALSE(play.object("bullet").isVisible);
	CHECK(play.object("player").variable["lives"] == 3.0f);
}

TEST_CASE("the cannon slides on A and D and the arrows", "[ai_games][demonattack]")
{
	Play play("games/demonattack.xml");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

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

TEST_CASE("a shot kills a demon and scores ten, and the last one brings the whole wave back", "[ai_games][demonattack]")
{
	Play play("games/demonattack.xml");
	play.tap(KeyCode::Space);
	play.freeze("alien");

	Object& demon = play.object("a1");
	play.object("player").position.x = demon.position.x;
	play.tap(KeyCode::Space);
	play.frames(80);

	CHECK_FALSE(demon.isVisible);
	CHECK(play.object("player").variable["score"] == 10.0f);

	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "alien") { object.isVisible = false; object.collisionData.enabled = false; }
	}
	play.frames(2);
	CHECK(play.alive("alien") == 7);
}

TEST_CASE("a demon on the cannon costs a life, and three is game over", "[ai_games][demonattack]")
{
	Play play("games/demonattack.xml");
	play.tap(KeyCode::Space);
	play.freeze("alien");

	Object& player = play.object("player");
	Object& demon = play.object("a5");
	player.position = { demon.position.x - 18.0f, demon.position.y };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(4);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(player.variable["lives"] == 2.0f);

	player.variable["lives"] = 0.0f;
	play.frames(2);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(player.variable["lives"] == 3.0f);
}

// ------------------------------------------------------- firing back, facing

TEST_CASE("Berserk's man shoots the way he last walked", "[ai_games][berserk][facing]")
{
	Play play("games/berserk.xml");
	play.tap(KeyCode::Space);
	play.freeze("robot");
	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "robot") { object.timers.clear(); } // no robot fire in this test
	}

	Object& player = play.object("player");
	Object& bullet = play.object("bullet");
	player.position = { 60.0f, 160.0f };

	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(3);
	play.engine.handleKeyReleased(KeyCode::A);
	CHECK(player.facing == Direction::Left);

	play.tap(KeyCode::Space);
	REQUIRE(bullet.isVisible);
	CHECK(bullet.velocity.x < 0);
	CHECK(bullet.velocity.y == 0);
	CHECK(bullet.position.x + bullet.size.x <= player.position.x);

	play.frames(80);
	play.engine.handleKeyPressed(KeyCode::S);
	play.frames(1);
	play.engine.handleKeyReleased(KeyCode::S);
	play.tap(KeyCode::Space);
	CHECK(bullet.velocity.y > 0);
}

TEST_CASE("Berserk's robots fire back, and their shots and the electrified walls cost a life", "[ai_games][berserk]")
{
	Play play("games/berserk.xml");
	play.tap(KeyCode::Space);
	Object& player = play.object("player");

	// Within ten seconds the robots have fired.
	bool fired = false;
	for (int frame = 0; frame < 600 && !fired; ++frame)
	{
		play.frames(1);
		fired = play.alive("robotshot") > 0;
	}
	CHECK(fired);

	// Walking into the maze's wall.
	player.position = { 80.0f, 86.0f };
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(10);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(player.variable["lives"] <= 2.0f);
	CHECK(player.position.y == Approx(190.0f).margin(30.0f)); // sent back to the start
}

TEST_CASE("Demon Attack's demons fire down at the cannon, and a hit costs a life", "[ai_games][demonattack]")
{
	Play play("games/demonattack.xml");
	play.tap(KeyCode::Space);
	Object& player = play.object("player");

	// Wait for a shot, then put the cannon under it.
	Object* shot = nullptr;
	for (int frame = 0; frame < 600 && !shot; ++frame)
	{
		play.frames(1);
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.objClass == "demonshot" && object.isVisible) { shot = &object; break; }
		}
	}
	REQUIRE(shot);
	CHECK(shot->velocity.y > 0);

	const float lives = player.variable["lives"];
	player.position.x = shot->position.x - 6.0f;
	play.frames(120);
	CHECK(player.variable["lives"] < lives);
}
