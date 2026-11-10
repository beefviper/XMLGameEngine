// push.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <push>: Goes to another state, on top of this one.
	static Command makePush(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPushState{ raw.state };
	}

	static bool runPush(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.pushState(std::get<CmdPushState>(command).name);
		return true;
	}

	extern const Verb pushVerb;
	const Verb pushVerb{
		.name = "push",
		.forms = { { "<push>", places::flow, kindOf<CmdPushState>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makePush,
		.run = runPush,
	};
}
