// test_win_condition.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for a <condition remaining="..."> (fires when no more than that
// many matching objects are still in play), using Space Invaders' win: the
// last alien dying sends the game to its gameover state, and reset() from
// there brings everything back.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	void kill(Object& object)
	{
		object.isVisible = false;
		object.collisionData.enabled = false;
	}

	int aliensLeft(Game& game)
	{
		int left = 0;
		for (const auto& object : game.getCurrentObjects())
		{
			if (object.baseName == "aliens" && object.isVisible) { ++left; }
		}
		return left;
	}
}

TEST_CASE("the game is not over while any alien is left", "[win_condition]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	int killed = 0;
	for (auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens" && killed < 54)
		{
			kill(object);
			++killed;
		}
	}

	REQUIRE(aliensLeft(game) == 1);
	game.updateObjects();
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("killing the last alien ends the game", "[win_condition]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	for (auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens") { kill(object); }
	}

	game.updateObjects();
	CHECK(game.getCurrentState().name == "gameover");
	CHECK(game.isShown(game.getObject("youwin")));
	CHECK(game.isShown(game.getObject("continue")));
}

TEST_CASE("the last alien dying to a bullet ends the game", "[win_condition]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	// Window::init() normally measures these.
	for (auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens") { object.size = { 50.0f, 50.0f }; }
		else if (object.name == "bullet") { object.size = { 8.0f, 8.0f }; }
	}

	Object& last = game.getObject("aliens.6.3");
	for (auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens" && &object != &last) { kill(object); }
	}

	Object& bullet = game.getObject("bullet");
	bullet.position = { last.position.x + 21.0f, last.position.y + 58.0f };
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	game.updateObjects(); // the bullet reaches it and both die...
	CHECK_FALSE(last.isVisible);
	game.updateObjects(); // ...and the condition sees it on the next check
	CHECK(game.getCurrentState().name == "gameover");
}

TEST_CASE("reset() from the win screen starts a fresh game", "[win_condition]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	for (auto& object : game.getCurrentObjects())
	{
		if (object.baseName == "aliens") { kill(object); }
	}
	game.updateObjects();
	REQUIRE(game.getCurrentState().name == "gameover");

	CommandExecutor executor(game);
	executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(game.getCurrentState().name == "mainmenu");
	CHECK(aliensLeft(game) == 55);

	// ...and playing again does not end at once.
	game.setCurrentState("playing");
	game.updateObjects();
	CHECK(game.getCurrentState().name == "playing");
}
