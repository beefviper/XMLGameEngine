// die.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/verbs.h"

namespace xge
{
	// <die>: Out of play until something puts it back.
	static Command makeDie(const RawCommand&, const ValueEvaluator&)
	{
		return CmdDie{};
	}

	// <die />: out of play; not drawn, moved or collided with.
	//
	// Everything else about it is left as it was, so whatever brings it back
	// (fire() re-launching a bullet, reset()) starts from a known place.
	static void die(Object& object)
	{
		object.collisionsOn = false;
		object.isVisible = false;
		object.path.name.clear();
	}

	static bool runDie(const CmdDie&, const Doer& doer)
	{
		if (doer.self) { xge::die(*doer.self); }
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
