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
	static void readDeflect(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		command.own.emplace<WrittenDeflect>().widest = readValue(node, where);
	}

	static Command makeDeflect(const WrittenCommand& raw, const ValueEvaluator& evaluate)
	{
		const float maxAngle = evaluate(ownOf<WrittenDeflect>(raw).widest);
		if (!(maxAngle >= 0.0f && maxAngle < 90.0f))
		{
			throw std::runtime_error("<deflect> has an angle of " + formatDisplayNumber(maxAngle) + "; expected 0 up to (not including) 90 degrees");
		}
		return CmdDeflect{ maxAngle };
	}

	// <deflect>maxDegrees</deflect>: back off `other` at an angle set by
	// where on the touched side it hit.
	//
	// How far the object's middle is from the middle of the touched side, as
	// a share of half that side's length (-1 at one end, 1 at the other, held
	// there past the ends), times maxDegrees, is the angle away from straight
	// back out, towards the end it hit nearer to: off a Pong paddle's top
	// half the ball goes back up, off its bottom half down.
	static void deflect(Object& object, const Object& other, Edge edge, float maxDegrees)
	{
		object.velocity = deflected(object.velocity, { object.position, sizeOf(object) }, { other.position, sizeOf(other) }, edge, maxDegrees);
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
