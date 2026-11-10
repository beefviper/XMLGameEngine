// knock.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <stdexcept>

namespace xge
{
	// <knock>: Two things with a mass trade their speed.
	static Command makeKnock(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float bounce = valueOr(raw.amount, 1.0f, evaluate);
		if (!(bounce >= 0.0f && bounce <= 1.0f))
		{
			throw std::runtime_error("<knock> has a bounce of " + formatDisplayNumber(bounce) + "; expected 0 up to 1");
		}
		return CmdKnock{ bounce };
	}

	static bool runKnock(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdKnock& knock = std::get<CmdKnock>(command);
		if (doer.self && doer.touch) { executor.knock(*doer.self, *doer.other, *doer.touch, knock.bounce); }
		return true;
	}

	extern const Verb knockVerb;
	const Verb knockVerb{
		.name = "knock",
		.forms = { { "<knock>", places::touch, kindOf<CmdKnock>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeKnock,
		.check = nullptr,
		.run = runKnock,
	};
}
