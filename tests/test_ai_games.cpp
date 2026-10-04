// test_ai_games.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for games/demonattack.xml, one of three games written by
// another AI from the schema alone to see whether the language was
// expressive enough (Frostbite and Berserk have been rewritten since and are
// tested in test_frostbite.cpp and test_berserk.cpp). It is played frame by
// frame through a real xge::Engine with no window, with the keys a player
// would press: Space starts, A and D (or the arrows) move, Space fires.
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
