// alternate.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
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
		std::vector<std::string>& players = command.own.emplace<RawAlternate>().players;
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "player") { fail(here, "unknown <" + child->getName() + ">; expected a <player> for each player"); }
			players.push_back(child->getAttribute("state"));
		}
		if (players.empty()) { fail(here, "has no <player>; it needs one for each player"); }
	}

	static Command makeAlternate(const RawCommand& raw, const ValueEvaluator&)
	{
		const std::vector<std::string>& players = ownOf<RawAlternate>(raw).players;
		if (players.empty()) { throw std::runtime_error("<alternate> has no <player>; it needs one for each player"); }
		return CmdAlternate{ players };
	}

	static void checkAlternate(const CmdAlternate& alternate, const Checking& checking)
	{
		for (const std::string& player : alternate.players)
		{
			if (!player.empty()) { checkState(checking, player, "<alternate> > <player state=\"" + player + "\" />"); }
		}
	}

	static bool runAlternate(const CmdAlternate& alternate, const Doer& doer)
	{
		if (!doer.self) { doer.game().startTurns(alternate.players); }
		return true;
	}

	static void printAlternate(std::ostream& o, const CmdAlternate& alternate)
	{
		o << "alternate(";
		for (std::size_t i = 0; i < alternate.players.size(); ++i) { o << (i ? ", " : "") << (alternate.players[i].empty() ? "-" : alternate.players[i]); }
		o << ")";
	}

	extern const Verb alternateVerb;
	const Verb alternateVerb{
		.name = "alternate",
		.forms = { { "<alternate>", places::flow, kindOf<CmdAlternate>() } },
		.read = readAlternate,
		.make = makeAlternate,
		.check = checkAs<checkAlternate>,
		.run = runAs<runAlternate>,
		.print = printAs<printAlternate>,
	};
}
