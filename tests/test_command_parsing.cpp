// test_command_parsing.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for makeCommands()/operator<<(Command) against the actual
// production command.cpp (not a reimplementation) - they don't touch SFML or
// exprtk, so nothing here needs a real Object/Game. A RawCommand is what
// game_xml reads a command tag (<bounce />, <inc variable="..." />) into.

#include "command.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>
#include <stdexcept>
#include <variant>

using namespace xge;

namespace
{
	// Stands in for game_expr: every value in these tests is a plain number.
	float evaluatePlainNumber(const RawValue& value)
	{
		return std::stof(value.text);
	}

	RawCommand tag(const std::string& verb)
	{
		RawCommand command;
		command.verb = verb;
		return command;
	}

	RawCommand resetOf(const std::string& object)
	{
		RawCommand command = tag("reset");
		command.object = object;
		return command;
	}

	RawCommand pushOf(const std::string& state)
	{
		RawCommand command = tag("push");
		command.state = state;
		return command;
	}
}

TEST_CASE("<reset /> makes the untargeted CmdReset", "[command_parsing]")
{
	// Only ever meaningful inside a collision rule (see
	// CommandExecutor::executeScreenEdgeCollision) or, since a state's
	// <input>/<condition> reuses the same command set, as the full-game reset
	// (CommandExecutor::executeInput/Game::resetAll) - either way, the same
	// untargeted CmdReset.
	const auto commands = makeCommands({ tag("reset") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 1);
	REQUIRE(std::holds_alternative<CmdReset>(commands[0]));
}

TEST_CASE("<reset object=\"name\" /> makes a targeted CmdResetObject", "[command_parsing]")
{
	// What a state's <input>/<condition> needs, since there's no "colliding
	// object" to be implicit about outside a collision.
	const auto commands = makeCommands({ resetOf("paddle1") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 1);
	REQUIRE(std::holds_alternative<CmdResetObject>(commands[0]));
	CHECK(std::get<CmdResetObject>(commands[0]).target == "paddle1");
}

TEST_CASE("a realistic gameover-screen list of commands comes out in order", "[command_parsing]")
{
	// What games/pong.xml's gameover state used to do on spacebar, before it
	// was simplified to the single global <reset />.
	const auto commands = makeCommands({ resetOf("paddle1"), resetOf("paddle2"), pushOf("mainmenu") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 3);

	REQUIRE(std::holds_alternative<CmdResetObject>(commands[0]));
	CHECK(std::get<CmdResetObject>(commands[0]).target == "paddle1");

	REQUIRE(std::holds_alternative<CmdResetObject>(commands[1]));
	CHECK(std::get<CmdResetObject>(commands[1]).target == "paddle2");

	REQUIRE(std::holds_alternative<CmdPushState>(commands[2]));
	CHECK(std::get<CmdPushState>(commands[2]).name == "mainmenu");
}

TEST_CASE("an unknown command tag is an error that names it", "[command_parsing]")
{
	REQUIRE_THROWS_WITH(makeCommands({ tag("explode") }, evaluatePlainNumber), "unknown command <explode>");
}

TEST_CASE("operator<<(CmdResetObject) prints readably", "[command_parsing]")
{
	// Used by Game::printGame's debug output.
	std::ostringstream out;
	out << Command{ CmdResetObject{ "paddle2" } };

	CHECK(out.str() == "reset(paddle2)");
}
