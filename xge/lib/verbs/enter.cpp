// enter.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"

namespace xge
{
	// <enter>: Goes to another scene of the state.
	static Command makeEnter(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdEnterScene{ raw.scene, raw.way };
	}

	static void checkEnter(const Command& command, const Checking& checking)
	{
		const CmdEnterScene& enter = std::get<CmdEnterScene>(command);
		checkScene(checking, "enter", enter.scene, enter.way);
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
		.check = checkEnter,
		.run = runEnter,
	};
}
