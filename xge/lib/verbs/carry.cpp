// carry.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <carry>: Picks up what it touched.
	static Command makeCarry(const RawCommand&, const ValueEvaluator&)
	{
		return CmdCarry{};
	}

	static bool runCarry(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && doer.touch) { executor.carry(*doer.self, *doer.other); }
		return true;
	}

	extern const Verb carryVerb;
	const Verb carryVerb{
		.name = "carry",
		.forms = { { "<carry />", places::touch, kindOf<CmdCarry>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCarry,
		.check = nullptr,
		.run = runCarry,
	};
}
