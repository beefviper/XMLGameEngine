// die.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

namespace xge
{
	// <die>: Out of play until something puts it back.
	static Command makeDie(const RawCommand&, const ValueEvaluator&)
	{
		return CmdDie{};
	}

	static bool runDie(const CmdDie&, const Doer& doer)
	{
		if (doer.self) { xge::die(*doer.self); }
		return true;
	}

	static void printDie(std::ostream& o, const CmdDie&)
	{
		o << "die";
	}

	extern const Verb dieVerb;
	const Verb dieVerb{
		.name = "die",
		.forms = { { "<die />", places::self, kindOf<CmdDie>() } },
		.make = makeDie,
		.run = runAs<runDie>,
		.print = printAs<printDie>,
	};
}
