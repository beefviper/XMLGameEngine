// track.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

namespace xge
{
	// <track>: Follows a control that points (a mouse, a stick): run by
	// CommandExecutor::executeAxis.
	static Command makeTrack(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdTrack{ raw.path, raw.onto };
	}

	extern const Verb trackVerb;
	const Verb trackVerb{
		.name = "track",
		.forms = { { "<track />", places::controls, kindOf<CmdTrack>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeTrack,
		.run = nullptr,
	};
}
