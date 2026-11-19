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
		RawMove& move = command.own.emplace<RawMove>();
		move.step = readLeadValue(node, { "wait" }, "its step, then <wait> if wanted", where);
		if (auto wait = findChild(&node, "wait")) { move.wait = readValue(*wait, where + " > <move>"); }
	}

	static Command makeMove(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const RawMove& written = ownOf<RawMove>(raw);
		CmdMove move{ directionFromName(raw.direction, raw.verb), evaluate(written.step) };
		if (written.wait)
		{
			move.wait = evaluate(*written.wait);
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

	// A direction held at `step` (0: let go), recombined with the others held
	// into the velocity: Left/Right and Up/Down are separate axes, and in a
	// <jump> the way across waits for the landing.
	//
	// Each direction's step is kept (Object::Drive::moveStep), so holding Down
	// and tapping Up stops, and letting go of Up moves down again.
	static void holdStep(Object& object, Drive& drive, Direction direction, float step)
	{
		drive.moveStep[static_cast<std::size_t>(direction)] = step;

		// With a <grip> that is not at once the velocity goes there over
		// time (playOneFrame), on the ground; a pull still has the up and down.
		const bool slides = !std::isinf(object.motion.grip);
		if (direction == Direction::Left || direction == Direction::Right)
		{
			if (!object.footing.jumping && !slides)
			{
				object.velocity.x = drive.moveStep[static_cast<std::size_t>(Direction::Right)]
					- drive.moveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
		else if (!slides || object.acceleration.y != 0.0f)
		{
			object.velocity.y = drive.moveStep[static_cast<std::size_t>(Direction::Down)]
				- drive.moveStep[static_cast<std::size_t>(Direction::Up)];
		}
	}

	// A <move> in an action, as its key goes down (pressed) or up: that
	// direction held at `step`, recombined with the others held into the
	// velocity, and the object faces that way: with a key of the other axis
	// held too, the diagonal between them, which its <fire>s follow.
	static void holdMove(Object& object, Drive& drive, Direction direction, float step, bool pressed)
	{
		holdStep(object, drive, direction, pressed ? step : 0.0f);
		const auto held = [&](Direction way) { return drive.moveStep[static_cast<std::size_t>(way)] != 0.0f; };
		const float across = (held(Direction::Right) ? 1.0f : 0.0f) - (held(Direction::Left) ? 1.0f : 0.0f);
		const float upDown = (held(Direction::Down) ? 1.0f : 0.0f) - (held(Direction::Up) ? 1.0f : 0.0f);
		if (pressed && step != 0.0f)
		{
			object.facing = direction;
			object.aim.reset();
		}
		else if (!pressed && (across == 0.0f) != (upDown == 0.0f))
		{
			// One of two let go: the way still held, straight.
			object.facing = across < 0.0f ? Direction::Left : across > 0.0f ? Direction::Right : upDown < 0.0f ? Direction::Up : Direction::Down;
			object.aim.reset();
		}
		// Two held across each other: an object with a <facing> faces the
		// diagonal between them, and its <fire>s leave along it. Let go of
		// both, it keeps facing the way it last went.
		if (object.hasFacing && across != 0.0f && upDown != 0.0f)
		{
			object.aim = Vector2f{ across, upDown } * (1.0f / std::sqrt(2.0f));
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

	static bool keepMoving(const CmdMove& move, const Doer& doer)
	{
		if (!doer.self) { return false; }
		holdStep(*doer.self, doer.game().driveFor(*doer.self), move.direction, move.step);
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
		.hold = runAs<keepMoving>,
		.print = printAs<printMove>,
	};
}
