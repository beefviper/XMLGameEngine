// demo.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <demo>: Plays the recorded game.
	static Command makeDemo(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdDemo{};
	}

	static bool runDemo(const CmdDemo&, const Doer& doer)
	{
		if (!doer.self) { doer.game().requestDemo(); }
		return true;
	}

	static void printDemo(std::ostream& o, const CmdDemo&)
	{
		o << "demo()";
	}

	extern const Verb demoVerb;
	const Verb demoVerb{
		.name = "demo",
		.forms = { { "<demo />", places::flow, kindOf<CmdDemo>() } },
		.make = makeDemo,
		.run = runAs<runDemo>,
		.print = printAs<printDemo>,
	};
}
