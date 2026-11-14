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
#include "play/geometry.h"
#include "play/verbs.h"

#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

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
			else if (tag == "search") { command.search = true; }
		}
		command.amount = readLeadValue(node, { "ahead", "with", "retreat", "search" }, "its speed, then <ahead>, <with>, <retreat> and <search />, each if wanted", where);
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

		CmdHunt hunt{ raw.object, speedOf(raw, "hunt", evaluate), 0.0f, raw.partner, raw.retreat, 0.0f, raw.search };
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

	// A hunt by hops: see CmdHuntHop.
	static bool huntByHops(CommandExecutor& executor, Object& object, const CmdHuntHop& hunt)
	{
		if (object.isAirborne() || !executor.game.isShown(object)) { return false; }

		const Object* quarry = executor.game.nearestInPlay(object, hunt.target);
		if (!quarry) { return false; }

		// The four hops it could make, in the order a tie goes: up before
		// down, then left before right.
		const std::array<std::pair<Direction, Vector2f>, 4> hops = hunt.diagonal
			? std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { -hunt.across, -hunt.down } }, { Direction::Up, { hunt.across, -hunt.down } },
				{ Direction::Down, { -hunt.across, hunt.down } }, { Direction::Down, { hunt.across, hunt.down } } } }
			: std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { 0.0f, -hunt.down } }, { Direction::Left, { -hunt.across, 0.0f } },
				{ Direction::Down, { 0.0f, hunt.down } }, { Direction::Right, { hunt.across, 0.0f } } } };

		const WindowDesc& window = executor.game.getWindowDesc();
		const Vector2f there = quarry->position + sizeOf(*quarry) * 0.5f;
		const Vector2f middle = object.position + sizeOf(object) * 0.5f;
		std::optional<std::pair<Direction, Vector2f>> best;
		float bestDistance = 0.0f;
		for (const auto& [way, step] : hops)
		{
			if (!insideWindow({ object.position + step, sizeOf(object) }, { window.width, window.height })) { continue; }
			const Vector2f off = middle + step - there;
			const float distance = off.x * off.x + off.y * off.y;
			if (!best || distance < bestDistance)
			{
				best = std::make_pair(way, step);
				bestDistance = distance;
			}
		}
		if (!best) { return false; }

		return executor.hop(object, CmdHop{ best->first, 0.0f, best->second, hunt.seconds, hunt.arc, {} });
	}

	static bool runHunt(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (!doer.self) { return true; }
		if (const auto* byHops = std::get_if<CmdHuntHop>(&command)) { return huntByHops(executor, *doer.self, *byHops); }
		startHunt(*doer.self, std::get<CmdHunt>(command));
		return true;
	}

	static void printHunt(std::ostream& o, const Command& command)
	{
		if (const auto* ph = std::get_if<CmdHunt>(&command))
		{
			const CmdHunt& h = *ph;
			o << "hunt(" << h.target << ", " << h.speed;
			if (h.ahead != 0.0f) { o << ", ahead " << h.ahead; }
			if (!h.partner.empty()) { o << ", with " << h.partner; }
			if (!h.retreatTo.empty()) { o << ", retreat " << h.retreatTo << " within " << h.retreatWithin; }
			if (h.search) { o << ", search"; }
			o << ")";
			return;
		}
		if (const auto* ph = std::get_if<CmdHuntHop>(&command))
		{
			const CmdHuntHop& h = *ph;
			o << "hunt(" << h.target << ", by " << (h.diagonal ? "diagonal " : "") << "hops of " << h.across << ", " << h.down
				<< (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
				<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{}) << ")";
			return;
		}
	}

	static void printHuntRaw(std::ostream& o, const RawCommand& command)
	{
		if (command.byHops)
		{
			o << (command.byHops ? "(" + command.object + ", by hops of " : "." + command.direction + "(") << command.amount << ", " << command.positionX << " " << command.positionY
				<< ", " << command.seconds << ")";
		}
		else
		{
			o << "(" << command.object << ", " << command.amount << ", ahead " << command.ahead << ", with " << command.partner
				<< ", retreat " << command.retreat << " " << command.distance << (command.search ? ", search" : "") << ")";
		}
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
		.print = printHunt,
		.printRaw = printHuntRaw,
	};
}
