// chase.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <chase>: Heads straight for another object.
	static void readChase(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Its speed as its content, then how near is near enough.
		requireAttribute(node, "object", where);
		const std::string here = where + " > <chase>";
		command.amount = readLeadValue(node, { "near" }, "its speed, then <near> if wanted", where);
		if (auto nearNode = findChild(&node, "near")) { command.distance = readValue(*nearNode, here); }
	}

	static Command makeChase(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float speed = evaluate(raw.amount);
		const float stopWithin = valueOr(raw.distance, 0.0f, evaluate);
		if (speed < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: its speed is below 0"); }
		if (stopWithin < 0.0f) { throw std::runtime_error("<chase object=\"" + raw.object + "\">: the <near> is below 0"); }
		return CmdChase{ raw.object, speed, stopWithin };
	}

	static void checkChase(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdChase>(command).target, "<chase>");
	}

	static bool runChase(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self) { executor.chase(*doer.self, std::get<CmdChase>(command)); }
		return true;
	}

	static void printChase(std::ostream& o, const Command& command)
	{
		const CmdChase& c = std::get<CmdChase>(command);
		o << "chase(" << c.target << ", " << c.speed << (c.stopWithin > 0.0f ? ", near " + formatDisplayNumber(c.stopWithin) : std::string{}) << ")";
	}

	static void printChaseRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.object << ", " << command.amount << ", " << command.distance << ")";
	}

	extern const Verb chaseVerb;
	const Verb chaseVerb{
		.name = "chase",
		.forms = { { "<chase>", places::steering, kindOf<CmdChase>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readChase,
		.make = makeChase,
		.check = checkChase,
		.run = runChase,
		.print = printChase,
		.printRaw = printChaseRaw,
	};
}
