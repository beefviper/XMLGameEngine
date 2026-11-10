// stick.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

namespace xge
{
	// <stick>: Stops at the window's edge, or against what it touched.
	static Command makeStick(const RawCommand&, const ValueEvaluator&)
	{
		return CmdStick{};
	}

	static bool runStick(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && doer.screenEdge) { xge::stick(*doer.self, executor.game.getWindowDesc(), *doer.screenEdge); }
		else if (doer.self && doer.touch) { stickAgainst(*doer.self, doer.touch->edge); }
		return true;
	}

	extern const Verb stickVerb;
	const Verb stickVerb{
		.name = "stick",
		.forms = { { "<stick />", places::edge | places::touch, kindOf<CmdStick>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeStick,
		.run = runStick,
	};
}
