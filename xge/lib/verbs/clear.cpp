// clear.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <clear>: Takes away the cells a check found.
	static Command makeClear(const RawCommand&, const ValueEvaluator&)
	{
		return CmdClear{};
	}

	static bool runClear(const CmdClear&, const Doer& doer)
	{
		if (doer.match) { doer.game().clearCells(*doer.match); }
		return true;
	}

	static void printClear(std::ostream& o, const CmdClear&)
	{
		o << "clear";
	}

	extern const Verb clearVerb;
	const Verb clearVerb{
		.name = "clear",
		.forms = { { "<clear />", places::cells, kindOf<CmdClear>() } },
		.make = makeClear,
		.run = runAs<runClear>,
		.print = printAs<printClear>,
	};
}
