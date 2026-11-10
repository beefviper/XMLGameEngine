// bounce.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

#include <stdexcept>

namespace xge
{
	// <bounce>: Off what it touched, or off the window's edge, keeping a share
	// of its speed.
	static Command makeBounce(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float share = valueOr(raw.amount, 1.0f, evaluate);
		if (!(share >= 0.0f))
		{
			throw std::runtime_error("<bounce> keeps a share of " + formatDisplayNumber(share) + " of its speed; expected 0 or more");
		}
		return CmdBounce{ share };
	}

	static bool runBounce(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdBounce& bounce = std::get<CmdBounce>(command);
		if (doer.self && doer.touch && doer.other) { xge::bounce(*doer.self, *doer.other, *doer.touch, bounce.share); }
		else if (doer.self && doer.screenEdge) { executor.bounceScreenEdge(*doer.self, *doer.screenEdge, bounce.share); }
		return true;
	}

	extern const Verb bounceVerb;
	const Verb bounceVerb{
		.name = "bounce",
		.forms = { { "<bounce />", places::edge | places::touch, kindOf<CmdBounce>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeBounce,
		.run = runBounce,
	};
}
