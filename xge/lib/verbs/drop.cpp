// drop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

namespace xge
{
	// <drop>: Lets go of what it carries, or puts back what it picked.
	static Command makeDrop(const RawCommand&, const ValueEvaluator&)
	{
		return CmdDrop{};
	}

	static bool runDrop(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		Object* const self = doer.self;
		// A picker puts back the piece it holds; refused in the middle of a run
		// of jumps.
		if (self && self->hand.is) { return executor.game.dropCards(*self); }
		if (self && !self->picked.empty()) { return executor.game.dropPiece(*self); }
		if (self) { xge::drop(*self); }
		return true;
	}

	extern const Verb dropVerb;
	const Verb dropVerb{
		.name = "drop",
		.forms = { { "<drop />", places::self, kindOf<CmdDrop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeDrop,
		.check = nullptr,
		.run = runDrop,
	};
}
