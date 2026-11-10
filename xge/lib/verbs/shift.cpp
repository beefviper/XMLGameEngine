// shift.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <shift>: Moves an object, and all of a group, by an amount.
	static Command makeShift(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdShift{ raw.object, { valueOr(raw.positionX, 0.0f, evaluate), valueOr(raw.positionY, 0.0f, evaluate) } };
	}

	static bool runShift(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdShift& shift = std::get<CmdShift>(command);
		executor.game.shift(shift.target, shift.by);
		return true;
	}

	extern const Verb shiftVerb;
	const Verb shiftVerb{
		.name = "shift",
		.forms = { { "<shift>", places::changes, kindOf<CmdShift>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeShift,
		.run = runShift,
	};
}
