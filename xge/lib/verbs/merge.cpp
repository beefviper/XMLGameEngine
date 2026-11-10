// merge.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <merge>: Two alike pieces become one.
	static Command makeMerge(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdMerge{ raw.variable };
	}

	static bool runMerge(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdMerge& merge = std::get<CmdMerge>(command);
		return doer.self && doer.touch && doer.other && executor.game.mergeCells(*doer.self, *doer.other, merge.variable);
	}

	extern const Verb mergeVerb;
	const Verb mergeVerb{
		.name = "merge",
		.forms = { { "<merge>", places::touch, kindOf<CmdMerge>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeMerge,
		.check = nullptr,
		.run = runMerge,
	};
}
