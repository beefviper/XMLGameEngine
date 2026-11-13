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

	static bool runDie(const Command&, const Doer& doer, CommandExecutor&)
	{
		if (doer.self) { xge::die(*doer.self); }
		return true;
	}

	static void printDie(std::ostream& o, const Command&)
	{
		o << "die";
	}

	extern const Verb dieVerb;
	const Verb dieVerb{
		.name = "die",
		.forms = { { "<die />", places::self, kindOf<CmdDie>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeDie,
		.check = nullptr,
		.run = runDie,
		.print = printDie,
	};
}
