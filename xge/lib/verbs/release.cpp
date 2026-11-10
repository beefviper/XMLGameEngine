// release.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <cmath>
#include <stdexcept>

namespace xge
{
	// <release>: Lets some of another object go from where it is.
	static Command makeRelease(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const int count = static_cast<int>(std::lround(evaluate(raw.amount)));
		if (count < 1) { throw std::runtime_error("<release> of " + raw.object + " has a count under 1"); }
		return CmdRelease{ raw.object, count };
	}

	static bool runRelease(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdRelease& release = std::get<CmdRelease>(command);
		if (doer.self) { executor.release(*doer.self, release.target, release.count); }
		return true;
	}

	extern const Verb releaseVerb;
	const Verb releaseVerb{
		.name = "release",
		.forms = { { "<release>", places::self, kindOf<CmdRelease>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeRelease,
		.run = runRelease,
	};
}
