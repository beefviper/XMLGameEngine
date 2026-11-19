// move.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <stdexcept>

namespace xge
{
	// <move>: A step: held in an action, one step anywhere else.
	static void readMove(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Its step, then on a cell map how long it waits before it repeats.
		requireAttribute(node, "direction", where);
		command.amount = readLeadValue(node, { "wait" }, "its step, then <wait> if wanted", where);
		if (auto wait = findChild(&node, "wait")) { command.seconds = readValue(*wait, where + " > <move>"); }
	}

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

	// One step, the whole block in lockstep with it.
	static void stepBy(Game& game, Object& object, Direction direction, float step)
	{
		const auto apply = [&](Object& target)
		{
			switch (direction)
			{
			case Direction::Up:    target.position.y -= step; break;
			case Direction::Down:  target.position.y += step; break;
			case Direction::Left:  target.position.x -= step; break;
			case Direction::Right: target.position.x += step; break;
			}
		};

		if (object.kind->collisionData.lockstep > 0)
		{
			for (auto& obj : game.getCurrentObjects())
			{
				if (obj.kind->collisionData.lockstep == object.kind->collisionData.lockstep)
				{
					apply(obj);
				}
			}
		}
		else
		{
			apply(object);
		}
	}

	static bool runMove(const CmdMove& move, const Doer& doer)
	{
		// In an action a move is held, like a key; anywhere else it is one step.
		if (!doer.self) { return true; }
		Game& game = doer.game();
		if (doer.action && doer.self->onCells) { game.holdCellMove(*doer.self, move, doer.pressed); }
		else if (doer.action) { holdMove(*doer.self, game.driveFor(*doer.self), move.direction, move.step, doer.pressed); }
		else { stepBy(game, *doer.self, move.direction, move.step); }
		return true;
	}

	static void printMove(std::ostream& o, const CmdMove& move)
	{
		const char* direction = (move.direction == Direction::Up) ? "up"
			: (move.direction == Direction::Down) ? "down"
			: (move.direction == Direction::Left) ? "left"
			: "right";
		o << "move." << direction << "(" << move.step << (move.wait > 0.0f ? ", wait " + formatDisplayNumber(move.wait) : std::string{}) << ")";
	}

	extern const Verb moveVerb;
	const Verb moveVerb{
		.name = "move",
		.forms = { { "<move>", places::self, kindOf<CmdMove>() } },
		.onRelease = true,
		.read = readMove,
		.make = makeMove,
		.run = runAs<runMove>,
		.print = printAs<printMove>,
	};
}
