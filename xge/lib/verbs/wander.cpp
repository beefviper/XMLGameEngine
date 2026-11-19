// wander.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/maze.h"
#include "load/reading.h"
#include <variant>

namespace xge
{
	// <wander>: Through a maze, any way at each corner.
	static void readWander(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.own = RawWander{ readValue(node, where) };
	}

	static Command makeWander(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdWander{ speedOf(ownOf<RawWander>(raw).speed, "wander", evaluate) };
	}

	static bool runWander(const Command& command, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self) { if (Maze* maze = executor.game.mazeOf(*doer.self)) { startWander(*maze, std::get<CmdWander>(command)); } }
		return true;
	}

	static void printWander(std::ostream& o, const Command& command)
	{
		const CmdWander& w = std::get<CmdWander>(command);
		o << "wander(" << w.speed << ")";
	}

	extern const Verb wanderVerb;
	const Verb wanderVerb{
		.name = "wander",
		.forms = { { "<wander>", places::steering, kindOf<CmdWander>() } },
		.read = readWander,
		.make = makeWander,
		.run = runWander,
		.print = printWander,
	};
}
