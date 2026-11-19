// grow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <grow>: A chain gets one link longer.
	static Command makeGrow(const RawCommand&, const ValueEvaluator&)
	{
		return CmdGrow{};
	}

	static bool runGrow(const CmdGrow&, const Doer& doer)
	{
		Game& game = doer.game();
		if (doer.self && !game.cellPart(*doer.self).chain.empty()) { ++game.placeFor(*doer.self).chainGrow; }
		return true;
	}

	static void printGrow(std::ostream& o, const CmdGrow&)
	{
		o << "grow";
	}

	extern const Verb growVerb;
	const Verb growVerb{
		.name = "grow",
		.forms = { { "<grow />", places::self, kindOf<CmdGrow>() } },
		.make = makeGrow,
		.run = runAs<runGrow>,
		.print = printAs<printGrow>,
	};
}
