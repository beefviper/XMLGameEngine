// accelerate.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <accelerate>: A push one way while held.
	static Command makeAccelerate(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdAccelerate{ directionFromName(raw.direction, raw.verb), evaluate(raw.amount), raw.burn };
	}

	static bool runAccelerate(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdAccelerate& accelerate = std::get<CmdAccelerate>(command);
		if (doer.self && doer.action) { executor.applyActionThrust(*doer.self, accelerate.direction, doer.pressed ? accelerate.amount : 0.0f, accelerate.burn); }
		return true;
	}

	extern const Verb accelerateVerb;
	const Verb accelerateVerb{
		.name = "accelerate",
		.forms = { { "<accelerate>", places::controls, kindOf<CmdAccelerate>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makeAccelerate,
		.check = nullptr,
		.run = runAccelerate,
	};
}
