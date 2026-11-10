// clear.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <clear>: Takes away the cells a check found.
	static Command makeClear(const RawCommand&, const ValueEvaluator&)
	{
		return CmdClear{};
	}

	static bool runClear(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.match) { executor.game.clearCells(*doer.match); }
		return true;
	}

	extern const Verb clearVerb;
	const Verb clearVerb{
		.name = "clear",
		.forms = { { "<clear />", places::cells, kindOf<CmdClear>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeClear,
		.check = nullptr,
		.run = runClear,
	};
}
