// test_win_condition.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for a <condition remaining="..."> (fires when no more than that
// many matching objects are still in play), using the fleet fixture's win: the
// last cell dying sends the game to its gameover state, and reset() from
// there brings everything back.

#include "play/executor.h"
#include "game/game.h"
#include "copies.h"
#include "fleet_fixture.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	void kill(Object& object)
	{
		object.isVisible = false;
		object.collisionsOn = false;
	}

	int cellsLeft(Game& game)
	{
		int left = 0;
		for (const auto& object : game.getCurrentObjects())
		{
			if (object.groupName == "fleet" && object.isVisible) { ++left; }
		}
		return left;
	}
}

TEST_CASE("the game is not over while any cell of the fleet is left", "[win_condition]")
{
	Game game{ fleet_fixture::path() };
	game.setCurrentState("playing");

	int killed = 0;
	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "fleet" && killed < 54)
		{
			kill(object);
			++killed;
		}
	}

	REQUIRE(cellsLeft(game) == 1);
	game.updateObjects(xge::kSixtieth);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("killing the last cell of the fleet ends the game", "[win_condition]")
{
	Game game{ fleet_fixture::path() };
	game.setCurrentState("playing");

	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "fleet") { kill(object); }
	}

	game.updateObjects(xge::kSixtieth);
	CHECK(game.getCurrentState().name == "gameover");
	CHECK(game.isShown(game.object("youwin")));
	CHECK(game.isShown(game.object("continue")));
}

TEST_CASE("the last cell dying to a bullet ends the game", "[win_condition]")
{
	Game game{ fleet_fixture::path() };
	game.setCurrentState("playing");

	// Window::init() normally measures these.
	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "fleet") { object.size = { 50.0f, 50.0f }; }
	}

	Object& last = game.object("fleet.6.3");
	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "fleet" && &object != &last) { kill(object); }
	}

	Object& bullet = copies::spawned(game, "bullet");
	bullet.size = { 8.0f, 8.0f };
	bullet.position = { last.position.x + 21.0f, last.position.y + 58.0f };

	game.updateObjects(xge::kSixtieth); // the bullet reaches it and both die...
	CHECK_FALSE(last.isVisible);
	game.updateObjects(xge::kSixtieth); // ...and the condition sees it on the next check
	CHECK(game.getCurrentState().name == "gameover");
}

TEST_CASE("reset() from the win screen starts a fresh game", "[win_condition]")
{
	Game game{ fleet_fixture::path() };
	game.setCurrentState("playing");

	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "fleet") { kill(object); }
	}
	game.updateObjects(xge::kSixtieth);
	REQUIRE(game.getCurrentState().name == "gameover");

	CommandExecutor executor(game);
	executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(game.getCurrentState().name == "title");
	CHECK(cellsLeft(game) == 55);

	// ...and playing again does not end at once.
	game.setCurrentState("playing");
	game.updateObjects(xge::kSixtieth);
	CHECK(game.getCurrentState().name == "playing");
}
