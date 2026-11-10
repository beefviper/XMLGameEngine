// wander.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/maze.h"

namespace xge
{
	// <wander>: Through a maze, any way at each corner.
	static Command makeWander(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdWander{ speedOf(raw, "wander", evaluate) };
	}

	static bool runWander(const Command& command, const Doer& doer, CommandExecutor&)
	{
		if (doer.self) { startWander(*doer.self, std::get<CmdWander>(command)); }
		return true;
	}

	extern const Verb wanderVerb;
	const Verb wanderVerb{
		.name = "wander",
		.forms = { { "<wander>", places::steering, kindOf<CmdWander>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeWander,
		.run = runWander,
	};
}
