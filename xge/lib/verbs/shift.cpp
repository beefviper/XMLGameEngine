// shift.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"

namespace xge
{
	// <shift>: Moves an object, and all of a group, by an amount.
	static void readShift(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		const PartialVector2 by = readPartialVector2(node, where);
		if (!by.x && !by.y) { fail(where, "<shift> moves by an <x> across, a <y> down, or both; it has neither"); }
		RawShift& shift = command.own.emplace<RawShift>();
		shift.x = by.x;
		shift.y = by.y;
	}

	static Command makeShift(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdShift{ raw.object, { valueOr(ownOf<RawShift>(raw).x, 0.0f, evaluate), valueOr(ownOf<RawShift>(raw).y, 0.0f, evaluate) } };
	}

	static void checkShift(const CmdShift& shift, const Checking& checking)
	{
		checkObject(checking, shift.target, "<shift>");
	}

	static bool runShift(const CmdShift& shift, const Doer& doer)
	{
		doer.game().shift(shift.target, shift.by);
		return true;
	}

	static void printShift(std::ostream& o, const CmdShift& shift)
	{
		o << "shift(" << shift.target << ", " << shift.by.x << ", " << shift.by.y << ")";
	}

	extern const Verb shiftVerb;
	const Verb shiftVerb{
		.name = "shift",
		.forms = { { "<shift>", places::changes, kindOf<CmdShift>() } },
		.read = readShift,
		.make = makeShift,
		.check = checkAs<checkShift>,
		.run = runAs<runShift>,
		.print = printAs<printShift>,
	};
}
