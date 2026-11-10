// play.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <play>: A sound; one that loops stops when its key is let go.
	static Command makePlay(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPlay{ raw.sound };
	}

	static bool runPlay(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		executor.playHeld(std::get<CmdPlay>(command).sound, doer.pressed);
		return true;
	}

	extern const Verb playVerb;
	const Verb playVerb{
		.name = "play",
		.forms = { { "<play>", places::changes, kindOf<CmdPlay>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makePlay,
		.run = runPlay,
	};
}
