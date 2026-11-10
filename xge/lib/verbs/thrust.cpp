// thrust.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <thrust>: A push the way it faces while held.
	static Command makeThrust(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdThrust{ evaluate(raw.amount), raw.burn };
	}

	static bool runThrust(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdThrust& thrust = std::get<CmdThrust>(command);
		if (doer.self && doer.action) { executor.applyActionThrustAhead(*doer.self, *doer.action, doer.pressed ? thrust.amount : 0.0f, thrust.burn); }
		return true;
	}

	extern const Verb thrustVerb;
	const Verb thrustVerb{
		.name = "thrust",
		.forms = { { "<thrust>", places::controls, kindOf<CmdThrust>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makeThrust,
		.run = runThrust,
	};
}
