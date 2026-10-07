// test_combat.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/combat.xml, played through a real xge::Engine with
// no window and the keys two players would press: driving and backing along
// the heading (<thrust>), turning (<turn>) and stopping (<drag>), a shell
// fired from the gun along the heading, a hit scoring and sending the tank
// back to its corner, the walls, and the first to five winning.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

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

	struct Combat
	{
		Game game{ "games/combat.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Combat()
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

		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
		}

		void hold(KeyCode key, int count)
		{
			engine.handleKeyPressed(key);
			frames(count);
			engine.handleKeyReleased(key);
		}

		std::string state() { return game.getCurrentState().name; }
		Object& tank1() { return game.getObject("tank1"); }
		Object& tank2() { return game.getObject("tank2"); }
	};
}

TEST_CASE("combat.xml loads with the tanks facing each other", "[combat]")
{
	Combat play;
	CHECK(play.state() == "title");
	play.start();
	CHECK(play.tank1().heading == Approx(90.0f));
	CHECK(play.tank2().heading == Approx(270.0f));
	CHECK(play.tank1().position.x < play.tank2().position.x);
}

TEST_CASE("a tank drives the way it points, backs up, and stops when let go", "[combat]")
{
	Combat play;
	play.start();
	const Vector2f from = play.tank1().position;

	play.hold(KeyCode::W, 30);
	CHECK(play.tank1().position.x > from.x + 30.0f);
	CHECK(play.tank1().position.y == Approx(from.y).margin(0.5f));
	play.frames(60);
	CHECK(std::hypot(play.tank1().velocity.x, play.tank1().velocity.y) < 0.05f);

	const float forward = play.tank1().position.x;
	play.hold(KeyCode::S, 30);
	play.frames(60);
	CHECK(play.tank1().position.x < forward - 10.0f);

	// A quarter turn to the right points it down the field.
	play.hold(KeyCode::D, 30);
	CHECK(play.tank1().heading == Approx(180.0f));
	const float y = play.tank1().position.y;
	play.hold(KeyCode::W, 20);
	CHECK(play.tank1().position.y > y + 15.0f);
}

TEST_CASE("a shell flies from the gun, one at a time, and a hit scores and sends the tank home", "[combat]")
{
	Combat play;
	play.start();
	// The two tanks face each other along the middle, with the middle wall
	// in between: take it away for this.
	play.game.getObject("walls.1").isVisible = false;

	play.tap(KeyCode::F);
	play.frames(1);
	Object& shell = play.game.getObject("shell1");
	REQUIRE(shell.isVisible);
	CHECK(shell.velocity.x == Approx(6.0f));
	CHECK(shell.velocity.y == Approx(0.0f).margin(0.01f));
	CHECK(shell.position.x > play.tank1().position.x + play.tank1().size.x / 2);

	// A second press while it flies does not fire another.
	const Vector2f flying = shell.position;
	play.tap(KeyCode::F);
	play.frames(1);
	CHECK(shell.position.x > flying.x);

	// Tank two moves off its corner; the hit puts it back.
	play.hold(KeyCode::Up, 10);
	const Vector2f home = play.tank2().positionOriginal;
	CHECK(play.tank2().position.x < home.x);
	play.frames(120);
	CHECK(play.tank1().variable.at("score") == 1);
	CHECK_FALSE(shell.isVisible);
	CHECK(play.tank2().position.x == Approx(home.x));
	CHECK(play.tank2().heading == Approx(270.0f));
}

TEST_CASE("a shell stops at a wall, and a tank bounces off one", "[combat]")
{
	Combat play;
	play.start();

	play.tap(KeyCode::RShift);
	play.frames(120);
	CHECK_FALSE(play.game.getObject("shell2").isVisible);
	CHECK(play.tank1().variable.at("score") == 0);
	CHECK(play.tank2().variable.at("score") == 0);

	// Straight at the middle wall.
	const Object& wall = play.game.getObject("walls.1");
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(240);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.tank1().position.x + play.tank1().size.x < wall.position.x + 10.0f);
}

TEST_CASE("five hits win, and space plays again", "[combat]")
{
	Combat play;
	play.start();

	play.tap(KeyCode::Space);
	CHECK(play.state() == "paused");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

	play.tank2().variable["score"] = 5;
	play.frames(2);
	CHECK(play.state() == "wins2");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.tank2().variable.at("score") == 0);
}
