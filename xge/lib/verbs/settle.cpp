// settle.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <settle>: A falling shape becomes part of its map.
	static Command makeSettle(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdSettle{};
	}

	static bool runSettle(const CmdSettle&, const Doer& doer)
	{
		if (doer.self) { doer.game().settle(*doer.self); }
		return true;
	}

	static void printSettle(std::ostream& o, const CmdSettle&)
	{
		o << "settle";
	}

	extern const Verb settleVerb;
	const Verb settleVerb{
		.name = "settle",
		.forms = { { "<settle />", places::self, kindOf<CmdSettle>() } },
		.make = makeSettle,
		.run = runAs<runSettle>,
		.print = printAs<printSettle>,
	};
}
