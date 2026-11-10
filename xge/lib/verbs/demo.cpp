// demo.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <demo>: Plays the recorded game.
	static Command makeDemo(const RawCommand&, const ValueEvaluator&)
	{
		return CmdDemo{};
	}

	static bool runDemo(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { executor.game.requestDemo(); }
		return true;
	}

	extern const Verb demoVerb;
	const Verb demoVerb{
		.name = "demo",
		.forms = { { "<demo />", places::flow, kindOf<CmdDemo>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeDemo,
		.run = runDemo,
	};
}
