// drop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <drop>: Lets go of what it carries, or puts back what it picked.
	static Command makeDrop(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdDrop{};
	}

	// <drop />: puts down what it carries.
	static void drop(Object::Carry& carry)
	{
		if (carry.what.empty())
		{
			return;
		}

		carry.dropped = carry.what;
		carry.what.clear();
	}

	static bool runDrop(const CmdDrop&, const Doer& doer)
	{
		Game& game = doer.game();
		Object* const self = doer.self;
		// A picker puts back the piece it holds; refused in the middle of a run
		// of jumps.
		if (self && game.isHand(*self)) { return game.dropItems(*self); }
		if (self && !game.pickerOf(*self).picked.empty()) { return game.cellMaps().dropPiece(*self); }
		if (self) { if (Object::Carry* carry = game.parts().carries.find(self->id)) { xge::drop(*carry); } }
		return true;
	}

	static void printDrop(std::ostream& o, const CmdDrop&)
	{
		o << "drop";
	}

	extern const Verb dropVerb;
	const Verb dropVerb{
		.name = "drop",
		.forms = { { "<drop />", places::self, kindOf<CmdDrop>() } },
		.make = makeDrop,
		.run = runAs<runDrop>,
		.print = printAs<printDrop>,
	};
}
