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

	static bool runSplit(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self) { executor.game.splitChain(*doer.self); }
		return true;
	}

	static void printSplit(std::ostream& o, const Command&)
	{
		o << "split";
	}

	extern const Verb splitVerb;
	const Verb splitVerb{
		.name = "split",
		.forms = { { "<split />", places::self, kindOf<CmdSplit>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeSplit,
		.check = nullptr,
		.run = runSplit,
		.print = printSplit,
	};
}
