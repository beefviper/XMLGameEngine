// cycle.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <cycle>: The cells of a shape change places in turn.
	static Command makeCycle(const RawCommand&, const ValueEvaluator&)
	{
		return CmdCycle{};
	}

	static bool runCycle(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self) { executor.game.cycleCells(*doer.self); }
		return true;
	}

	extern const Verb cycleVerb;
	const Verb cycleVerb{
		.name = "cycle",
		.forms = { { "<cycle />", places::self, kindOf<CmdCycle>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCycle,
		.check = nullptr,
		.run = runCycle,
	};
}
