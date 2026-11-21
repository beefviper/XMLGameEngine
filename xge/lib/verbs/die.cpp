// die.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <die>: Out of play until something puts it back.
	static Command makeDie(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdDie{};
	}

	// <die />: out of play; not drawn, moved or collided with.
	//
	// Everything else about it is left as it was, so whatever brings it back
	// (fire() re-launching a bullet, reset()) starts from a known place.
	static void die(Object& object, Parts& parts)
	{
		object.collisionsOn = false;
		object.isVisible = false;
		parts.onPaths.remove(object.id);
	}

	static bool runDie(const CmdDie&, const Doer& doer)
	{
		if (doer.self) { xge::die(*doer.self, doer.game().parts()); }
		return true;
	}

	static void printDie(std::ostream& o, const CmdDie&)
	{
		o << "die";
	}

	extern const Verb dieVerb;
	const Verb dieVerb{
		.name = "die",
		.forms = { { "<die />", places::self, kindOf<CmdDie>() } },
		.make = makeDie,
		.run = runAs<runDie>,
		.print = printAs<printDie>,
	};
}
