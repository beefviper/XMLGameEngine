// swing.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <algorithm>
#include <stdexcept>

namespace xge
{
	// <swing>: One eased stroke round its pivot while held, and back when let
	// go.
	static Command makeSwing(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const Direction direction = rotationFromName(raw.direction, raw.verb);
		const float seconds = evaluate(raw.seconds);
		if (!(seconds > 0.0f)) { throw std::runtime_error("<swing> takes <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
		const float degrees = evaluate(raw.distance);
		if (!(degrees > 0.0f)) { throw std::runtime_error("<swing> takes <degrees> of " + formatDisplayNumber(degrees) + "; expected more than 0"); }
		return CmdSwing{ direction == Direction::Left ? -degrees : degrees, seconds };
	}

	static bool runSwing(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdSwing& swing = std::get<CmdSwing>(command);
		Object* const self = doer.self;
		if (!self || !doer.action) { return true; }
		self->swingFrames = std::max(1, executor.game.framesFor(swing.seconds));
		self->swingBy = swing.by;
		self->swingHeld = doer.pressed;
		return true;
	}

	extern const Verb swingVerb;
	const Verb swingVerb{
		.name = "swing",
		.forms = { { "<swing>", places::controls, kindOf<CmdSwing>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makeSwing,
		.run = runSwing,
	};
}
