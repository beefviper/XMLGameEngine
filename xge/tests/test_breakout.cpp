// test_breakout.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for Breakout played frame by frame: each colour's bricks score
// their own worth and sound their own pitch; the ball goes faster at the
// fourth and twelfth hit and at the first brick in the back rows; the paddle
// halves once the ball reaches the top wall; three balls, two walls; and each
// state's own objects, with paused and the endings drawing the game still.

#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	// Breakout with the playing screen showing, and the sizes Window::init()
	// would measure.
	void play(Game& game)
	{
		game.setCurrentState("playing");
		for (Object& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	// The ball put inside `brick`, going up, so it meets that brick alone.
	void hit(Game& game, const std::string& brick)
	{
		const Object& target = game.getObject(brick);
		Object& ball = game.getObject("ball");
		ball.position = { target.position.x + 14.0f, target.position.y + 2.0f };
		ball.velocity = { 0.0f, -4.0f };
		game.updateObjects();
	}

	float status(Game& game, const char* name) { return game.getObject("status").variable.at(name); }

	float speed(Game& game)
	{
		const Object& ball = game.getObject("ball");
		return std::hypot(ball.velocity.x, ball.velocity.y);
	}

	int bricksIn(Game& game)
	{
		int count = 0;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.objClass == "brick" && object.isVisible) { ++count; }
		}
		return count;
	}

	void clearWall(Game& game)
	{
		for (Object& object : game.getCurrentObjects())
		{
			if (object.objClass == "brick") { object.isVisible = false; }
		}
	}
}

TEST_CASE("Breakout's wall is eight rows of fourteen, two of each colour", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	CHECK(bricksIn(game) == 112);

	const Object& first = game.getObject("red.1.1");
	CHECK(first.size.x == 38.0f);
	CHECK(first.size.y == 14.0f);
	CHECK(first.position.x == 28.0f); // the wall in the middle of the window
	CHECK(first.position.y == 110.0f);
	CHECK(game.getObject("red.1.2").position.y == 128.0f);
	CHECK(game.getObject("orange.1.1").position.y == 146.0f);
	CHECK(game.getObject("yellow.14.2").position.y == 236.0f);
	CHECK(game.getObject("yellow.14.2").position.x == 28.0f + 13 * 42.0f);
}

TEST_CASE("a brick scores its colour's worth and sounds its colour's pitch", "[breakout][sound]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);

	hit(game, "yellow.5.2");
	CHECK(status(game, "score") == 1.0f);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "yellow" });
	CHECK_FALSE(game.isShown(game.getObject("yellow.5.2")));
	CHECK(game.isShown(game.getObject("yellow.4.2"))); // its neighbour was not touched

	hit(game, "green.5.2");
	CHECK(status(game, "score") == 4.0f);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "green" });

	hit(game, "orange.5.2");
	CHECK(status(game, "score") == 9.0f);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "orange" });

	hit(game, "red.5.2");
	CHECK(status(game, "score") == 16.0f);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "red" });
	CHECK(bricksIn(game) == 108);
}

TEST_CASE("the first brick in the back four rows makes the ball faster, once", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);

	hit(game, "green.3.1");
	CHECK(speed(game) == Approx(4.0f));
	hit(game, "orange.3.2");
	CHECK(speed(game) == Approx(5.0f));
	CHECK(game.getObject("ball").variable.at("deep") == 1.0f);
	hit(game, "red.3.1");
	CHECK(speed(game) == Approx(4.0f)); // hit() serves it at 4 again: no second speed-up
}

TEST_CASE("the fourth and the twelfth hit of the paddle make Breakout's ball faster", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	Object& ball = game.getObject("ball");
	const Object& player = game.getObject("player");

	std::vector<float> speeds;
	for (int bounce = 1; bounce <= 12; ++bounce)
	{
		ball.position = { player.position.x + player.size.x / 2 - 5.0f, player.position.y - 12.0f };
		ball.velocity = { 0.0f, speeds.empty() ? 4.0f : speeds.back() };
		game.updateObjects();
		speeds.push_back(speed(game));
	}
	CHECK(game.takeSoundRequests().front() == "paddle");
	CHECK(speeds[2] == Approx(4.0f));
	CHECK(speeds[3] == Approx(5.0f));
	CHECK(speeds[10] == Approx(5.0f));
	CHECK(speeds[11] == Approx(6.25f));
	CHECK(ball.variable.at("hits") == 12.0f);
}

TEST_CASE("once the ball reaches the top wall, the paddle is half as wide", "[breakout][looks]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	REQUIRE(game.getObject("player").lookName() == "full");

	Object& ball = game.getObject("ball");
	ball.position = { 300.0f, 4.0f };
	ball.velocity = { 0.0f, -3.0f };
	for (int frame = 0; frame < 3; ++frame) { game.updateObjects(); }
	CHECK(ball.velocity.y > 0.0f);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "wall" });
	CHECK(game.getObject("player").lookName() == "half");

	game.resetAll();
	CHECK(game.getObject("player").lookName() == "full");
}

TEST_CASE("three balls lost is the end of Breakout", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	Object& ball = game.getObject("ball");

	for (int lost = 1; lost <= 3; ++lost)
	{
		INFO(lost);
		ball.position = { 300.0f, 715.0f };
		ball.velocity = { 0.0f, 4.0f };
		game.updateObjects();
		CHECK(status(game, "balls") == 3.0f - static_cast<float>(lost));
		CHECK(game.takeSoundRequests() == std::vector<std::string>{ "lost" });
		CHECK(ball.position.y < 400.0f); // served again from the middle
		if (lost < 3) { CHECK(game.getCurrentState().name == "playing"); }
	}
	game.updateObjects();
	CHECK(game.getCurrentState().name == "gameover");
}

TEST_CASE("clearing the first wall builds a second, and clearing that wins", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	game.getObject("status").variable["score"] = 448.0f;

	clearWall(game);
	game.updateObjects();
	game.updateObjects();
	CHECK(status(game, "walls") == 1.0f);
	CHECK(bricksIn(game) == 112);
	CHECK(status(game, "score") == 448.0f); // the score carries on
	CHECK(game.getCurrentState().name == "playing");

	clearWall(game);
	game.updateObjects();
	CHECK(game.getCurrentState().name == "youwin");
}

TEST_CASE("each state of Breakout has its own objects, and paused and the endings draw the game still", "[breakout][scenes][shows]")
{
	Game game{ "descriptions/breakout.xml" };
	game.setCurrentState("mainmenu");
	CHECK(game.getObject("ball").name == "playing.ball");
	CHECK(game.getObject("title").name == "mainmenu.title");
	CHECK(game.getObject("again").name == "again"); // the game's own: both endings show it
	CHECK(game.isShown(game.getObject("title")));
	CHECK_FALSE(game.isShown(game.getObject("ball")));

	play(game);
	CHECK_FALSE(game.isShown(game.getObject("title")));
	CHECK(game.isShown(game.getObject("ball")));
	CHECK(game.isShown(game.getObject("red.5.1")));
	game.updateObjects();

	// Paused: the ball does not move, and the wall and the paddle are drawn under PAUSED.
	game.pushState("paused");
	const Vector2f at = game.getObject("ball").position;
	for (int frame = 0; frame < 10; ++frame) { game.updateObjects(); }
	CHECK(game.getObject("ball").position.x == at.x);
	CHECK(game.getObject("ball").position.y == at.y);
	CHECK(game.isStill(game.getObject("ball")));
	CHECK(game.isStill(game.getObject("player")));
	CHECK(game.isStill(game.getObject("yellow.1.1")));
	CHECK(game.isShown(game.getObject("paused")));
	game.popState();
	CHECK_FALSE(game.isStill(game.getObject("ball")));

	// Each ending is written over the game still, with the shared line under it.
	for (const char* ending : { "youwin", "gameover" })
	{
		INFO(ending);
		game.pushState(ending);
		CHECK(game.isShown(game.getObject(std::string(ending) + ".message")));
		CHECK(game.isShown(game.getObject("again")));
		CHECK(game.isStill(game.getObject("red.1.1")));
		CHECK_FALSE(game.isShown(game.getObject("paused")));
		game.popState();
	}
}
