// test_combat.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/descriptions/combat.xml, played through a real xge::Engine with
// no window and the keys two players would press: driving and backing along
// the heading (<thrust>), turning (<turn>) and stopping (<drag>), a shell
// fired from the gun along the heading, a hit scoring and knocking the tank
// back the way the shell flew, the walls, and the clock ending the game with
// the most hits winning.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

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

	struct Combat
	{
		Game game{ "descriptions/combat.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Combat()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
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

TEST_CASE("a shell flies from the gun, one at a time, and a hit scores and knocks the tank back", "[combat]")
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

	// Tank two drives toward it; the hit knocks it back the way the shell flew.
	play.engine.handleKeyPressed(KeyCode::Up);
	bool knocked = false;
	for (int frame = 0; frame < 120 && !knocked; ++frame)
	{
		play.frames(1);
		knocked = play.tank2().velocity.x > 0.0f;
	}
	play.engine.handleKeyReleased(KeyCode::Up);
	CHECK(knocked);
	CHECK(shell.variable.at("hits") == 1);
	CHECK_FALSE(shell.isVisible);
	CHECK(play.tank2().heading == Approx(270.0f)); // knocked, not turned
}

TEST_CASE("a shell stops at a wall, and a tank bounces off one", "[combat]")
{
	Combat play;
	play.start();

	play.tap(KeyCode::RShift);
	play.frames(120);
	CHECK_FALSE(play.game.getObject("shell2").isVisible);
	CHECK(play.game.getObject("shell1").variable.at("hits") == 0);
	CHECK(play.game.getObject("shell2").variable.at("hits") == 0);

	// Straight at the middle wall.
	const Object& wall = play.game.getObject("walls.1");
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(240);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.tank1().position.x + play.tank1().size.x < wall.position.x + 10.0f);
}

TEST_CASE("after two minutes sixteen the most hits win, and space plays again", "[combat]")
{
	Combat play;
	play.start();

	play.tap(KeyCode::Space);
	CHECK(play.state() == "paused");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

	play.game.getObject("shell2").variable["hits"] = 3;
	play.frames(135 * 60);
	CHECK(play.state() == "playing");
	play.frames(62);
	REQUIRE(play.state() == "wins");
	CHECK(play.game.getObject("wins.winner").lookName() == "blue");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.game.getObject("shell2").variable.at("hits") == 0);
	CHECK(play.game.getObject("wins.winner").lookName() == "draw");
}

TEST_CASE("paused, the field is drawn still: a shell in flight stops where it is", "[combat]")
{
	Combat play;
	play.start();
	play.game.getObject("walls.1").isVisible = false;
	play.tap(KeyCode::F);
	play.frames(5);
	Object& shell = play.game.getObject("shell1");
	REQUIRE(shell.isVisible);
	const Vector2f shellAt = shell.position;

	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	play.frames(30);
	CHECK(shell.position == shellAt);
	CHECK(play.game.isStill(shell));
	CHECK(play.game.isStill(play.tank1()));
	CHECK(play.game.isStill(play.game.getObject("walls.2")));
	CHECK_FALSE(play.game.isStill(play.game.getObject("shell2"))); // never fired
	CHECK_FALSE(play.game.isShown(play.tank1()));
	CHECK(play.game.isShown(play.game.getObject("paused")));

	play.tap(KeyCode::P);
	REQUIRE(play.state() == "playing");
	play.frames(1);
	CHECK(shell.position != shellAt);
}

TEST_CASE("each state owns what it draws, and a win draws the field still under its message", "[combat]")
{
	Combat play;
	CHECK(play.tank1().ownerState == "playing");
	CHECK(play.game.getObject("keys1").ownerState == "title");
	CHECK(play.game.getObject("again").ownerState == "wins");
	CHECK(play.game.isShown(play.game.getObject("title.title")));
	CHECK_FALSE(play.game.isShown(play.tank1()));

	play.start();
	play.game.getObject("shell1").variable["hits"] = 5;
	play.game.pushState("wins");
	play.frames(1);
	REQUIRE(play.state() == "wins");
	CHECK(play.game.isStill(play.tank2()));
	CHECK(play.game.isStill(play.game.getObject("score1")));
	CHECK(play.game.isShown(play.game.getObject("wins.winner")));
	CHECK(play.game.getObject("wins.winner").lookName() == "yellow");
	CHECK(play.game.isShown(play.game.getObject("again")));
}
