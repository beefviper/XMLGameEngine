// pop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"

#include <string>

namespace xge
{
	// <pop>: Back to the state below, or down to a named one.
	static Command makePop(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPopState{ raw.state };
	}

	static void checkPop(const Command& command, const Checking& checking)
	{
		const std::string& name = std::get<CmdPopState>(command).name;
		if (!name.empty()) { checkState(checking, name, "<pop state=\"" + name + "\" />"); }
	}

	static bool runPop(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.popState(std::get<CmdPopState>(command).name);
		return true;
	}

	static void printPop(std::ostream& o, const Command& command)
	{
		const CmdPopState& s = std::get<CmdPopState>(command);
		o << "state()"; if (!s.name.empty()) { o << " state(" << s.name << ")"; }
	}

	extern const Verb popVerb;
	const Verb popVerb{
		.name = "pop",
		.forms = { { "<pop>", places::flow, kindOf<CmdPopState>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makePop,
		.check = checkPop,
		.run = runPop,
		.print = printPop,
	};
}
