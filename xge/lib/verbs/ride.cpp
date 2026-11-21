// ride.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

namespace xge
{
	// <ride>: Is carried along by what it stands on.
	static Command makeRide(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdRide{};
	}

	// <ride />: moves with `other` for this frame.
	//
	// Not kept: worked out again every frame from whatever is still being
	// touched, so the moment it is off the other one, it is at rest.
	static void ride(Object& object, const Object& other)
	{
		object.riding = other.velocity;
	}

	static bool runRide(const CmdRide&, const Doer& doer)
	{
		if (doer.self && doer.touch) { xge::ride(*doer.self, *doer.other); }
		return true;
	}

	static void printRide(std::ostream& o, const CmdRide&)
	{
		o << "ride";
	}

	extern const Verb rideVerb;
	const Verb rideVerb{
		.name = "ride",
		.forms = { { "<ride />", places::touch, kindOf<CmdRide>() } },
		.make = makeRide,
		.run = runAs<runRide>,
		.print = printAs<printRide>,
	};
}
