// dec.cpp
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
	// <dec>: Takes from a variable.
	static void readDec(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "variable", where);
		// The amount is optional: a bare <dec variable="..." /> is 1.
		RawDec& own = command.own.emplace<RawDec>();
		if (hasContent(node)) { own.by = readValue(node, where); }
	}

	static Command makeDec(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// By its amount, or by 1 when it has none.
		return CmdDecrement{ VariableName::of(raw.variable), ownOf<RawDec>(raw).by ? evaluate(*ownOf<RawDec>(raw).by) : 1.0f };
	}

	static void checkDec(const Command& command, const Checking& checking)
	{
		// A variable belongs to an object: paddle1.score is paddle1's
		// <variable name="score">.
		const VariableName& target = std::get<CmdDecrement>(command).target;
		const std::string tag = "<dec variable=\"" + (target.variable.empty() ? target.owner : target.written()) + "\" />";
		checkVariable(checking, target, tag);
		refuseFollowed(checking, target, tag);
	}

	static bool runDec(const Command& command, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		const CmdDecrement& dec = std::get<CmdDecrement>(command);
		executor.game.addTo(dec.target, -dec.amount * executor.heldShare);
		return true;
	}

	static void printDec(std::ostream& o, const Command& command)
	{
		const CmdDecrement& c = std::get<CmdDecrement>(command);
		o << "dec(" << c.target.written() << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")";
	}

	extern const Verb decVerb;
	const Verb decVerb{
		.name = "dec",
		.forms = { { "<dec>", places::changes, kindOf<CmdDecrement>() } },
		.read = readDec,
		.make = makeDec,
		.check = checkDec,
		.run = runDec,
		.print = printDec,
	};
}
