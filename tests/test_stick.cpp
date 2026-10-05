// test_stick.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for stick() (CommandExecutor::stick, plus Game's post-move
// re-application of it) against a real xge::Game built from games/pong.xml.
// paddle2 is the only Pong object with all four directions bound, and its
// collision rule is stick() on the top and bottom edges.
//
// Regressions covered: (1) a stick()ed object used to end a frame past the wall
// by up to one frame of velocity, and stayed there once it stopped moving;
// (2) stick() used to zero velocity.x whatever edge was touched, so pushing left
// while pressed against the bottom wall froze the paddle.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	constexpr float kStep = 2.0f;
	constexpr float kWindowHeight = 720.0f;
	constexpr float kPaddleWidth = 30.0f;
	constexpr float kPaddleHeight = 150.0f;
	constexpr float kBottomBound = kWindowHeight - kPaddleHeight;

	// Window::init() normally measures each object's real size once a backend
	// exists; a Game built on its own has {0, 0}, so give paddle2 the size of
	// assets/paddle.jpg.
	Object& startNearBottom(Game& game)
	{
		game.setCurrentState("playing");
		Object& paddle2 = game.getObject("paddle2");
		paddle2.size = { kPaddleWidth, kPaddleHeight };
		paddle2.position.y = kBottomBound - 20.0f;
		return paddle2;
	}

	void press(CommandExecutor& executor, const char* action, bool down)
	{
		executor.executeInput(Command{ CmdTriggerAction{ "paddle2", action } }, down);
	}
}

TEST_CASE("a stick()ed object never ends a frame outside the screen", "[stick]")
{
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame)
	{
		game.updateObjects();
		CHECK(paddle2.position.y <= kBottomBound);
	}
	CHECK(paddle2.position.y == kBottomBound);

	// Releasing Down leaves the paddle stopped, but it must still be inside.
	press(executor, "down", false);
	game.updateObjects();
	CHECK(paddle2.position.y == kBottomBound);
}

TEST_CASE("pressing sideways against the bottom wall still slides", "[stick]")
{
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	REQUIRE(paddle2.position.y == kBottomBound);

	// Left-down: the wall stops the downward part, the paddle keeps sliding left.
	const float startX = paddle2.position.x;
	press(executor, "left", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(paddle2.position.y == kBottomBound);
		CHECK(paddle2.position.x == startX - kStep * static_cast<float>(frame));
	}

	// Right-down: same, the other way.
	press(executor, "left", false);
	const float turnX = paddle2.position.x;
	press(executor, "right", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(paddle2.position.y == kBottomBound);
		CHECK(paddle2.position.x == turnX + kStep * static_cast<float>(frame));
	}
}

TEST_CASE("sliding along the wall after letting go of the wall-ward key", "[stick]")
{
	// The reported bug: hold Down into the bottom wall, let go of Down, then
	// hold Left. The paddle used to nudge up a couple of pixels and freeze.
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	press(executor, "down", false);
	game.updateObjects();

	const float startX = paddle2.position.x;
	press(executor, "left", true);
	for (int frame = 1; frame <= 5; ++frame)
	{
		game.updateObjects();
		CHECK(paddle2.position.x == startX - kStep * static_cast<float>(frame));
		CHECK(paddle2.position.y <= kBottomBound);
	}
}

TEST_CASE("the wall only cancels velocity heading into it", "[stick]")
{
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = startNearBottom(game);

	press(executor, "down", true);
	for (int frame = 0; frame < 30; ++frame) { game.updateObjects(); }
	REQUIRE(paddle2.position.y == kBottomBound);

	// Turning around must move it straight back off the wall.
	press(executor, "down", false);
	press(executor, "up", true);
	game.updateObjects();
	CHECK(paddle2.position.y == kBottomBound - kStep);
}
