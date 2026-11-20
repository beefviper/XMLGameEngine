// copy.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"

#include <string>

namespace xge
{
	// <copy>: One variable's value into another.
	static void readCopy(const XmlNode& node, const std::string& where, WrittenCommand&)
	{
		requireAttribute(node, "variable", where);
		requireAttribute(node, "into", where);
	}

	static Command makeCopy(const WrittenCommand& raw, const ValueEvaluator&)
	{
		return CmdCopy{ VariableName::of(raw.variable), VariableName::of(raw.into) };
	}

	static void checkCopy(const CmdCopy& copy, const Checking& checking)
	{
		// Both are owner.variable: a copy reads one object's variable and writes
		// another's.
		const std::string tag = "<copy variable=\"" + copy.from.written() + "\" into=\"" + copy.into.written() + "\" />";
		checkVariable(checking, copy.from, tag);
		checkVariable(checking, copy.into, tag);
		refuseFollowed(checking, copy.into, tag);
	}

	static bool runCopy(const CmdCopy& copy, const Doer& doer)
	{
		doer.game().copyVariable(copy.from, copy.into);
		return true;
	}

	static void printCopy(std::ostream& o, const CmdCopy& copy)
	{
		o << "copy(" << copy.from.written() << ", " << copy.into.written() << ")";
	}

	extern const Verb copyVerb;
	const Verb copyVerb{
		.name = "copy",
		.forms = { { "<copy>", places::changes, kindOf<CmdCopy>() } },
		.names = { .variable = Naming::Variable, .into = Naming::Variable },
		.read = readCopy,
		.make = makeCopy,
		.check = checkAs<checkCopy>,
		.run = runAs<runCopy>,
		.print = printAs<printCopy>,
	};
}
