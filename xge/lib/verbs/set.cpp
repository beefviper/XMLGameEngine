// set.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// <set>: Gives a variable, or one of an object's own numbers, a value.
	static void readSet(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "variable", where);
		command.amount = readValue(node, where);
	}

	static Command makeSet(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// One of the numbers every object has (ship.velocity.x) is split before
		// its part, not at its last dot.
		const std::string part = liveNumberPart(raw.variable);
		VariableName target = part.empty() ? VariableName::of(raw.variable)
			: VariableName{ raw.variable.substr(0, raw.variable.size() - part.size() - 1), part };
		return CmdSet{ std::move(target), evaluate(raw.amount) };
	}

	static void checkSet(const Command& command, const Checking& checking)
	{
		// A variable of an object's, or one of the numbers every object has
		// (where it is, how it is going, its heading).
		const CmdSet& set = std::get<CmdSet>(command);
		const std::vector<Object>& objects = checking.objects;
		const std::string tag = "<set variable=\"" + (set.target.variable.empty() ? set.target.owner : set.target.written()) + "\">";
		refuseFollowed(checking, set.target, tag);
		const std::string part = liveNumberPart(set.target.written());
		const Object* holder = part.empty() ? nullptr : findObject(objects, set.target.owner);
		if (part.empty() || (holder && holder->variable.count(part)))
		{
			checkVariable(checking, set.target, tag);
			return;
		}

		const std::string& owner = set.target.owner;
		const Object* found = findObject(objects, owner);
		if (!found)
		{
			throw std::runtime_error(checking.where + ": " + tag + ": there is no object named '" + owner + "'" + didYouMean(owner, objectNames(objects)));
		}
		if (part == "heading" && !found->hasHeading)
		{
			throw std::runtime_error(checking.where + ": " + tag + ": '" + owner + "' has no <heading> to set");
		}
		if ((part == "altitude" || part == "rise") && !found->hasAltitude)
		{
			throw std::runtime_error(checking.where + ": " + tag + ": '" + owner + "' has no <altitude> to set");
		}
	}

	static bool runSet(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdSet& set = std::get<CmdSet>(command);
		executor.game.setNumber(set.target, set.value);
		return true;
	}

	static void printSet(std::ostream& o, const Command& command)
	{
		const CmdSet& s = std::get<CmdSet>(command);
		o << "set(" << s.target.written() << ", " << s.value << ")";
	}

	static void printSetRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.variable << ", " << command.amount << ")";
	}

	extern const Verb setVerb;
	const Verb setVerb{
		.name = "set",
		.forms = { { "<set>", places::changes, kindOf<CmdSet>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readSet,
		.make = makeSet,
		.check = checkSet,
		.run = runSet,
		.print = printSet,
		.printRaw = printSetRaw,
	};
}
