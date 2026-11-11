// test_swept_collision.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for moving and colliding inside a real xge::Game: a fast, small
// object no longer jumps over a thin one between frames, only the object that
// was hit reacts (each cell of a group is its own object), and the rest of a
// step is played out after a bounce.

#include "play/executor.h"
#include "game/game.h"
#include "copies.h"
#include "invaders_fixture.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <set>
#include <string>

using namespace xge;
using Catch::Matchers::WithinAbs;

namespace
{
	// Window::init() normally measures these; a Game built on its own has {0, 0}.
	void measureInvaders(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			if (object.groupName == "aliens" || object.name == "player") { object.size = { 50.0f, 50.0f }; }
		}
	}

	// A bullet, a copy of the piece, in the game now.
	Object& bullet(Game& game)
	{
		Object& made = copies::spawned(game, "bullet");
		made.size = { 8.0f, 8.0f };
		return made;
	}
}

TEST_CASE("every cell of a group has its own name", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };

	std::set<std::string> names;
	int cells = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "aliens")
		{
			names.insert(object.name);
			++cells;
		}
	}

	CHECK(cells == 55);
	CHECK(names.size() == 55);
	CHECK(names.count("aliens.1.1") == 1);
	CHECK(names.count("aliens.11.5") == 1);
	CHECK(names.count("aliens") == 0);

	// Columns and rows count from 1, and follow where the cell is.
	const Object& first = game.getObject("aliens.1.1");
	const Object& lastColumn = game.getObject("aliens.11.1");
	const Object& lastRow = game.getObject("aliens.1.5");
	CHECK(lastColumn.position.x > first.position.x);
	CHECK(lastColumn.position.y == first.position.y);
	CHECK(lastRow.position.y > first.position.y);
	CHECK(lastRow.position.x == first.position.x);
}

TEST_CASE("a plain object keeps its name, and the group's name finds its first cell", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };

	CHECK(game.getObject("player").name == "player");
	CHECK(game.getObject("aliens").name == "aliens.1.1"); // the first cell
	CHECK(game.tryGetObject("nothing.1.1") == nullptr);
}

TEST_CASE("a state that shows a group shows every cell of it", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");

	CHECK(game.isShown(game.getObject("aliens.6.3")));

	game.setCurrentState("paused");
	CHECK_FALSE(game.isShown(game.getObject("aliens.6.3")));
}

TEST_CASE("a bullet moving faster than an alien is tall still hits it, and only that alien dies", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");
	measureInvaders(game);

	Object& target = game.getObject("aliens.4.5"); // bottom row: nothing below it to run into first
	Object& bullet = ::bullet(game);

	// Just below the bottom row, in line with the fourth column, moving 90 px
	// a frame - the bullet ends the frame 20 px above the alien, having crossed all of it.
	bullet.position = { target.position.x + 21.0f, target.position.y + 70.0f };
	bullet.velocity = { 0.0f, -90.0f };
	game.updateObjects();

	// The hit took the bullet away.
	CHECK(copies::count(game, "bullet") == 0);
	CHECK_FALSE(target.collisionsOn);
	CHECK_FALSE(target.isVisible);

	// Nothing else was affected.
	int alive = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "aliens" && object.collisionsOn) { ++alive; }
	}
	CHECK(alive == 54);
}

TEST_CASE("the bullet also hits a cell in the middle of the block, not just the front row", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");
	measureInvaders(game);

	Object& front = game.getObject("aliens.6.5");
	Object& behind = game.getObject("aliens.6.4");
	Object& bullet = ::bullet(game);

	bullet.position = { front.position.x + 21.0f, front.position.y + 300.0f };
	bullet.velocity = { 0.0f, -400.0f };

	game.updateObjects();

	// The nearest one along the path is the one that dies, not one further up.
	CHECK_FALSE(front.collisionsOn);
	CHECK(behind.collisionsOn);
	CHECK(copies::count(game, "bullet") == 0);
}

TEST_CASE("the rest of the step is played after a bounce", "[swept_collision]")
{
	Game game{ "descriptions/pong.xml" };
	game.setCurrentState("playing");

	Object& ball = game.getObject("ball");
	Object& paddle = game.getObject("paddle1");
	ball.size = { 20.0f, 20.0f };
	paddle.size = { 30.0f, 150.0f };
	paddle.position = { 100.0f, 300.0f };
	paddle.velocity = { 0.0f, 0.0f };

	// Heading left at 30 a frame, 10 px from the paddle's right side (x = 130):
	// touches after a third of the step, then has the other 20 px to go back.
	ball.position = { 140.0f, 365.0f };
	ball.velocity = { -30.0f, 0.0f };

	game.updateObjects();

	CHECK(ball.velocity.x == 30.0f);
	CHECK_THAT(ball.position.x, WithinAbs(130.0f + 20.0f, 1e-3f));
}

TEST_CASE("a ball fast enough to pass the paddle in one step bounces off it instead", "[swept_collision]")
{
	Game game{ "descriptions/pong.xml" };
	game.setCurrentState("playing");

	Object& ball = game.getObject("ball");
	Object& paddle = game.getObject("paddle1");
	ball.size = { 20.0f, 20.0f };
	paddle.size = { 30.0f, 150.0f };
	paddle.position = { 100.0f, 300.0f };
	paddle.velocity = { 0.0f, 0.0f };

	ball.position = { 400.0f, 365.0f };
	ball.velocity = { -400.0f, 0.0f };

	game.updateObjects();

	CHECK(ball.velocity.x > 0.0f);
	CHECK(ball.position.x > 130.0f);
}

TEST_CASE("a bullet is a piece: fired, a copy leaves the ship at its own velocity, and once it is gone another can be fired", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");
	measureInvaders(game);

	Object& player = game.getObject("player");
	CommandExecutor executor(game);

	// Not fired: no bullet in the game.
	game.updateObjects();
	CHECK(copies::count(game, "bullet") == 0);

	// Fired: a copy at the ship, moving at the piece's own velocity.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	game.settleSpawns();
	REQUIRE(copies::count(game, "bullet") == 1);
	Object& bullet = game.getObject("bullet.1");
	CHECK(bullet.collisionsOn);
	CHECK(bullet.velocity.y == -12.0f);
	CHECK(bullet.position.y == player.position.y);

	// One at a time.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	game.settleSpawns();
	CHECK(copies::count(game, "bullet") == 1);

	// Off the top: gone, and the gun is ready again.
	for (int frame = 0; frame < 200 && copies::count(game, "bullet") > 0; ++frame)
	{
		game.updateObjects();
	}
	CHECK(copies::count(game, "bullet") == 0);
	executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	game.settleSpawns();
	CHECK(copies::count(game, "bullet") == 1);
	CHECK(game.getObject("bullet.1").position.y == player.position.y);
}

TEST_CASE("a full reset takes away a bullet in flight and brings back a dead alien", "[swept_collision]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");

	Object& alien = game.getObject("aliens.2.2");
	CommandExecutor executor(game);

	executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	game.settleSpawns();
	REQUIRE(copies::count(game, "bullet") == 1);
	alien.isVisible = false;
	alien.collisionsOn = false;

	game.resetAll();
	game.settleSpawns();

	CHECK(copies::count(game, "bullet") == 0);
	CHECK(alien.isVisible);
	CHECK(alien.collisionsOn);
}
