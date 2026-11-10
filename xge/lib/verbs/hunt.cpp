// hunt.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/maze.h"
#include "load/check.h"
#include "load/reading.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace xge
{
	// <hunt>: After another object through a maze, or towards it hop by hop.
	static void readHunt(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		const std::string here = where + " > <hunt>";
		if (findChild(&node, "hop"))
		{
			// A hunt by hops: the one hop it makes toward what it hunts.
			std::unique_ptr<XmlNode> hop = node.getFirstChild();
			if (hop->getName() != "hop" || hop->getNextSibling() || !readText(node).empty())
			{
				fail(here, "a hunt by hops has its <hop> and nothing else (no speed, <ahead>, <with> or <retreat>: those are for a maze)");
			}
			if (!hop->getAttribute("direction").empty()) { fail(here + " > <hop>", "has a direction=; the hunt chooses the way"); }
			command.byHops = true;
			readHopParts(*hop, here, command);
			return;
		}

		// What it hunts, how fast, and how its target is found from that: tiles
		// ahead of it, with a partner, or backing off when near.
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag == "with") { command.partner = requireAttribute(*child, "object", here + " > <with>"); }
			else if (tag == "retreat")
			{
				command.retreat = requireAttribute(*child, "object", here + " > <retreat>");
				command.distance = readValue(*child, here + " > <retreat>");
			}
		}
		command.amount = readLeadValue(node, { "ahead", "with", "retreat" }, "its speed, then <ahead>, <with> and <retreat>, each if wanted", where);
		if (auto ahead = findChild(&node, "ahead")) { command.ahead = readValue(*ahead, here); }
	}

	static Command makeHunt(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (raw.byHops)
		{
			const bool diagonal = !raw.positionX.text.empty() || raw.positionX.kind != RawValue::Kind::Expression;
			const float distance = diagonal ? 0.0f : evaluate(raw.amount);
			const float seconds = hopSeconds(raw, evaluate);
			return CmdHuntHop{ raw.object, diagonal ? evaluate(raw.positionX) : distance, diagonal ? evaluate(raw.positionY) : distance, diagonal, seconds, arcOf(raw, "hop", seconds, evaluate) };
		}

		CmdHunt hunt{ raw.object, speedOf(raw, "hunt", evaluate), 0.0f, raw.partner, raw.retreat, 0.0f };
		if (isWritten(raw.ahead)) { hunt.ahead = evaluate(raw.ahead); }
		if (!raw.retreat.empty())
		{
			hunt.retreatWithin = evaluate(raw.distance);
			if (hunt.retreatWithin < 0.0f) { throw std::runtime_error("<hunt object=\"" + raw.object + "\">: the <retreat> is below 0 tiles"); }
		}
		return hunt;
	}

	static void checkHunt(const Command& command, const Checking& checking)
	{
		if (const auto* byHops = std::get_if<CmdHuntHop>(&command))
		{
			checkObject(checking, byHops->target, "<hunt>");
			return;
		}
		const CmdHunt& hunt = std::get<CmdHunt>(command);
		if (!hunt.target.empty()) { checkObject(checking, hunt.target, "<hunt>"); }
		if (!hunt.partner.empty()) { checkObject(checking, hunt.partner, "<hunt> > <with>"); }
		if (!hunt.retreatTo.empty()) { checkObject(checking, hunt.retreatTo, "<hunt> > <retreat>"); }
	}

	static bool runHunt(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { return true; }
		if (const auto* byHops = std::get_if<CmdHuntHop>(&command)) { return executor.huntByHop(*doer.self, *byHops); }
		startHunt(*doer.self, std::get<CmdHunt>(command));
		return true;
	}

	extern const Verb huntVerb;
	const Verb huntVerb{
		.name = "hunt",
		.forms = { { "<hunt>", places::steering, kindOf<CmdHunt>() }, { "<hunt> by <hop>", places::steering, kindOf<CmdHuntHop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readHunt,
		.make = makeHunt,
		.check = checkHunt,
		.run = runHunt,
	};
}
