// test_input_resolution.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for CommandExecutor::triggerObjectAction/applyActionVelocity
// (xge/lib/play/executor.cpp) against a real xge::Game built from
// games/descriptions/tennis.xml - not a reimplementation. Game's constructor fully
// evaluates every object's <action> list (including each move's numeric
// step) without needing a Window/backend first (see the comment on Game's
// construction in xge/cli/source/main.cpp), so tennis.xml's player, who walks
// all four ways, is enough on its own; no Engine required.
//
// Regression covered: holding one direction and then tapping the opposite
// one used to just overwrite the whole axis with the new key's velocity
// (0 on release), stopping the object instead of letting the still-held
// key resume it. See Object::activeMoveStep for the fix.

#include "play/executor.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	// the player's own <action> step, straight from games/descriptions/tennis.xml's
	// <variable name="step">3</variable>.
	constexpr float kStep = 3.0f;
}

TEST_CASE("holding one direction and tapping the opposite cancels, then resumes on release", "[input_resolution]")
{
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = game.getObject("player");

	// Press and hold Down.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "down" } }, true);
	CHECK(player.velocity.y == kStep);

	// Tap Up while Down is still held: the two forces should cancel out,
	// not have Up's key event simply overwrite the axis.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "up" } }, true);
	CHECK(player.velocity.y == 0.0f);

	// Release Up: Down is still physically held, so the player should
	// resume moving down instead of staying stopped.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "up" } }, false);
	CHECK(player.velocity.y == kStep);

	// Finally release Down: nothing left held on this axis.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "down" } }, false);
	CHECK(player.velocity.y == 0.0f);
}

TEST_CASE("two independent axes combine into a diagonal", "[input_resolution]")
{
	// The scenario an upcoming digital-D-pad game needs: two of the player's
	// four bound directions held together should move it diagonally,
	// each axis resolved independently of the other.
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = game.getObject("player");

	executor.executeInput(Command{ CmdTriggerAction{ "player", "left" } }, true);
	executor.executeInput(Command{ CmdTriggerAction{ "player", "up" } }, true);

	CHECK(player.velocity.x == -kStep);
	CHECK(player.velocity.y == -kStep);

	// Releasing one axis leaves the other axis's motion untouched.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "left" } }, false);
	CHECK(player.velocity.x == 0.0f);
	CHECK(player.velocity.y == -kStep);
}

TEST_CASE("a full-game reset clears any direction still recorded as held", "[input_resolution]")
{
	// Guards the fix in Game::resetObjectState: if a key was never released
	// across a reset() (e.g. the player was still holding it), a later,
	// unrelated key event on the same object shouldn't be able to resurrect
	// the old held direction from stale state.
	Game game{ "descriptions/tennis.xml" };
	CommandExecutor executor(game);
	Object& player = game.getObject("player");

	executor.executeInput(Command{ CmdTriggerAction{ "player", "down" } }, true);
	CHECK(player.velocity.y == kStep);

	game.resetAll();
	CHECK(player.velocity.y == 0.0f);

	// Left was never touched; pressing it now shouldn't drag the old,
	// never-released Down contribution back in.
	executor.executeInput(Command{ CmdTriggerAction{ "player", "left" } }, true);
	CHECK(player.velocity.x == -kStep);
	CHECK(player.velocity.y == 0.0f);
}
