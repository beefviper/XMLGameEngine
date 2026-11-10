// dec.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <dec>: Takes from a variable.
	static Command makeDec(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// By its amount, or by 1 when it has none.
		return CmdDecrement{ VariableName::of(raw.variable), valueOr(raw.amount, 1.0f, evaluate) };
	}

	static bool runDec(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdDecrement& dec = std::get<CmdDecrement>(command);
		executor.game.addTo(dec.target, -dec.amount);
		return true;
	}

	extern const Verb decVerb;
	const Verb decVerb{
		.name = "dec",
		.forms = { { "<dec>", places::changes, kindOf<CmdDecrement>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeDec,
		.run = runDec,
	};
}
