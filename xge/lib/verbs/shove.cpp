// shove.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <shove>: Pushes the piece in its way one cell on.
	static Command makeShove(const RawCommand&, const ValueEvaluator&)
	{
		return CmdShove{};
	}

	static bool runShove(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		return doer.self && doer.touch && doer.other && executor.game.shoveCells(*doer.other, doer.touch->edge);
	}

	extern const Verb shoveVerb;
	const Verb shoveVerb{
		.name = "shove",
		.forms = { { "<shove />", places::touch, kindOf<CmdShove>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeShove,
		.check = nullptr,
		.run = runShove,
	};
}
