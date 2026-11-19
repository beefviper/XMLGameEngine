// chase.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <chase>: Heads straight for another object.
	static void readChase(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Its speed as its content, then how near is near enough.
		requireAttribute(node, "object", where);
		const std::string here = where + " > <chase>";
		command.amount = readLeadValue(node, { "near" }, "its speed, then <near> if wanted", where);
		if (auto nearNode = findChild(&node, "near")) { command.distance = readValue(*nearNode, here); }
	}

	static Command makeChase(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float speed = evaluate(raw.amount);
		const float stopWithin = valueOr(raw.distance, 0.0f, evaluate);
		if (speed < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: its speed is below 0"); }
		if (stopWithin < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: the <near> is below 0"); }
		return CmdChase{ raw.object, speed, stopWithin };
	}

	static void checkChase(const CmdChase& chase, const Checking& checking)
	{
		checkObject(checking, chase.target, "<chase>");
	}

	// Heads for the nearest of `chase.target` in play: straight at its middle
	// at `chase.speed`, or, within `chase.stopWithin` of it, stops (inside
	// this step, if it gets there). It faces the
	// way it goes, and a heading turns to it. With none in play it carries on
	// as it was.
	static void chaseNearest(Game& game, Object& object, const CmdChase& command)
	{
		const Object* target = game.nearestInPlay(object, command.target);
		if (!target) { return; }

		if (const auto way = xge::chase(middleOf(object), middleOf(*target), command.speed, command.stopWithin, game.stepTime(), object.velocity))
		{
			object.facing = directionOf(*way);
			if (object.turning.hasHeading)
			{
				object.heading = headingOf(*way);
				object.showHeading();
			}
		}
	}

	static bool runChase(const CmdChase& chase, const Doer& doer)
	{
		if (doer.self) { chaseNearest(doer.game(), *doer.self, chase); }
		return true;
	}

	static void printChase(std::ostream& o, const CmdChase& chase)
	{
		o << "chase(" << chase.target << ", " << chase.speed << (chase.stopWithin > 0.0f ? ", near " + formatDisplayNumber(chase.stopWithin) : std::string{}) << ")";
	}

	extern const Verb chaseVerb;
	const Verb chaseVerb{
		.name = "chase",
		.forms = { { "<chase>", places::steering, kindOf<CmdChase>() } },
		.read = readChase,
		.make = makeChase,
		.check = checkAs<checkChase>,
		.run = runAs<runChase>,
		.print = printAs<printChase>,
	};
}
