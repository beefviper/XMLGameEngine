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

	static bool runStick(const CmdStick&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self && doer.screenEdge) { xge::stick(*doer.self, executor.game.getWindowDesc(), *doer.screenEdge); }
		else if (doer.self && doer.touch) { stickAgainst(*doer.self, doer.touch->edge); }
		return true;
	}

	static void printStick(std::ostream& o, const CmdStick&)
	{
		o << "stick";
	}

	extern const Verb stickVerb;
	const Verb stickVerb{
		.name = "stick",
		.forms = { { "<stick />", places::edge | places::touch, kindOf<CmdStick>() } },
		.make = makeStick,
		.run = runAs<runStick>,
		.print = printAs<printStick>,
	};
}
