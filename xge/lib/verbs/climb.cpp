// climb.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <stdexcept>
#include <variant>

namespace xge
{
	// <climb>: Up or down a ladder while held.
	static void readClimb(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		requireAttribute(node, "class", where);
		command.own = RawClimb{ readValue(node, where) };
	}

	static Command makeClimb(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const Direction direction = directionFromName(raw.direction, raw.verb);
		if (direction != Direction::Up && direction != Direction::Down)
		{
			throw std::runtime_error("<climb> has direction=\"" + raw.direction + "\"; expected up or down");
		}
		return CmdClimb{ direction, evaluate(ownOf<RawClimb>(raw).speed), raw.objClass };
	}

	static bool runClimb(const CmdClimb& climb, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		Object* const self = doer.self;
		if (!self || !doer.action) { return true; }
		Object::Climb& asked = executor.game.parts().give(executor.game.parts().climbs, self->id);
		asked.activeClimb[climb.direction == Direction::Up ? 0 : 1] = doer.pressed ? climb.step : 0.0f;
		asked.climbClass = climb.ladderClass;
		asked.climbOnce = doer.pressed && doer.byRule;
		return true;
	}

	static bool keepClimbing(const CmdClimb& climb, const Doer& doer)
	{
		if (!doer.self) { return false; }
		Object::Climb& asked = doer.game().parts().give(doer.game().parts().climbs, doer.self->id);
		asked.activeClimb[climb.direction == Direction::Up ? 0 : 1] = climb.step;
		asked.climbClass = climb.ladderClass;
		return true;
	}

	static void printClimb(std::ostream& o, const CmdClimb& climb)
	{
		o << "climb." << (climb.direction == Direction::Up ? "up" : "down") << "(" << climb.step << ", " << climb.ladderClass << ")";
	}

	extern const Verb climbVerb;
	const Verb climbVerb{
		.name = "climb",
		.forms = { { "<climb>", places::controls, kindOf<CmdClimb>() } },
		.onRelease = true,
		.read = readClimb,
		.make = makeClimb,
		.run = runAs<runClimb>,
		.hold = runAs<keepClimbing>,
		.print = printAs<printClimb>,
	};
}
