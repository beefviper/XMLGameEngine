// enter.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

namespace xge
{
	// <enter>: Goes to another scene of the state.
	static void readEnter(const XmlNode&, const std::string& where, RawCommand& command)
	{
		if (command.scene.empty() == command.way.empty())
		{
			fail(where, "<enter> names a scene=\"...\" or a way=\"...\", one of them");
		}
	}

	static Command makeEnter(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdEnterScene{ raw.scene, raw.way };
	}

	static void checkEnter(const Command& command, const Checking& checking)
	{
		const CmdEnterScene& enter = std::get<CmdEnterScene>(command);
		checkScene(checking, "enter", enter.scene, enter.way);
	}

	static bool runEnter(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.enter(std::get<CmdEnterScene>(command));
		return true;
	}

	static void printEnter(std::ostream& o, const Command& command)
	{
		const CmdEnterScene& e = std::get<CmdEnterScene>(command);
		o << "enter(" << (e.way.empty() ? e.scene : "way " + e.way) << ")";
	}

	static void printEnterRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(scene " << command.scene << ")";
	}

	extern const Verb enterVerb;
	const Verb enterVerb{
		.name = "enter",
		.forms = { { "<enter>", places::changes, kindOf<CmdEnterScene>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readEnter,
		.make = makeEnter,
		.check = checkEnter,
		.run = runEnter,
		.print = printEnter,
		.printRaw = printEnterRaw,
	};
}
