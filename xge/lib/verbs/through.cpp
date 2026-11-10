// through.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <through>: Makes the refused cell step anyway: the two share the cell.
	static Command makeThrough(const RawCommand&, const ValueEvaluator&)
	{
		return CmdThrough{};
	}

	static bool runThrough(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && doer.touch) { executor.game.stepThrough(); }
		return true;
	}

	extern const Verb throughVerb;
	const Verb throughVerb{
		.name = "through",
		.forms = { { "<through />", places::touch, kindOf<CmdThrough>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeThrough,
		.check = nullptr,
		.run = runThrough,
	};
}
