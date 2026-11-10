// deflect.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

#include <stdexcept>

namespace xge
{
	// <deflect>: Off a paddle, at an angle by where along it the touch was.
	static Command makeDeflect(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float maxAngle = evaluate(raw.amount);
		if (!(maxAngle >= 0.0f && maxAngle < 90.0f))
		{
			throw std::runtime_error("<deflect> has an angle of " + formatDisplayNumber(maxAngle) + "; expected 0 up to (not including) 90 degrees");
		}
		return CmdDeflect{ maxAngle };
	}

	static bool runDeflect(const Command& command, const Doer& doer, CommandExecutor&)
	{
		const CmdDeflect& deflect = std::get<CmdDeflect>(command);
		if (doer.self && doer.touch) { xge::deflect(*doer.self, *doer.other, doer.touch->edge, deflect.maxAngle); }
		return true;
	}

	extern const Verb deflectVerb;
	const Verb deflectVerb{
		.name = "deflect",
		.forms = { { "<deflect>", places::touch, kindOf<CmdDeflect>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeDeflect,
		.run = runDeflect,
	};
}
