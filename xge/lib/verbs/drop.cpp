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

	static bool runDrop(const Command&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		Object* const self = doer.self;
		// A picker puts back the piece it holds; refused in the middle of a run
		// of jumps.
		if (self && executor.game.isHand(*self)) { return executor.game.dropItems(*self); }
		if (self && !executor.game.pickerOf(*self).picked.empty()) { return executor.game.dropPiece(*self); }
		if (self) { if (Object::Carry* carry = executor.game.parts().carries.find(self->id)) { xge::drop(*carry); } }
		return true;
	}

	static void printDrop(std::ostream& o, const Command&)
	{
		o << "drop";
	}

	extern const Verb dropVerb;
	const Verb dropVerb{
		.name = "drop",
		.forms = { { "<drop />", places::self, kindOf<CmdDrop>() } },
		.make = makeDrop,
		.run = runDrop,
		.print = printDrop,
	};
}
