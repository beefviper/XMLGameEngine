// jump.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <jump>: Through the air to a place, touching nothing until it lands.
	static Command makeJump(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		CmdJump jump{ directionFromName(raw.direction, raw.verb), evaluate(raw.amount) };
		if (isWritten(raw.seconds)) { jump.seconds = evaluate(raw.seconds); }
		if (!(jump.seconds > 0.0f)) { throw std::runtime_error("<jump> has <seconds> of " + std::to_string(jump.seconds) + "; expected more than 0"); }
		jump.arc = arcOf(raw, "jump", jump.seconds, evaluate);
		return jump;
	}

	static bool runJump(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !(doer.self && doer.action) || executor.startJump(*doer.self, std::get<CmdJump>(command));
	}

	extern const Verb jumpVerb;
	const Verb jumpVerb{
		.name = "jump",
		.forms = { { "<jump>", places::controls, kindOf<CmdJump>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeJump,
		.run = runJump,
	};
}
