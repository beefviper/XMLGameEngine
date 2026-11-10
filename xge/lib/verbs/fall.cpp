// fall.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <fall>: Every piece with nothing under it comes down.
	static Command makeFall(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdFall{ raw.objClass };
	}

	static bool runFall(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.match) { executor.game.fallCells(doer.match->map, std::get<CmdFall>(command).objClass); }
		return true;
	}

	extern const Verb fallVerb;
	const Verb fallVerb{
		.name = "fall",
		.forms = { { "<fall />", places::cells, kindOf<CmdFall>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeFall,
		.check = nullptr,
		.run = runFall,
	};
}
