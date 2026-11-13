// copy.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <string>

namespace xge
{
	// <copy>: One variable's value into another.
	static void readCopy(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "variable", where);
		requireAttribute(node, "into", where);
	}

	static Command makeCopy(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdCopy{ VariableName::of(raw.variable), VariableName::of(raw.into) };
	}

	static void checkCopy(const Command& command, const Checking& checking)
	{
		// Both are owner.variable: a copy reads one object's variable and writes
		// another's.
		const CmdCopy& copy = std::get<CmdCopy>(command);
		const std::string tag = "<copy variable=\"" + copy.from.written() + "\" into=\"" + copy.into.written() + "\" />";
		checkVariable(checking, copy.from, tag);
		checkVariable(checking, copy.into, tag);
		refuseFollowed(checking, copy.into, tag);
	}

	static bool runCopy(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdCopy& copy = std::get<CmdCopy>(command);
		executor.game.copyVariable(copy.from, copy.into);
		return true;
	}

	static void printCopy(std::ostream& o, const Command& command)
	{
		const CmdCopy& c = std::get<CmdCopy>(command);
		o << "copy(" << c.from.written() << ", " << c.into.written() << ")";
	}

	extern const Verb copyVerb;
	const Verb copyVerb{
		.name = "copy",
		.forms = { { "<copy>", places::changes, kindOf<CmdCopy>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readCopy,
		.make = makeCopy,
		.check = checkCopy,
		.run = runCopy,
		.print = printCopy,
	};
}
