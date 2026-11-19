// out.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <out>: The player whose turn it is has finished.
	static Command makeOut(const RawCommand&, const ValueEvaluator&)
	{
		return CmdOut{};
	}

	static bool runOut(const CmdOut&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (!doer.self) { executor.game.passTurn(true); }
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
