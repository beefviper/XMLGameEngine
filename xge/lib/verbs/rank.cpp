// rank.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <rank>: Enters a score in a table that lasts.
	static Command makeRank(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdRank{ VariableName::of(raw.variable), raw.into };
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
		.run = runRank,
	};
}
