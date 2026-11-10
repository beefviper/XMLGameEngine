// rank.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/spelling.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <rank>: Enters a score in a table that lasts.
	static Command makeRank(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdRank{ VariableName::of(raw.variable), raw.into };
	}

	static void checkRank(const Command& command, const Checking& checking)
	{
		// The entry's variable, and a group whose every member has one of that
		// name to be ordered by.
		const CmdRank& rank = std::get<CmdRank>(command);
		const std::string tag = "<rank variable=\"" + rank.entry.written() + "\" into=\"" + rank.table + "\" />";
		checkVariable(checking, rank.entry, tag);
		const std::string& variableName = rank.entry.variable;
		bool any = false;
		for (const Object& object : checking.objects)
		{
			if (!object.inGroup(rank.table)) { continue; }
			any = true;
			if (!object.variable.count(variableName))
			{
				throw std::runtime_error(checking.where + ": " + tag + ": '" + object.name + "' has no variable named '" + variableName + "' to be ranked by");
			}
		}
		if (!any)
		{
			throw std::runtime_error(checking.where + ": " + tag + ": into= names no <group> of the game" + didYouMean(rank.table, objectNames(checking.objects)));
		}
	}

	static bool runRank(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdRank& rank = std::get<CmdRank>(command);
		executor.game.rank(rank.entry, rank.table);
		return true;
	}

	extern const Verb rankVerb;
	const Verb rankVerb{
		.name = "rank",
		.forms = { { "<rank>", places::changes, kindOf<CmdRank>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeRank,
		.check = checkRank,
		.run = runRank,
	};
}
