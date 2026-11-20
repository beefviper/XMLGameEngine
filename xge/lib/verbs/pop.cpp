// pop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"

#include <string>

namespace xge
{
	// <pop>: Back to the state below, or down to a named one.
	static Command makePop(const WrittenCommand& raw, const ValueEvaluator&)
	{
		return CmdPopState{ raw.state };
	}

	static void checkPop(const CmdPopState& popState, const Checking& checking)
	{
		const std::string& name = popState.name;
		if (!name.empty()) { checkState(checking, name, "<pop state=\"" + name + "\" />"); }
	}

	static bool runPop(const CmdPopState& popStateCommand, const Doer& doer)
	{
		doer.game().popState(popStateCommand.name);
		return true;
	}

	static void printPop(std::ostream& o, const CmdPopState& popState)
	{
		o << "state()"; if (!popState.name.empty()) { o << " state(" << popState.name << ")"; }
	}

	extern const Verb popVerb;
	const Verb popVerb{
		.name = "pop",
		.forms = { { "<pop>", places::flow, kindOf<CmdPopState>() } },
		.make = makePop,
		.check = checkAs<checkPop>,
		.run = runAs<runPop>,
		.print = printAs<printPop>,
	};
}
