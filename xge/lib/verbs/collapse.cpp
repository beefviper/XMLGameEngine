// collapse.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <collapse>: The rows above what a check found come down.
	static Command makeCollapse(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdCollapse{};
	}

	static bool runCollapse(const CmdCollapse&, const Doer& doer)
	{
		if (doer.match) { doer.game().collapseCells(*doer.match); }
		return true;
	}

	static void printCollapse(std::ostream& o, const CmdCollapse&)
	{
		o << "collapse";
	}

	extern const Verb collapseVerb;
	const Verb collapseVerb{
		.name = "collapse",
		.forms = { { "<collapse />", places::cells, kindOf<CmdCollapse>() } },
		.make = makeCollapse,
		.run = runAs<runCollapse>,
		.print = printAs<printCollapse>,
	};
}
