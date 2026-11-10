// pick.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <pick>: Takes up the piece, or the cards, under it.
	static Command makePick(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPick{ raw.objClass, raw.only };
	}

	static bool runPick(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdPick& pick = std::get<CmdPick>(command);
		Object* const self = doer.self;
		return self && (self->hand.is ? executor.game.pickCards(*self) : executor.game.pickPiece(*self, pick.objClass, pick.only));
	}

	extern const Verb pickVerb;
	const Verb pickVerb{
		.name = "pick",
		.forms = { { "<pick>", places::self, kindOf<CmdPick>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makePick,
		.check = nullptr,
		.run = runPick,
	};
}
