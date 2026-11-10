// set.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <string>
#include <utility>

namespace xge
{
	// <set>: Gives a variable, or one of an object's own numbers, a value.
	static Command makeSet(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// One of the numbers every object has (ship.velocity.x) is split before
		// its part, not at its last dot.
		const std::string part = liveNumberPart(raw.variable);
		VariableName target = part.empty() ? VariableName::of(raw.variable)
			: VariableName{ raw.variable.substr(0, raw.variable.size() - part.size() - 1), part };
		return CmdSet{ std::move(target), evaluate(raw.amount) };
	}

	static bool runSet(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdSet& set = std::get<CmdSet>(command);
		executor.game.setNumber(set.target, set.value);
		return true;
	}

	extern const Verb setVerb;
	const Verb setVerb{
		.name = "set",
		.forms = { { "<set>", places::changes, kindOf<CmdSet>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeSet,
		.run = runSet,
	};
}
