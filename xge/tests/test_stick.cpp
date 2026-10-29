// test_stick.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for stick() (CommandExecutor::stick, plus Game's post-move
// re-application of it) against a real xge::Game built from games/descriptions/tennis.xml.
// Its player walks all four ways and stick()s at every edge of the window.
//
// Regressions covered: (1) a stick()ed object used to end a frame past the wall
// by up to one frame of velocity, and stayed there once it stopped moving;
// (2) stick() used to zero velocity.x whatever edge was touched, so pushing left
// while pressed against the bottom wall froze the player.

#include "play/executor.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	constexpr float kStep = 3.0f;
	constexpr float kWindowHeight = 600.0f;
	constexpr float kPlayerHeight = 24.0f; // a bitmap 8 rows tall at scale 3
	constexpr float kBottomBound = kWindowHeight - kPlayerHeight;

	Object& startNearBottom(Game& game)
	{
		game.setCurrentState("playing");
		Object& player = game.getObject("player");
		player.size = measureShapeSize(player.picture);
		player.position.y = kBottomBound - 20.0f;
		return player;
	}

	void press(CommandExecutor& executor, const char* action, bool down)
	{
		executor.executeInput(Command{ CmdTriggerAction{ "player", action } }, down);
	}
}

TEST_CASE("a stick()ed object never ends a frame outside the screen", "[stick]")
{
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame)
	{
		game.updateObjects();
		CHECK(player.position.y <= kBottomBound);
	}
	CHECK(player.position.y == kBottomBound);

	// Releasing Down leaves the player stopped, but it must still be inside.
	press(executor, "down", false);
	game.updateObjects();
	CHECK(player.position.y == kBottomBound);
}

TEST_CASE("pressing sideways against the bottom wall still slides", "[stick]")
{
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	REQUIRE(player.position.y == kBottomBound);

	// Left-down: the wall stops the downward part, the player keeps sliding left.
	const float startX = player.position.x;
	press(executor, "left", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(player.position.y == kBottomBound);
		CHECK(player.position.x == startX - kStep * static_cast<float>(frame));
	}

	// Right-down: same, the other way.
	press(executor, "left", false);
	const float turnX = player.position.x;
	press(executor, "right", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(player.position.y == kBottomBound);
		CHECK(player.position.x == turnX + kStep * static_cast<float>(frame));
	}
}

TEST_CASE("sliding along the wall after letting go of the wall-ward key", "[stick]")
{
	// The reported bug: hold Down into the bottom wall, let go of Down, then
	// hold Left. The player used to nudge up a couple of pixels and freeze.
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	press(executor, "down", false);
	game.updateObjects();

	const float startX = player.position.x;
	press(executor, "left", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(player.position.x == startX - kStep * static_cast<float>(frame));
		CHECK(player.position.y <= kBottomBound);
	}
}

TEST_CASE("the wall only cancels velocity heading into it", "[stick]")
{
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	REQUIRE(player.position.y == kBottomBound);

	// Turning around must move it straight back off the wall.
	press(executor, "down", false);
	press(executor, "up", true);
	game.updateObjects();
	CHECK(player.position.y == kBottomBound - kStep);
}
