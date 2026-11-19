// settle.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <settle>: A falling shape becomes part of its map.
	static Command makeSettle(const RawCommand&, const ValueEvaluator&)
	{
		return CmdSettle{};
	}

	static bool runSettle(const Command&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self) { executor.game.settle(*doer.self); }
		return true;
	}

	static void printSettle(std::ostream& o, const Command&)
	{
		o << "settle";
	}

	extern const Verb settleVerb;
	const Verb settleVerb{
		.name = "settle",
		.forms = { { "<settle />", places::self, kindOf<CmdSettle>() } },
		.make = makeSettle,
		.run = runSettle,
		.print = printSettle,
	};
}
