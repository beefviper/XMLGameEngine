// stop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

namespace xge
{
	// <stop>: Stands still.
	static Command makeStop(const RawCommand&, const ValueEvaluator&)
	{
		return CmdStop{};
	}

	static bool runStop(const Command&, const Doer& doer, CommandExecutor&)
	{
		if (doer.self) { xge::stop(*doer.self); }
		return true;
	}

	extern const Verb stopVerb;
	const Verb stopVerb{
		.name = "stop",
		.forms = { { "<stop />", places::self, kindOf<CmdStop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeStop,
		.check = nullptr,
		.run = runStop,
	};
}
