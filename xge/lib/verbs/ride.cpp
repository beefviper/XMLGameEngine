// ride.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

namespace xge
{
	// <ride>: Is carried along by what it stands on.
	static Command makeRide(const RawCommand&, const ValueEvaluator&)
	{
		return CmdRide{};
	}

	static bool runRide(const Command&, const Doer& doer, CommandExecutor&)
	{
		if (doer.self && doer.touch) { xge::ride(*doer.self, *doer.other); }
		return true;
	}

	static void printRide(std::ostream& o, const Command&)
	{
		o << "ride";
	}

	extern const Verb rideVerb;
	const Verb rideVerb{
		.name = "ride",
		.forms = { { "<ride />", places::touch, kindOf<CmdRide>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeRide,
		.check = nullptr,
		.run = runRide,
		.print = printRide,
	};
}
