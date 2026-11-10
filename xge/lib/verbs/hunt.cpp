// hunt.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/maze.h"

#include <stdexcept>

namespace xge
{
	// <hunt>: After another object through a maze, or towards it hop by hop.
	static Command makeHunt(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (raw.byHops)
		{
			const bool diagonal = !raw.positionX.text.empty() || raw.positionX.kind != RawValue::Kind::Expression;
			const float distance = diagonal ? 0.0f : evaluate(raw.amount);
			const float seconds = hopSeconds(raw, evaluate);
			return CmdHuntHop{ raw.object, diagonal ? evaluate(raw.positionX) : distance, diagonal ? evaluate(raw.positionY) : distance, diagonal, seconds, arcOf(raw, "hop", seconds, evaluate) };
		}

		CmdHunt hunt{ raw.object, speedOf(raw, "hunt", evaluate), 0.0f, raw.partner, raw.retreat, 0.0f };
		if (isWritten(raw.ahead)) { hunt.ahead = evaluate(raw.ahead); }
		if (!raw.retreat.empty())
		{
			hunt.retreatWithin = evaluate(raw.distance);
			if (hunt.retreatWithin < 0.0f) { throw std::runtime_error("<hunt object=\"" + raw.object + "\">: the <retreat> is below 0 tiles"); }
		}
		return hunt;
	}

	static bool runHunt(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { return true; }
		if (const auto* byHops = std::get_if<CmdHuntHop>(&command)) { return executor.huntByHop(*doer.self, *byHops); }
		startHunt(*doer.self, std::get<CmdHunt>(command));
		return true;
	}

	extern const Verb huntVerb;
	const Verb huntVerb{
		.name = "hunt",
		.forms = { { "<hunt>", places::steering, kindOf<CmdHunt>() }, { "<hunt> by <hop>", places::steering, kindOf<CmdHuntHop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeHunt,
		.run = runHunt,
	};
}
