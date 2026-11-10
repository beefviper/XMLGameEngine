// collapse.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <collapse>: The rows above what a check found come down.
	static Command makeCollapse(const RawCommand&, const ValueEvaluator&)
	{
		return CmdCollapse{};
	}

	static bool runCollapse(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.match) { executor.game.collapseCells(*doer.match); }
		return true;
	}

	extern const Verb collapseVerb;
	const Verb collapseVerb{
		.name = "collapse",
		.forms = { { "<collapse />", places::cells, kindOf<CmdCollapse>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCollapse,
		.check = nullptr,
		.run = runCollapse,
	};
}
