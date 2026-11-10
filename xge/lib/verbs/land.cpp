// land.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <land>: Comes to rest on what it fell onto.
	static Command makeLand(const RawCommand&, const ValueEvaluator&)
	{
		return CmdLand{};
	}

	static bool runLand(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && doer.touch) { executor.land(*doer.self, *doer.other, doer.touch->edge); }
		return true;
	}

	extern const Verb landVerb;
	const Verb landVerb{
		.name = "land",
		.forms = { { "<land />", places::touch, kindOf<CmdLand>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeLand,
		.check = nullptr,
		.run = runLand,
	};
}
