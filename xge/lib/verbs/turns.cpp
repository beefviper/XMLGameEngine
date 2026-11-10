// turns.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <turns>: Starts a game the players take in turn.
	static Command makeTurns(const RawCommand& raw, const ValueEvaluator&)
	{
		if (raw.players.empty()) { throw std::runtime_error("<turns> has no <player>; it needs one for each player"); }
		return CmdTurns{ raw.players };
	}

	static void checkTurns(const Command& command, const Checking& checking)
	{
		for (const std::string& player : std::get<CmdTurns>(command).players)
		{
			if (!player.empty()) { checkState(checking, player, "<turns> > <player state=\"" + player + "\" />"); }
		}
	}

	static bool runTurns(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { executor.game.startTurns(std::get<CmdTurns>(command).players); }
		return true;
	}

	extern const Verb turnsVerb;
	const Verb turnsVerb{
		.name = "turns",
		.forms = { { "<turns>", places::flow, kindOf<CmdTurns>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeTurns,
		.check = checkTurns,
		.run = runTurns,
	};
}
