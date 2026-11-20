// undo.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <undo>: Takes back the last move on the cells.
	static Command makeUndo(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdUndo{};
	}

	static bool runUndo(const CmdUndo&, const Doer& doer)
	{
		return doer.game().undo();
	}

	static void printUndo(std::ostream& o, const CmdUndo&)
	{
		o << "undo";
	}

	extern const Verb undoVerb;
	const Verb undoVerb{
		.name = "undo",
		.forms = { { "<undo />", places::flow, kindOf<CmdUndo>() } },
		.make = makeUndo,
		.run = runAs<runUndo>,
		.print = printAs<printUndo>,
	};
}
