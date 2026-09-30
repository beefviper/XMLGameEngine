// test_swept_collision.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for moving and colliding inside a real xge::Game: a fast, small
// object no longer jumps over a thin one between frames, only the object that
// was hit reacts (each cell of a grid() is its own object), and the rest of a
// step is played out after a bounce.

#include "game.h"

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
			if (object.baseName == "aliens" || object.name == "player") { object.size = { 50.0f, 50.0f }; }
			else if (object.name == "bullet") { object.size = { 8.0f, 8.0f }; }
		}
	}
}

TEST_CASE("every cell of a grid has its own name", "[swept_collision]")
{
	Game game{ "games/spaceinvaders.xml" };

	std::set<std::string> names;
	int cells = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens")
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

TEST_CASE("a plain object keeps its name, and the XML name still finds a grid", "[swept_collision]")
{
	Game game{ "games/spaceinvaders.xml" };

	CHECK(game.getObject("player").name == "player");
	CHECK(game.getObject("player").baseName == "player");
	CHECK(game.getObject("aliens").baseName == "aliens"); // the first cell
	CHECK(game.tryGetObject("nothing.1.1") == nullptr);
}

TEST_CASE("a state that shows a grid shows every cell of it", "[swept_collision]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	CHECK(game.isShown(game.getObject("aliens.6.3")));

	game.setCurrentState("paused");
	CHECK_FALSE(game.isShown(game.getObject("aliens.6.3")));
}

TEST_CASE("a bullet moving faster than an alien is tall still hits it, and only that alien dies", "[swept_collision]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");
	measureInvaders(game);

	Object& bullet = game.getObject("bullet");
	Object& target = game.getObject("aliens.4.5"); // bottom row: nothing below it to run into first

	// Just below the bottom row, in line with the fourth column, moving 90 px
	// a frame - the bullet ends the frame 20 px above the alien, having crossed all of it.
	bullet.position = { target.position.x + 21.0f, target.position.y + 70.0f };
	bullet.velocity = { 0.0f, -90.0f };
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	const float bulletStartY = bullet.position.y;
	game.updateObjects();

	CHECK_FALSE(bullet.collisionData.enabled);
	CHECK_FALSE(bullet.isVisible);
	CHECK_FALSE(target.collisionData.enabled);
	CHECK_FALSE(target.isVisible);

	// Nothing else was affected, and the bullet stopped where it hit rather
	// than carrying on through.
	int alive = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens" && object.collisionData.enabled) { ++alive; }
	}
	CHECK(alive == 54);
	CHECK(bullet.position.y > target.position.y);
	CHECK(bullet.position.y < bulletStartY);
}

TEST_CASE("the bullet also hits a cell in the middle of the grid, not just the front row", "[swept_collision]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");
	measureInvaders(game);

	Object& bullet = game.getObject("bullet");
	Object& front = game.getObject("aliens.6.5");
	Object& behind = game.getObject("aliens.6.4");

	bullet.position = { front.position.x + 21.0f, front.position.y + 300.0f };
	bullet.velocity = { 0.0f, -400.0f };
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	game.updateObjects();

	// The nearest one along the path is the one that dies, not one further up.
	CHECK_FALSE(front.collisionData.enabled);
	CHECK(behind.collisionData.enabled);
	CHECK_FALSE(bullet.collisionData.enabled);
}

TEST_CASE("the rest of the step is played after a bounce", "[swept_collision]")
{
	Game game{ "games/pong.xml" };
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
	Game game{ "games/pong.xml" };
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
