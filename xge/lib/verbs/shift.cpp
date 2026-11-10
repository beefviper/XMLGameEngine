// shift.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
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
		if (by.x) { command.positionX = *by.x; }
		if (by.y) { command.positionY = *by.y; }
	}

	static Command makeShift(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdShift{ raw.object, { valueOr(raw.positionX, 0.0f, evaluate), valueOr(raw.positionY, 0.0f, evaluate) } };
	}

	static void checkShift(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdShift>(command).target, "<shift>");
	}

	static bool runShift(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdShift& shift = std::get<CmdShift>(command);
		executor.game.shift(shift.target, shift.by);
		return true;
	}

	extern const Verb shiftVerb;
	const Verb shiftVerb{
		.name = "shift",
		.forms = { { "<shift>", places::changes, kindOf<CmdShift>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readShift,
		.make = makeShift,
		.check = checkShift,
		.run = runShift,
	};
}
