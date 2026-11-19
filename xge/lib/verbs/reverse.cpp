// reverse.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <reverse>: Goes back the way it came.
	static Command makeReverse(const RawCommand&, const ValueEvaluator&)
	{
		return CmdReverse{};
	}

	static bool runReverse(const CmdReverse&, const Doer& doer)
	{
		Object* const self = doer.self;
		if (!self) { return true; }
		Game& game = doer.game();
		// On a map of cells, a <go> turns round.
		if (self->onCells && game.place(*self).going)
		{
			Object::Cells::Place& going = game.placeFor(*self);
			const Direction way = going.goWay;
			going.goWay = way == Direction::Up ? Direction::Down : way == Direction::Down ? Direction::Up : way == Direction::Left ? Direction::Right : Direction::Left;
			self->facing = going.goWay;
		}
		else { self->velocity = self->velocity * -1.0f; }
		return true;
	}

	static void printReverse(std::ostream& o, const CmdReverse&)
	{
		o << "reverse";
	}

	extern const Verb reverseVerb;
	const Verb reverseVerb{
		.name = "reverse",
		.forms = { { "<reverse />", places::self, kindOf<CmdReverse>() } },
		.make = makeReverse,
		.run = runAs<runReverse>,
		.print = printAs<printReverse>,
	};
}
