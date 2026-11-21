// thrust.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

namespace xge
{
	// <thrust>: A push while held, the way it faces, or with direction= one
	// way across the screen whatever its heading.
	static void readThrust(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		command.own.emplace<WrittenThrust>().amount = readValue(node, where);
	}

	static Command makeThrust(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		const float amount = evaluate(ownOf<WrittenThrust>(written).amount);
		if (!written.direction.empty()) { return CmdThrustAcross{ directionFromName(written.direction, written.verb), amount, written.burn }; }
		return CmdThrust{ amount, written.burn };
	}

	// Held by `amount` (0: let go), burning `burn`; the frame adds every
	// direction held, each its own amount, with nothing to recombine.
	static void pushAcross(Game& game, Object& object, Direction direction, float amount, Name burn)
	{
		const auto index = static_cast<std::size_t>(direction);
		Drive& drive = game.driveFor(object);
		drive.thrust[index] = amount;
		drive.thrustBurn[index] = amount == 0.0f ? Name{} : burn;
	}

	// Thrust along the heading is now held (amount 0 = just
	// released) by that action, and the variable it burns. Each action's
	// thrust is its own, so two held at once add up.
	static void pushAhead(Game& game, Object& object, Name action, float amount, Name burn)
	{
		if (amount == 0.0f)
		{
			if (Drive* drive = game.parts().drives.find(object.id)) { drive->thrustAhead.erase(action); }
			return;
		}
		game.driveFor(object).thrustAhead[action] = { amount, burn };
	}

	// Held: the push goes on; let go, it stops.
	static bool runThrust(const Command& command, const Doer& doer)
	{
		if (!doer.self || !doer.action) { return true; }
		Game& game = doer.game();
		if (const auto* across = std::get_if<CmdThrustAcross>(&command))
		{
			pushAcross(game, *doer.self, across->direction, doer.pressed ? across->amount : 0.0f, across->burn);
			return true;
		}
		const CmdThrust& ahead = std::get<CmdThrust>(command);
		pushAhead(game, *doer.self, *doer.action, doer.pressed ? ahead.amount : 0.0f, ahead.burn);
		return true;
	}

	static bool keepPushing(const Command& command, const Doer& doer)
	{
		return doer.self && doer.action && runThrust(command, doer);
	}

	static void printThrust(std::ostream& o, const Command& command)
	{
		if (const auto* pt = std::get_if<CmdThrust>(&command))
		{
			const CmdThrust& t = *pt;
			o << "thrust(" << t.amount << (t.burn.empty() ? "" : ", burn " + t.burn) << ")";
			return;
		}
		if (const auto* pa = std::get_if<CmdThrustAcross>(&command))
		{
			const CmdThrustAcross& a = *pa;
			const char* direction = (a.direction == Direction::Up) ? "up"
				: (a.direction == Direction::Down) ? "down"
				: (a.direction == Direction::Left) ? "left"
				: "right";
			o << "thrust." << direction << "(" << a.amount << (a.burn.empty() ? "" : ", burn " + a.burn) << ")";
			return;
		}
	}

	extern const Verb thrustVerb;
	const Verb thrustVerb{
		.name = "thrust",
		.forms = { { "<thrust>", places::controls, kindOf<CmdThrust>() }, { "<thrust direction>", places::controls, kindOf<CmdThrustAcross>() } },
		.onRelease = true,
		.read = readThrust,
		.make = makeThrust,
		.run = runThrust,
		.hold = keepPushing,
		.print = printThrust,
	};
}
