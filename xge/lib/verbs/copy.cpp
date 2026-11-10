// copy.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <copy>: One variable's value into another.
	static Command makeCopy(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdCopy{ VariableName::of(raw.variable), VariableName::of(raw.into) };
	}

	static bool runCopy(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdCopy& copy = std::get<CmdCopy>(command);
		executor.game.copyVariable(copy.from, copy.into);
		return true;
	}

	extern const Verb copyVerb;
	const Verb copyVerb{
		.name = "copy",
		.forms = { { "<copy>", places::changes, kindOf<CmdCopy>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCopy,
		.run = runCopy,
	};
}
