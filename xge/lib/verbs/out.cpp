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

	static bool runOut(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { executor.game.passTurn(true); }
		return true;
	}

	static void printOut(std::ostream& o, const Command&)
	{
		o << "out()";
	}

	extern const Verb outVerb;
	const Verb outVerb{
		.name = "out",
		.forms = { { "<out />", places::flow, kindOf<CmdOut>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeOut,
		.check = nullptr,
		.run = runOut,
		.print = printOut,
	};
}
