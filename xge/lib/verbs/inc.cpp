// inc.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <string>
#include <variant>

namespace xge
{
	// <inc>: Adds to a variable.
	static void readInc(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "variable", where);
		// The amount is optional: a bare <inc variable="..." /> is 1.
		RawInc& own = command.own.emplace<RawInc>();
		if (hasContent(node)) { own.by = readValue(node, where); }
	}

	static Command makeInc(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// By its amount, or by 1 when it has none.
		return CmdIncrement{ VariableName::of(raw.variable), ownOf<RawInc>(raw).by ? evaluate(*ownOf<RawInc>(raw).by) : 1.0f };
	}

	static void checkInc(const Command& command, const Checking& checking)
	{
		// A variable belongs to an object: paddle1.score is paddle1's
		// <variable name="score">.
		const VariableName& target = std::get<CmdIncrement>(command).target;
		const std::string tag = "<inc variable=\"" + (target.variable.empty() ? target.owner : target.written()) + "\" />";
		checkVariable(checking, target, tag);
		refuseFollowed(checking, target, tag);
	}

	static bool runInc(const Command& command, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		const CmdIncrement& inc = std::get<CmdIncrement>(command);
		executor.game.addTo(inc.target, inc.amount * executor.heldShare);
		return true;
	}

	static void printInc(std::ostream& o, const Command& command)
	{
		const CmdIncrement& c = std::get<CmdIncrement>(command);
		o << "inc(" << c.target.written() << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")";
	}

	extern const Verb incVerb;
	const Verb incVerb{
		.name = "inc",
		.forms = { { "<inc>", places::changes, kindOf<CmdIncrement>() } },
		.read = readInc,
		.make = makeInc,
		.check = checkInc,
		.run = runInc,
		.print = printInc,
	};
}
