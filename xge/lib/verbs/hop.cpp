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
#include <variant>

namespace xge
{
	// <hop>: At once, or with <seconds> through the air.
	static void readHop(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		RawHop& hop = command.own.emplace<RawHop>();
		if (findChild(&node, "onto"))
		{
			readOnto(node, where + " > <hop>", hop.onto);
			if (command.direction.find('-') != std::string::npos) { fail(where, "<hop direction=\"" + command.direction + "\"> with <onto>: only up, down, left or right"); }
		}
		else if (readHopParts(node, where, hop) && command.direction.find('-') == std::string::npos)
		{
			fail(where, "<hop direction=\"" + command.direction + "\"> is not diagonal; it gives one distance");
		}
	}

	static Command makeHop(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const RawHop& written = ownOf<RawHop>(raw);
		// up-left and the rest: one of up/down, then one of left/right.
		const std::string& name = raw.direction;
		const auto dash = name.find('-');
		if (dash == std::string::npos)
		{
			// Nothing written: none (a hop on a cell map, one cell).
			CmdHop hop{ directionFromName(name, raw.verb), written.distance ? evaluate(*written.distance) : 0.0f, {}, hopSeconds(written, evaluate), 0.0f, {} };
			hop.arc = arcOf(written, "hop", hop.seconds, evaluate);
			hop.step = stepToward(hop.direction, hop.distance);
			hop.onto = written.onto;
			return hop;
		}

		const Direction upDown = directionFromName(name.substr(0, dash), raw.verb);
		const Direction leftRight = directionFromName(name.substr(dash + 1), raw.verb);
		if ((upDown != Direction::Up && upDown != Direction::Down) || (leftRight != Direction::Left && leftRight != Direction::Right))
		{
			throw std::runtime_error("<hop> has direction=\"" + name + "\"; a diagonal is up-left, up-right, down-left or down-right");
		}
		const float distance = written.diagonal ? 0.0f : evaluate(*written.distance);
		const float across = written.diagonal ? evaluate(written.diagonal->x) : distance;
		const float down = written.diagonal ? evaluate(written.diagonal->y) : distance;
		CmdHop hop{ upDown, distance, {}, hopSeconds(written, evaluate), 0.0f, {} };
		hop.arc = arcOf(written, "hop", hop.seconds, evaluate);
		hop.step = { leftRight == Direction::Left ? -across : across, upDown == Direction::Up ? -down : down };
		return hop;
	}

	static bool runHop(const CmdHop& hopCommand, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		return !doer.self || executor.hop(*doer.self, hopCommand);
	}

	static void printHop(std::ostream& o, const CmdHop& hop)
	{
		const char* direction = (hop.direction == Direction::Up) ? "up"
			: (hop.direction == Direction::Down) ? "down"
			: (hop.direction == Direction::Left) ? "left"
			: "right";
		if (hop.step.x != 0.0f && hop.step.y != 0.0f) { o << "hop(" << hop.step.x << ", " << hop.step.y; }
		else { o << "hop." << direction << "(" << hop.distance; }
		o << (hop.seconds > 0.0f ? ", " + formatDisplayNumber(hop.seconds) + "s" : std::string{})
			<< (hop.arc > 0.0f ? ", arc " + formatDisplayNumber(hop.arc) : std::string{});
		for (const std::string& onto : hop.onto) { o << ", onto " << onto; }
		o << ")";
	}

	extern const Verb hopVerb;
	const Verb hopVerb{
		.name = "hop",
		.forms = { { "<hop>", places::self, kindOf<CmdHop>() } },
		.read = readHop,
		.make = makeHop,
		.run = runAs<runHop>,
		.print = printAs<printHop>,
	};
}
