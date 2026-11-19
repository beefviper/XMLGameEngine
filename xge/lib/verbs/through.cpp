// through.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <through>: Makes the refused cell step anyway: the two share the cell.
	static Command makeThrough(const RawCommand&, const ValueEvaluator&)
	{
		return CmdThrough{};
	}

	static bool runThrough(const CmdThrough&, const Doer& doer)
	{
		if (doer.self && doer.touch) { doer.game().stepThrough(); }
		return true;
	}

	static void printThrough(std::ostream& o, const CmdThrough&)
	{
		o << "through";
	}

	extern const Verb throughVerb;
	const Verb throughVerb{
		.name = "through",
		.forms = { { "<through />", places::touch, kindOf<CmdThrough>() } },
		.make = makeThrough,
		.run = runAs<runThrough>,
		.print = printAs<printThrough>,
	};
}
