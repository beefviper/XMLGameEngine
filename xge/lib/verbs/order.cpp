// order.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <order>: Puts what a holder holds in order.
	static Command makeOrder(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdOrder{ raw.object };
	}

	static bool runOrder(const Command& command, const Doer&, CommandExecutor& executor)
	{
		return executor.game.orderHeld(std::get<CmdOrder>(command).group);
	}

	extern const Verb orderVerb;
	const Verb orderVerb{
		.name = "order",
		.forms = { { "<order>", places::changes, kindOf<CmdOrder>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeOrder,
		.check = nullptr,
		.run = runOrder,
	};
}
