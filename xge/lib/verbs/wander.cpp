// wander.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "play/maze.h"
#include "load/reading.h"

namespace xge
{
	// <wander>: Through a maze, any way at each corner.
	static void readWander(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.amount = readValue(node, where);
	}

	static Command makeWander(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdWander{ speedOf(raw, "wander", evaluate) };
	}

	static bool runWander(const Command& command, const Doer& doer, CommandExecutor&)
	{
		if (doer.self) { startWander(*doer.self, std::get<CmdWander>(command)); }
		return true;
	}

	static void printWander(std::ostream& o, const Command& command)
	{
		const CmdWander& w = std::get<CmdWander>(command);
		o << "wander(" << w.speed << ")";
	}

	static void printWanderRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.amount << ")";
	}

	extern const Verb wanderVerb;
	const Verb wanderVerb{
		.name = "wander",
		.forms = { { "<wander>", places::steering, kindOf<CmdWander>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readWander,
		.make = makeWander,
		.check = nullptr,
		.run = runWander,
		.print = printWander,
		.printRaw = printWanderRaw,
	};
}
