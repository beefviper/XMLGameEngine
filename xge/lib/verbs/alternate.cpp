// alternate.cpp
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
	// <alternate>: Starts a game the players take in turn, each with a game
	// of their own (the arcade way: the machine swaps whole games).
	static void readAlternate(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// A <player> for each player, each with the state (if any) that says it
		// is their turn.
		const std::string here = where + " > <alternate>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "player") { fail(here, "unknown <" + child->getName() + ">; expected a <player> for each player"); }
			command.players.push_back(child->getAttribute("state"));
		}
		if (command.players.empty()) { fail(here, "has no <player>; it needs one for each player"); }
	}

	static Command makeAlternate(const RawCommand& raw, const ValueEvaluator&)
	{
		if (raw.players.empty()) { throw std::runtime_error("<alternate> has no <player>; it needs one for each player"); }
		return CmdAlternate{ raw.players };
	}

	static void checkAlternate(const Command& command, const Checking& checking)
	{
		for (const std::string& player : std::get<CmdAlternate>(command).players)
		{
			if (!player.empty()) { checkState(checking, player, "<alternate> > <player state=\"" + player + "\" />"); }
		}
	}

	static bool runAlternate(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { executor.game.startTurns(std::get<CmdAlternate>(command).players); }
		return true;
	}

	extern const Verb alternateVerb;
	const Verb alternateVerb{
		.name = "alternate",
		.forms = { { "<alternate>", places::flow, kindOf<CmdAlternate>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readAlternate,
		.make = makeAlternate,
		.check = checkAlternate,
		.run = runAlternate,
	};
}
