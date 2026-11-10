// climb.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

#include <stdexcept>

namespace xge
{
	// <climb>: Up or down a ladder while held.
	static Command makeClimb(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const Direction direction = directionFromName(raw.direction, raw.verb);
		if (direction != Direction::Up && direction != Direction::Down)
		{
			throw std::runtime_error("<climb> has direction=\"" + raw.direction + "\"; expected up or down");
		}
		return CmdClimb{ direction, evaluate(raw.amount), raw.objClass };
	}

	static bool runClimb(const Command& command, const Doer& doer, CommandExecutor&)
	{
		const CmdClimb& climb = std::get<CmdClimb>(command);
		Object* const self = doer.self;
		if (!self || !doer.action) { return true; }
		self->activeClimb[climb.direction == Direction::Up ? 0 : 1] = doer.pressed ? climb.step : 0.0f;
		self->climbClass = climb.ladderClass;
		self->climbOnce = doer.pressed && doer.byRule;
		return true;
	}

	extern const Verb climbVerb;
	const Verb climbVerb{
		.name = "climb",
		.forms = { { "<climb>", places::controls, kindOf<CmdClimb>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makeClimb,
		.check = nullptr,
		.run = runClimb,
	};
}
