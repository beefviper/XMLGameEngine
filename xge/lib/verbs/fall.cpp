// fall.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <fall>: Every piece with nothing under it comes down.
	static Command makeFall(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdFall{ raw.objClass };
	}

	static bool runFall(const CmdFall& fall, const Doer& doer)
	{
		if (doer.match) { doer.game().fallCells(doer.match->map, fall.objClass); }
		return true;
	}

	static void printFall(std::ostream& o, const CmdFall& fall)
	{
		o << "fall"; if (!fall.objClass.empty()) { o << "(" << fall.objClass << ")"; }
	}

	extern const Verb fallVerb;
	const Verb fallVerb{
		.name = "fall",
		.forms = { { "<fall />", places::cells, kindOf<CmdFall>() } },
		.make = makeFall,
		.run = runAs<runFall>,
		.print = printAs<printFall>,
	};
}
