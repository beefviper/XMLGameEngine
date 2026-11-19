// split.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <split>: Cuts a chain where it was touched.
	static Command makeSplit(const RawCommand&, const ValueEvaluator&)
	{
		return CmdSplit{};
	}

	static bool runSplit(const CmdSplit&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self) { executor.game.splitChain(*doer.self); }
		return true;
	}

	static void printSplit(std::ostream& o, const CmdSplit&)
	{
		o << "split";
	}

	extern const Verb splitVerb;
	const Verb splitVerb{
		.name = "split",
		.forms = { { "<split />", places::self, kindOf<CmdSplit>() } },
		.make = makeSplit,
		.run = runAs<runSplit>,
		.print = printAs<printSplit>,
	};
}
