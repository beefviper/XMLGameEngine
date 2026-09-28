// verify_reset_command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026
//
// Not part of the build (see CMakeLists.txt - it isn't listed there).
// A standalone check of parseCommands()'s handling of the new "resetobject"
// token (produced by reset('objectName'), the targetable sibling of the
// original bare reset()) against the *actual* production command.cpp - not
// a reimplementation. parseCommands doesn't touch SFML, so this builds and
// runs with no dependencies at all. Build and run it with:
//   g++ -std=c++20 -Iinclude tests/verify_reset_command.cpp source/command.cpp -o verify_reset && ./verify_reset

#include "command.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <variant>

using namespace xge;

namespace
{
	int failures = 0;

	void expect(bool condition, const std::string& description)
	{
		std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description.c_str());
		if (!condition) { ++failures; }
	}
}

int main()
{
	// reset() with no args still parses to the original, untargeted CmdReset
	// (only ever meaningful inside a collision action) - unchanged behaviour.
	{
		const auto commands = parseCommands({ "collide", "reset" });
		expect(commands.size() == 1, "reset(): produces exactly one command");
		expect(commands.size() == 1 && std::holds_alternative<CmdReset>(commands[0]),
			"reset(): produces the bare, untargeted CmdReset");
	}

	// reset('paddle1') parses to the new targetable CmdResetObject - this is
	// what a state's <input>/<condition> action string actually needs, since
	// there's no "colliding object" to be implicit about outside a collision.
	{
		const auto commands = parseCommands({ "resetobject", "paddle1" });
		expect(commands.size() == 1, "reset('paddle1'): produces exactly one command");
		expect(commands.size() == 1 && std::holds_alternative<CmdResetObject>(commands[0]),
			"reset('paddle1'): produces a CmdResetObject");
		if (commands.size() == 1 && std::holds_alternative<CmdResetObject>(commands[0]))
		{
			expect(std::get<CmdResetObject>(commands[0]).target == "paddle1",
				"reset('paddle1'): target is 'paddle1'");
		}
	}

	// A realistic gameover-screen action string: reset both paddles, then go
	// back to the main menu - exactly what games/pong.xml's gameover state
	// now does on spacebar.
	{
		const auto commands = parseCommands({
			"resetobject", "paddle1",
			"resetobject", "paddle2",
			"state", "mainmenu"
			});
		expect(commands.size() == 3, "reset(p1);reset(p2);state(mainmenu): produces three commands");
		expect(commands.size() == 3
			&& std::holds_alternative<CmdResetObject>(commands[0])
			&& std::get<CmdResetObject>(commands[0]).target == "paddle1"
			&& std::holds_alternative<CmdResetObject>(commands[1])
			&& std::get<CmdResetObject>(commands[1]).target == "paddle2"
			&& std::holds_alternative<CmdPushState>(commands[2])
			&& std::get<CmdPushState>(commands[2]).name == "mainmenu",
			"reset(p1);reset(p2);state(mainmenu): commands are in order with correct targets");
	}

	// operator<< (used by Game::printGame's debug output) round-trips a
	// CmdResetObject readably.
	{
		std::ostringstream out;
		out << Command{ CmdResetObject{ "paddle2" } };
		expect(out.str() == "reset(paddle2)", "operator<<(CmdResetObject): prints 'reset(paddle2)'");
	}

	std::printf("\n%d failure(s)\n", failures);
	return failures == 0 ? 0 : 1;
}
