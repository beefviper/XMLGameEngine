// carry.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <carry>: Picks up what it touched.
	static Command makeCarry(const RawCommand&, const ValueEvaluator&)
	{
		return CmdCarry{};
	}

	// Taken from whatever had it: one thing is carried by one object at a time.
	static void takeUp(Game& game, Object& object, const Object& item)
	{
		Parts& parts = game.parts();
		if (const Object::Carry* mine = parts.carries.find(object.id); mine && (item.name == mine->what || item.name == mine->dropped))
		{
			return;
		}

		for (auto& other : game.getCurrentObjects())
		{
			Object::Carry* theirs = parts.carries.find(other.id);
			if (&other != &object && theirs && theirs->what == item.name)
			{
				theirs->what.clear();
			}
		}

		xge::carry(object, parts.give(parts.carries, object.id), item);
	}

	static bool runCarry(const CmdCarry&, const Doer& doer)
	{
		if (doer.touching()) { takeUp(doer.game(), *doer.self, *doer.other); }
		return true;
	}

	static void printCarry(std::ostream& o, const CmdCarry&)
	{
		o << "carry";
	}

	extern const Verb carryVerb;
	const Verb carryVerb{
		.name = "carry",
		.forms = { { "<carry />", places::touch, kindOf<CmdCarry>() } },
		.make = makeCarry,
		.run = runAs<runCarry>,
		.print = printAs<printCarry>,
	};
}
