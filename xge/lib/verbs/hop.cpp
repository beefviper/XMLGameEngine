// hop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/geometry.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <algorithm>
#include <cmath>
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
			CmdHop hop{ directionFromName(name, raw.verb), valueOr(written.distance, 0.0f, evaluate), {}, hopSeconds(written, evaluate), 0.0f, {} };
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

	// Through the air by `whole` over `seconds`, touching nothing until it
	// lands: a <hop> that takes time. Not when it is in the air already, nor
	// when it would land outside the window. With an arc it is drawn that
	// high above its way halfway there.
	static bool startFlight(Game& game, Object& object, Vector2f whole, float seconds, float arc)
	{
		if (object.isAirborne())
		{
			return false;
		}

		// A jump that would land off the screen is not made at all (up from the
		// top row stays where it is), rather than going part of the way.
		const WindowDesc& window = game.getWindowDesc();
		const Box landing{ object.position + whole, sizeOf(object) };
		if (!insideWindow(landing, { window.width, window.height }))
		{
			return false;
		}

		object.flight.whole = whole;
		// None at all: it lands on the next step.
		object.flight.seconds = std::max(seconds, kTimeSlack);
		object.flight.flown = 0.0;
		object.flight.arc = arc;
		return true;
	}

	bool hopObject(Game& game, Object& object, const CmdHop& hop)
	{
		// On a cell map: whole cells (one, with no distance), refused (and
		// touching) when it cannot go.
		if (game.isHand(object)) { return game.hopHand(object, hop.direction); }
		if (object.onCells && !hop.onto.empty()) { return game.hopOnto(object, hop.direction, hop.onto); }
		if (object.onCells)
		{
			const auto cells = [](float pixels, float distance)
			{
				if (pixels == 0.0f && distance == 0.0f) { return 0; }
				const int count = std::max(1, static_cast<int>(std::lround(std::fabs(distance > 0.0f ? distance : pixels))));
				return pixels < 0.0f ? -count : count;
			};
			Vector2f way = hop.step;
			if (way.x == 0.0f && way.y == 0.0f) { way = stepToward(hop.direction, 1.0f); }
			const int across = way.x == 0.0f ? 0 : cells(way.x, hop.distance);
			const int down = way.y == 0.0f ? 0 : cells(way.y, hop.distance);
			if (!game.stepCells(object, across, down)) { return false; }
			object.facing = hop.direction;
			return true;
		}

		// One in the air waits for the landing.
		if (object.isAirborne()) { return false; }

		if (hop.seconds > 0.0f)
		{
			if (!startFlight(game, object, hop.step, hop.seconds, hop.arc)) { return false; }
		}
		else
		{
			// playOneFrame refuses it again where the object is then; asked
			// here, so the action knows at once.
			const WindowDesc& window = game.getWindowDesc();
			if (!insideWindow({ object.position + hop.step, sizeOf(object) }, { window.width, window.height })) { return false; }
			object.hopPending = hop.step;
		}
		object.facing = hop.direction;
		if (hop.step.x != 0.0f) { object.mirror.goingLeft = hop.step.x < 0.0f; }
		object.aim.reset();
		return true;
	}

	static bool runHop(const CmdHop& hop, const Doer& doer)
	{
		return !doer.self || hopObject(doer.game(), *doer.self, hop);
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
