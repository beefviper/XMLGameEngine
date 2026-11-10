// reverse.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <reverse>: Goes back the way it came.
	static Command makeReverse(const RawCommand&, const ValueEvaluator&)
	{
		return CmdReverse{};
	}

	static bool runReverse(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		Object* const self = doer.self;
		// On a map of cells, a <go> turns round.
		if (self && self->onCells && self->cellGoing)
		{
			const Direction way = self->cellGoWay;
			self->cellGoWay = way == Direction::Up ? Direction::Down : way == Direction::Down ? Direction::Up : way == Direction::Left ? Direction::Right : Direction::Left;
			self->facing = self->cellGoWay;
		}
		else if (self) { executor.reverse(*self); }
		return true;
	}

	extern const Verb reverseVerb;
	const Verb reverseVerb{
		.name = "reverse",
		.forms = { { "<reverse />", places::self, kindOf<CmdReverse>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeReverse,
		.run = runReverse,
	};
}
