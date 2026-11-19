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
		return CmdIncrement{ VariableName::of(raw.variable), valueOr(ownOf<RawInc>(raw).by, 1.0f, evaluate) };
	}

	static void checkInc(const CmdIncrement& increment, const Checking& checking)
	{
		// A variable belongs to an object: paddle1.score is paddle1's
		// <variable name="score">.
		const VariableName& target = increment.target;
		const std::string tag = "<inc variable=\"" + (target.variable.empty() ? target.owner : target.written()) + "\" />";
		checkVariable(checking, target, tag);
		refuseFollowed(checking, target, tag);
	}

	static bool runInc(const CmdIncrement& inc, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		executor.game.addTo(inc.target, inc.amount * executor.heldShare);
		return true;
	}

	static void printInc(std::ostream& o, const CmdIncrement& increment)
	{
		o << "inc(" << increment.target.written() << (increment.amount == 1.0f ? "" : ", " + formatDisplayNumber(increment.amount)) << ")";
	}

	extern const Verb incVerb;
	const Verb incVerb{
		.name = "inc",
		.forms = { { "<inc>", places::changes, kindOf<CmdIncrement>() } },
		.names = { .variable = Naming::Variable },
		.read = readInc,
		.make = makeInc,
		.check = checkAs<checkInc>,
		.run = runAs<runInc>,
		.print = printAs<printInc>,
	};
}
