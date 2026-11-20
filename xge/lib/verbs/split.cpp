// split.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <split>: Cuts a chain where it was touched.
	static Command makeSplit(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdSplit{};
	}

	static bool runSplit(const CmdSplit&, const Doer& doer)
	{
		if (doer.self) { doer.game().cellMaps().splitChain(*doer.self); }
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
