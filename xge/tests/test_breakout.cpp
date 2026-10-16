// test_breakout.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for Breakout played frame by frame: the top row of bricks
// (group strong) takes two hits, a whole brick cracking at the first and
// going out of play at the second, and a reset makes it whole again; and the
// sounds the ball asks for off a wall, the paddle and the bricks; and each
// state's own objects, with paused and the endings drawing the game still.

#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace xge;

namespace
{
	// Breakout with the playing screen showing, and the sizes Window::init()
	// would measure: the ball 20 across, every brick 120 by 30.
	void play(Game& game)
	{
		game.setCurrentState("playing");
		for (Object& object : game.getCurrentObjects())
		{
			if (object.name == "playing.ball") { object.size = { 20.0f, 20.0f }; }
			if (object.objClass == "bricks") { object.size = { 120.0f, 30.0f }; }
		}
	}

	// The ball put just above `brick`, over its middle, falling onto it.
	void dropOnto(Game& game, const Object& brick)
	{
		Object& ball = game.getObject("ball");
		ball.position = { brick.position.x + 60.0f - 10.0f, brick.position.y - 24.0f };
		ball.velocity = { 0.0f, 4.0f };
	}

	// Frames until the ball has bounced back up, at most 20.
	void playUntilBounced(Game& game)
	{
		for (int frame = 0; frame < 20 && game.getObject("ball").velocity.y > 0.0f; ++frame)
		{
			game.updateObjects();
		}
	}
}

TEST_CASE("a brick of the top row cracks at the first hit and goes at the second", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);

	const Object& brick = game.getObject("strong.5.1");
	REQUIRE(brick.lookName() == "whole");

	dropOnto(game, brick);
	playUntilBounced(game);
	CHECK(game.getObject("ball").velocity.y < 0.0f);
	CHECK(game.getObject("strong.5.1").lookName() == "cracked");
	CHECK(game.isShown(game.getObject("strong.5.1")));

	dropOnto(game, game.getObject("strong.5.1"));
	playUntilBounced(game);
	CHECK(game.getObject("ball").velocity.y < 0.0f);
	CHECK_FALSE(game.isShown(game.getObject("strong.5.1")));

	// its neighbours were not touched
	CHECK(game.getObject("strong.4.1").lookName() == "whole");
	CHECK(game.isShown(game.getObject("strong.4.1")));
}

TEST_CASE("starting Breakout again makes a cracked brick whole", "[breakout]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);

	dropOnto(game, game.getObject("strong.1.1"));
	playUntilBounced(game);
	REQUIRE(game.getObject("strong.1.1").lookName() == "cracked");

	game.resetAll();
	CHECK(game.getObject("strong.1.1").lookName() == "whole");
}

TEST_CASE("Breakout's ball asks for a sound off a wall, the paddle, a cracking brick and a brick", "[breakout][sound]")
{
	Game game{ "descriptions/breakout.xml" };
	play(game);
	game.getObject("player").size = { 120.0f, 30.0f };

	// off the top wall
	Object& ball = game.getObject("ball");
	ball.position = { 600.0f, 4.0f };
	ball.velocity = { 0.0f, -3.0f };
	for (int frame = 0; frame < 5; ++frame) { game.updateObjects(); }
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "wall" });

	// onto the paddle
	const Object& player = game.getObject("player");
	ball.position = { player.position.x + 50.0f, player.position.y - 24.0f };
	ball.velocity = { 0.0f, 4.0f };
	playUntilBounced(game);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "paddle" });

	// the top row cracks at the first hit and goes at the second
	dropOnto(game, game.getObject("strong.5.1"));
	playUntilBounced(game);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "crack" });
	dropOnto(game, game.getObject("strong.5.1"));
	playUntilBounced(game);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "brick" });

	// any other brick goes at the first
	dropOnto(game, game.getObject("bricks.5.1"));
	playUntilBounced(game);
	CHECK(game.takeSoundRequests() == std::vector<std::string>{ "brick" });
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
	CHECK(game.isShown(game.getObject("strong.5.1")));
	game.updateObjects();

	// Paused: the ball does not move, and the wall and the paddle are drawn under PAUSED.
	game.pushState("paused");
	const Vector2f at = game.getObject("ball").position;
	for (int frame = 0; frame < 10; ++frame) { game.updateObjects(); }
	CHECK(game.getObject("ball").position.x == at.x);
	CHECK(game.getObject("ball").position.y == at.y);
	CHECK(game.isStill(game.getObject("ball")));
	CHECK(game.isStill(game.getObject("player")));
	CHECK(game.isStill(game.getObject("bricks.1.1")));
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
		CHECK(game.isStill(game.getObject("strong.1.1")));
		CHECK_FALSE(game.isShown(game.getObject("paused")));
		game.popState();
	}
}
