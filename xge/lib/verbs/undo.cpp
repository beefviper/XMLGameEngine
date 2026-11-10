// undo.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <undo>: Takes back the last move on the cells.
	static Command makeUndo(const RawCommand&, const ValueEvaluator&)
	{
		return CmdUndo{};
	}

	static bool runUndo(const Command&, const Doer&, CommandExecutor& executor)
	{
		return executor.game.undo();
	}

	extern const Verb undoVerb;
	const Verb undoVerb{
		.name = "undo",
		.forms = { { "<undo />", places::flow, kindOf<CmdUndo>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeUndo,
		.run = runUndo,
	};
}
