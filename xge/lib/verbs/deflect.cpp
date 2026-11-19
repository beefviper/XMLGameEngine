// deflect.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <stdexcept>

namespace xge
{
	// <deflect>: Off a paddle, at an angle by where along it the touch was.
	static void readDeflect(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.amount = readValue(node, where);
	}

	static Command makeDeflect(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float maxAngle = evaluate(raw.amount);
		if (!(maxAngle >= 0.0f && maxAngle < 90.0f))
		{
			throw std::runtime_error("<deflect> has an angle of " + formatDisplayNumber(maxAngle) + "; expected 0 up to (not including) 90 degrees");
		}
		return CmdDeflect{ maxAngle };
	}

	static bool runDeflect(const CmdDeflect& deflect, const Doer& doer)
	{
		if (doer.self && doer.touch) { xge::deflect(*doer.self, *doer.other, doer.touch->edge, deflect.maxAngle); }
		return true;
	}

	static void printDeflect(std::ostream& o, const CmdDeflect& deflect)
	{
		o << "deflect(" << deflect.maxAngle << ")";
	}

	extern const Verb deflectVerb;
	const Verb deflectVerb{
		.name = "deflect",
		.forms = { { "<deflect>", places::touch, kindOf<CmdDeflect>() } },
		.read = readDeflect,
		.make = makeDeflect,
		.run = runAs<runDeflect>,
		.print = printAs<printDeflect>,
	};
}
