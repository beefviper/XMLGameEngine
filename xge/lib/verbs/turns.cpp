// turns.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace xge
{
	// <turns>: Starts a game the players take in turn.
	static void readTurns(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// A <player> for each player, each with the state (if any) that says it
		// is their turn.
		const std::string here = where + " > <turns>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "player") { fail(here, "unknown <" + child->getName() + ">; expected a <player> for each player"); }
			command.players.push_back(child->getAttribute("state"));
		}
		if (command.players.empty()) { fail(here, "has no <player>; it needs one for each player"); }
	}

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
		.read = readTurns,
		.make = makeTurns,
		.check = checkTurns,
		.run = runTurns,
	};
}
