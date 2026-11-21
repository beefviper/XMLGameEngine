// out.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <out>: The player whose turn it is has finished.
	static Command makeOut(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdOut{};
	}

	static bool runOut(const CmdOut&, const Doer& doer)
	{
		if (!doer.self) { doer.game().passTurn(true); }
		return true;
	}

	static void printOut(std::ostream& o, const CmdOut&)
	{
		o << "out()";
	}

	extern const Verb outVerb;
	const Verb outVerb{
		.name = "out",
		.forms = { { "<out />", places::flow, kindOf<CmdOut>() } },
		.make = makeOut,
		.run = runAs<runOut>,
		.print = printAs<printOut>,
	};
}
