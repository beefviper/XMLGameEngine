// thrust.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

namespace xge
{
	// <thrust>: A push while held, the way it faces, or with direction= one
	// way across the screen whatever its heading.
	static void readThrust(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.amount = readValue(node, where);
	}

	static Command makeThrust(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (!raw.direction.empty()) { return CmdThrustAcross{ directionFromName(raw.direction, raw.verb), evaluate(raw.amount), raw.burn }; }
		return CmdThrust{ evaluate(raw.amount), raw.burn };
	}

	static bool runThrust(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self || !doer.action) { return true; }
		if (const auto* way = std::get_if<CmdThrustAcross>(&command))
		{
			executor.applyActionThrust(*doer.self, way->direction, doer.pressed ? way->amount : 0.0f, way->burn);
			return true;
		}
		const CmdThrust& thrust = std::get<CmdThrust>(command);
		executor.applyActionThrustAhead(*doer.self, *doer.action, doer.pressed ? thrust.amount : 0.0f, thrust.burn);
		return true;
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

	static void printThrustRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
	}

	extern const Verb thrustVerb;
	const Verb thrustVerb{
		.name = "thrust",
		.forms = { { "<thrust>", places::controls, kindOf<CmdThrust>() }, { "<thrust direction>", places::controls, kindOf<CmdThrustAcross>() } },
		.formOf = nullptr,
		.onRelease = true,
		.read = readThrust,
		.make = makeThrust,
		.check = nullptr,
		.run = runThrust,
		.print = printThrust,
		.printRaw = printThrustRaw,
	};
}
