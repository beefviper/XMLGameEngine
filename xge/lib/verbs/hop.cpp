// hop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <hop>: At once, or with <seconds> through the air.
	static void readHop(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		if (findChild(&node, "onto"))
		{
			readOnto(node, where + " > <hop>", command);
			if (command.direction.find('-') != std::string::npos) { fail(where, "<hop direction=\"" + command.direction + "\"> with <onto>: only up, down, left or right"); }
		}
		else if (readHopParts(node, where, command) && command.direction.find('-') == std::string::npos)
		{
			fail(where, "<hop direction=\"" + command.direction + "\"> is not diagonal; it gives one distance");
		}
	}

	static Command makeHop(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// up-left and the rest: one of up/down, then one of left/right.
		const std::string& name = raw.direction;
		const auto dash = name.find('-');
		if (dash == std::string::npos)
		{
			// Nothing written: none (a hop on a cell map, one cell).
			CmdHop hop{ directionFromName(name, raw.verb), valueOr(raw.amount, 0.0f, evaluate), {}, hopSeconds(raw, evaluate), 0.0f, {} };
			hop.arc = arcOf(raw, "hop", hop.seconds, evaluate);
			hop.step = stepToward(hop.direction, hop.distance);
			hop.onto = raw.onto;
			return hop;
		}

		const Direction upDown = directionFromName(name.substr(0, dash), raw.verb);
		const Direction leftRight = directionFromName(name.substr(dash + 1), raw.verb);
		if ((upDown != Direction::Up && upDown != Direction::Down) || (leftRight != Direction::Left && leftRight != Direction::Right))
		{
			throw std::runtime_error("<hop> has direction=\"" + name + "\"; a diagonal is up-left, up-right, down-left or down-right");
		}
		const bool split = !raw.positionX.text.empty() || raw.positionX.kind != RawValue::Kind::Expression;
		const float distance = split ? 0.0f : evaluate(raw.amount);
		const float across = split ? evaluate(raw.positionX) : distance;
		const float down = split ? evaluate(raw.positionY) : distance;
		CmdHop hop{ upDown, distance, {}, hopSeconds(raw, evaluate), 0.0f, {} };
		hop.arc = arcOf(raw, "hop", hop.seconds, evaluate);
		hop.step = { leftRight == Direction::Left ? -across : across, upDown == Direction::Up ? -down : down };
		return hop;
	}

	static bool runHop(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || executor.hop(*doer.self, std::get<CmdHop>(command));
	}

	static void printHop(std::ostream& o, const Command& command)
	{
		const CmdHop& h = std::get<CmdHop>(command);
		const char* direction = (h.direction == Direction::Up) ? "up"
			: (h.direction == Direction::Down) ? "down"
			: (h.direction == Direction::Left) ? "left"
			: "right";
		if (h.step.x != 0.0f && h.step.y != 0.0f) { o << "hop(" << h.step.x << ", " << h.step.y; }
		else { o << "hop." << direction << "(" << h.distance; }
		o << (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
			<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{});
		for (const std::string& onto : h.onto) { o << ", onto " << onto; }
		o << ")";
	}

	static void printHopRaw(std::ostream& o, const RawCommand& command)
	{
		o << (command.byHops ? "(" + command.object + ", by hops of " : "." + command.direction + "(") << command.amount << ", " << command.positionX << " " << command.positionY
			<< ", " << command.seconds << ")";
	}

	extern const Verb hopVerb;
	const Verb hopVerb{
		.name = "hop",
		.forms = { { "<hop>", places::self, kindOf<CmdHop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readHop,
		.make = makeHop,
		.check = nullptr,
		.run = runHop,
		.print = printHop,
		.printRaw = printHopRaw,
	};
}
