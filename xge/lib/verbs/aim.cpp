// aim.cpp
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

namespace xge
{
	// <aim>: Faces another object; with <lined>, refused unless it is lined
	// up.
	static void readAim(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Optionally only when lined up, within so many pixels.
		requireAttribute(node, "object", where);
		RawAim& aim = command.own.emplace<RawAim>();
		if (auto lined = findChild(&node, "lined")) { aim.lined = readValue(*lined, where + " > <aim>"); }
	}

	static Command makeAim(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const std::optional<RawValue>& written = ownOf<RawAim>(raw).lined;
		const bool lined = written.has_value();
		const float within = lined ? evaluate(*written) : 0.0f;
		if (within < 0.0f) { throw std::runtime_error("<aim object=\"" + raw.object + "\">: the <lined> is below 0"); }
		return CmdAim{ raw.object, lined, within };
	}

	static void checkAim(const CmdAim& aim, const Checking& checking)
	{
		checkObject(checking, aim.target, "<aim>");
	}

	// Turns to the nearest of `target` in play: the next shots go straight at
	// where its middle is now, and the object faces that way (to the nearest
	// of the four, or exactly, if it has a heading). Lined, only along one of
	// the eight ways, and refused (false) when none is near enough.
	static bool aimAtNearest(Game& game, Object& object, const CmdAim& command)
	{
		const Object* other = game.nearestInPlay(object, command.target);
		if (!other) { return !command.lined; }

		auto way = wayTo(middleOf(object), middleOf(*other));
		if (!way) { return !command.lined; }
		if (command.lined)
		{
			way = linedUp(middleOf(*other) - middleOf(object), command.within);
			if (!way) { return false; }
		}

		object.aim = *way;
		object.facing = directionOf(*way);
		if (object.turning.hasHeading)
		{
			object.heading = headingOf(*way);
			object.showHeading();
		}
		return true;
	}

	static bool runAim(const CmdAim& aim, const Doer& doer)
	{
		return !doer.self || aimAtNearest(doer.game(), *doer.self, aim);
	}

	static void printAim(std::ostream& o, const CmdAim& aim)
	{
		o << "aim(" << aim.target << (aim.lined ? ", lined " + std::to_string(aim.within) : std::string()) << ")";
	}

	extern const Verb aimVerb;
	const Verb aimVerb{
		.name = "aim",
		.forms = { { "<aim>", places::steering, kindOf<CmdAim>() } },
		.read = readAim,
		.make = makeAim,
		.check = checkAs<checkAim>,
		.run = runAs<runAim>,
		.print = printAs<printAim>,
	};
}
