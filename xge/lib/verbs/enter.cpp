// enter.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <enter>: Goes to another scene of the state.
	static Command makeEnter(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdEnterScene{ raw.scene, raw.way };
	}

	static bool runEnter(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.enter(std::get<CmdEnterScene>(command));
		return true;
	}

	extern const Verb enterVerb;
	const Verb enterVerb{
		.name = "enter",
		.forms = { { "<enter>", places::changes, kindOf<CmdEnterScene>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeEnter,
		.run = runEnter,
	};
}
