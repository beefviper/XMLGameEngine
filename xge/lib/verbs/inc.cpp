// inc.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <inc>: Adds to a variable.
	static Command makeInc(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// By its amount, or by 1 when it has none.
		return CmdIncrement{ VariableName::of(raw.variable), valueOr(raw.amount, 1.0f, evaluate) };
	}

	static bool runInc(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdIncrement& inc = std::get<CmdIncrement>(command);
		executor.game.addTo(inc.target, inc.amount);
		return true;
	}

	extern const Verb incVerb;
	const Verb incVerb{
		.name = "inc",
		.forms = { { "<inc>", places::changes, kindOf<CmdIncrement>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeInc,
		.run = runInc,
	};
}
