// become.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <become>: Shows another of its looks, or wears another style's colours.
	static Command makeBecome(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdBecome{ raw.sprite, raw.object, raw.from, raw.style };
	}

	static bool runBecome(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		executor.become(doer.self, std::get<CmdBecome>(command));
		return true;
	}

	extern const Verb becomeVerb;
	const Verb becomeVerb{
		.name = "become",
		.forms = { { "<become>", places::changes, kindOf<CmdBecome>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeBecome,
		.run = runBecome,
	};
}
