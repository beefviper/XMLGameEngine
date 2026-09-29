// test_input_resolution.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for CommandExecutor::triggerObjectAction/applyActionVelocity
// (source/command_executor.cpp) against a real xge::Game built from
// games/pong.xml - not a reimplementation. Game's constructor fully
// evaluates every object's <action> list (including each move's numeric
// step) without needing a Window/backend first (see the comment on Game's
// construction in source/main.cpp), so paddle2 - the only pong.xml object
// bound to all four directions - is enough on its own; no Engine required.
//
// Regression covered: holding one direction and then tapping the opposite
// one used to just overwrite the whole axis with the new key's velocity
// (0 on release), stopping the object instead of letting the still-held
// key resume it. See Object::activeMoveStep for the fix.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	// paddle2's own <action> step, straight from games/pong.xml's
	// <variable name="step" value="2" />.
	constexpr float kStep = 2.0f;
}

TEST_CASE("holding one direction and tapping the opposite cancels, then resumes on release", "[input_resolution]")
{
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = game.getObject("paddle2");

	// Press and hold Down.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "down" } }, true);
	CHECK(paddle2.velocity.y == kStep);

	// Tap Up while Down is still held: the two forces should cancel out,
	// not have Up's key event simply overwrite the axis.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "up" } }, true);
	CHECK(paddle2.velocity.y == 0.0f);

	// Release Up: Down is still physically held, so the paddle should
	// resume moving down instead of staying stopped.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "up" } }, false);
	CHECK(paddle2.velocity.y == kStep);

	// Finally release Down: nothing left held on this axis.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "down" } }, false);
	CHECK(paddle2.velocity.y == 0.0f);
}

TEST_CASE("two independent axes combine into a diagonal", "[input_resolution]")
{
	// The scenario an upcoming digital-D-pad game needs: two of paddle2's
	// four bound directions held together should move it diagonally,
	// each axis resolved independently of the other.
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = game.getObject("paddle2");

	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "left" } }, true);
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "up" } }, true);

	CHECK(paddle2.velocity.x == -kStep);
	CHECK(paddle2.velocity.y == -kStep);

	// Releasing one axis leaves the other axis's motion untouched.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "left" } }, false);
	CHECK(paddle2.velocity.x == 0.0f);
	CHECK(paddle2.velocity.y == -kStep);
}

TEST_CASE("a full-game reset clears any direction still recorded as held", "[input_resolution]")
{
	// Guards the fix in Game::resetObjectState: if a key was never released
	// across a reset() (e.g. the player was still holding it), a later,
	// unrelated key event on the same object shouldn't be able to resurrect
	// the old held direction from stale state.
	Game game{ "games/pong.xml" };
	CommandExecutor executor(game);
	Object& paddle2 = game.getObject("paddle2");

	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "down" } }, true);
	CHECK(paddle2.velocity.y == kStep);

	game.resetAll();
	CHECK(paddle2.velocity.y == 0.0f);

	// Left was never touched; pressing it now shouldn't drag the old,
	// never-released Down contribution back in.
	executor.executeInput(Command{ CmdTriggerAction{ "paddle2", "left" } }, true);
	CHECK(paddle2.velocity.x == -kStep);
	CHECK(paddle2.velocity.y == 0.0f);
}
