// move.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

#include <stdexcept>

namespace xge
{
	// <move>: A step: held in an action, one step anywhere else.
	static Command makeMove(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		CmdMove move{ directionFromName(raw.direction, raw.verb), evaluate(raw.amount) };
		if (isWritten(raw.seconds))
		{
			move.wait = evaluate(raw.seconds);
			if (move.wait < 0.0f) { throw std::runtime_error("<move> has a <wait> of " + formatDisplayNumber(move.wait) + "; expected 0 or more"); }
		}
		return move;
	}

	static bool runMove(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		// In an action a move is held, like a key; anywhere else it is one step,
		// the whole block in lockstep with it.
		const CmdMove& move = std::get<CmdMove>(command);
		if (!doer.self) { return true; }
		if (doer.action && doer.self->onCells) { executor.game.holdCellMove(*doer.self, move, doer.pressed); }
		else if (doer.action) { holdMove(*doer.self, move.direction, move.step, doer.pressed); }
		else { executor.moveByStep(*doer.self, move.direction, move.step); }
		return true;
	}

	extern const Verb moveVerb;
	const Verb moveVerb{
		.name = "move",
		.forms = { { "<move>", places::self, kindOf<CmdMove>() } },
		.formOf = nullptr,
		.onRelease = true,
		.make = makeMove,
		.run = runMove,
	};
}
