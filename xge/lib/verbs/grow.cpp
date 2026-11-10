// grow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

namespace xge
{
	// <grow>: A chain gets one link longer.
	static Command makeGrow(const RawCommand&, const ValueEvaluator&)
	{
		return CmdGrow{};
	}

	static bool runGrow(const Command&, const Doer& doer, CommandExecutor&)
	{
		if (doer.self && !doer.self->chain.empty()) { ++doer.self->chainGrow; }
		return true;
	}

	extern const Verb growVerb;
	const Verb growVerb{
		.name = "grow",
		.forms = { { "<grow />", places::self, kindOf<CmdGrow>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeGrow,
		.run = runGrow,
	};
}
