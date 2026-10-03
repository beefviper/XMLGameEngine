// test_ai_games.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for games/berserk.xml, games/demonattack.xml and
// games/frostbite.xml, three games written by another AI from the schema alone
// to see whether the language was expressive enough. Each is played frame by
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

// ----------------------------------------------------------------- Frostbite

TEST_CASE("frostbite.xml loads a builder, an igloo, ten floes, a bird and a fish", "[ai_games][frostbite]")
{
	Play play("games/frostbite.xml");

	CHECK(play.state() == "title");
	CHECK(play.alive("floe") == 10);
	CHECK(play.alive("bird") == 1);
	CHECK(play.alive("fish") == 1);
	CHECK(play.object("player").variable["temperature"] == 120.0f);
}

TEST_CASE("the four keys hop the builder about, a step at a time", "[ai_games][frostbite]")
{
	Play play("games/frostbite.xml");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

	Object& player = play.object("player");
	const float y = player.position.y;
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(player.position.y < y);
}

TEST_CASE("a floe carries the builder along", "[ai_games][frostbite]")
{
	Play play("games/frostbite.xml");
	play.tap(KeyCode::Space);

	Object& player = play.object("player");
	Object& floe = play.object("f10");
	player.position = { floe.position.x + 8.0f, floe.position.y };
	const float x = player.position.x;
	play.frames(10);
	CHECK(player.position.x > x);
}

TEST_CASE("the cold takes a degree a second, and a bird or a fish takes ten", "[ai_games][frostbite]")
{
	Play play("games/frostbite.xml");
	play.tap(KeyCode::Space);
	play.freeze("floe");
	play.freeze("bird");
	play.freeze("fish");

	Object& player = play.object("player");
	play.frames(75);
	CHECK(player.variable["temperature"] <= 119.0f);
	CHECK(player.variable["temperature"] >= 118.0f);

	const float before = player.variable["temperature"];
	Object& bird = play.object("b1");
	player.position = { bird.position.x - 14.0f, bird.position.y };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(4);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(player.variable["temperature"] <= before - 10.0f);
}

TEST_CASE("reaching the igloo scores fifty, and freezing is game over", "[ai_games][frostbite]")
{
	Play play("games/frostbite.xml");
	play.tap(KeyCode::Space);
	play.freeze("floe");
	play.freeze("bird");
	play.freeze("fish");

	Object& player = play.object("player");
	Object& igloo = play.object("igloo");
	player.position = { igloo.position.x, igloo.position.y + 30.0f };
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(10);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.object("player").variable["score"] == 50.0f);

	player.variable["temperature"] = 0.0f;
	play.frames(2);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(player.variable["temperature"] == 120.0f);
}
