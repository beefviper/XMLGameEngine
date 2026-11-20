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
		return CmdDecrement{ VariableName::of(raw.variable), valueOr(ownOf<RawDec>(raw).by, 1.0f, evaluate) };
	}

	static void checkDec(const CmdDecrement& decrement, const Checking& checking)
	{
		// A variable belongs to an object: paddle1.score is paddle1's
		// <variable name="score">.
		const VariableName& target = decrement.target;
		const std::string tag = "<dec variable=\"" + (target.variable.empty() ? target.owner.str() : target.written()) + "\" />";
		checkVariable(checking, target, tag);
		refuseFollowed(checking, target, tag);
	}

	static bool runDec(const CmdDecrement& dec, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		executor.game.addTo(dec.target, -dec.amount * executor.heldShare);
		return true;
	}

	static void printDec(std::ostream& o, const CmdDecrement& decrement)
	{
		o << "dec(" << decrement.target.written() << (decrement.amount == 1.0f ? "" : ", " + formatDisplayNumber(decrement.amount)) << ")";
	}

	extern const Verb decVerb;
	const Verb decVerb{
		.name = "dec",
		.forms = { { "<dec>", places::changes, kindOf<CmdDecrement>() } },
		.names = { .variable = Naming::Variable },
		.read = readDec,
		.make = makeDec,
		.check = checkAs<checkDec>,
		.run = runAs<runDec>,
		.print = printAs<printDec>,
	};
}
