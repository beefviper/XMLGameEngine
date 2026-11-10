// hole.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

#include <stdexcept>

namespace xge
{
	// <hole>: Cuts a hole in its picture where it was touched.
	static Command makeHole(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float radius = evaluate(raw.amount);
		const float rough = valueOr(raw.distance, 0.0f, evaluate);
		if (!(radius > 0.0f)) { throw std::runtime_error("<hole> has a radius of " + formatDisplayNumber(radius) + "; expected above 0"); }
		if (!(rough >= 0.0f && rough <= 1.0f)) { throw std::runtime_error("<hole> has a <rough> of " + formatDisplayNumber(rough) + "; expected 0 up to 1"); }
		return CmdHole{ radius, rough };
	}

	static bool runHole(const Command& command, const Doer& doer, CommandExecutor&)
	{
		const CmdHole& hole = std::get<CmdHole>(command);
		if (doer.self && doer.touch) { cutHole(*doer.self, *doer.other, hole.radius, hole.rough); }
		return true;
	}

	extern const Verb holeVerb;
	const Verb holeVerb{
		.name = "hole",
		.forms = { { "<hole>", places::touch, kindOf<CmdHole>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeHole,
		.run = runHole,
	};
}
