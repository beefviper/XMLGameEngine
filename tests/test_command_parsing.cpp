// test_command_parsing.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for parseCommands()/operator<<(Command) against the actual
// production command.cpp (not a reimplementation) - parseCommands doesn't
// touch SFML, so nothing here needs a real Object/Game.

#include "command.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <variant>

using namespace xge;

TEST_CASE("bare reset() parses to the untargeted CmdReset", "[command_parsing]")
{
	// Only ever meaningful inside a collision action (see
	// CommandExecutor::executeScreenEdgeCollision) or, since a state's
	// <input>/<condition> action reuses the same command set, as the
	// full-game reset (CommandExecutor::executeInput/Game::resetAll) -
	// either way, parsing produces the same untargeted CmdReset.
	const auto commands = parseCommands({ "collide", "reset" });

	REQUIRE(commands.size() == 1);
	REQUIRE(std::holds_alternative<CmdReset>(commands[0]));
}

TEST_CASE("reset('name') parses to a targeted CmdResetObject", "[command_parsing]")
{
	// What a state's <input>/<condition> action string actually needs, since
	// there's no "colliding object" to be implicit about outside a collision.
	const auto commands = parseCommands({ "resetobject", "paddle1" });

	REQUIRE(commands.size() == 1);
	REQUIRE(std::holds_alternative<CmdResetObject>(commands[0]));
	CHECK(std::get<CmdResetObject>(commands[0]).target == "paddle1");
}

TEST_CASE("a realistic gameover-screen action string parses in order", "[command_parsing]")
{
	// Exactly what games/pong.xml's gameover state used to do on spacebar,
	// before it was simplified to the single global reset().
	const auto commands = parseCommands({
		"resetobject", "paddle1",
		"resetobject", "paddle2",
		"state", "mainmenu"
		});

	REQUIRE(commands.size() == 3);

	REQUIRE(std::holds_alternative<CmdResetObject>(commands[0]));
	CHECK(std::get<CmdResetObject>(commands[0]).target == "paddle1");

	REQUIRE(std::holds_alternative<CmdResetObject>(commands[1]));
	CHECK(std::get<CmdResetObject>(commands[1]).target == "paddle2");

	REQUIRE(std::holds_alternative<CmdPushState>(commands[2]));
	CHECK(std::get<CmdPushState>(commands[2]).name == "mainmenu");
}

TEST_CASE("operator<<(CmdResetObject) prints readably", "[command_parsing]")
{
	// Used by Game::printGame's debug output.
	std::ostringstream out;
	out << Command{ CmdResetObject{ "paddle2" } };

	CHECK(out.str() == "reset(paddle2)");
}
