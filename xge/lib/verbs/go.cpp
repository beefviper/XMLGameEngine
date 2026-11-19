// go.cpp
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
	// <go>: Sets off along a maze's corridors, or cell by cell.
	static void readGo(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		command.own = RawGo{ readValue(node, where) };
	}

	static Command makeGo(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdGo{ directionFromName(raw.direction, raw.verb), speedOf(ownOf<RawGo>(raw).speed, "go", evaluate) };
	}

	static bool runGo(const Command& command, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		const CmdGo& go = std::get<CmdGo>(command);
		if (doer.self && doer.self->onCells) { return executor.game.goCells(*doer.self, go.direction, go.speed); }
		if (doer.self && doer.action) { if (Maze* maze = executor.game.mazeOf(*doer.self)) { startGo(*maze, go); } }
		return true;
	}

	static void printGo(std::ostream& o, const Command& command)
	{
		const CmdGo& g = std::get<CmdGo>(command);
		const char* direction = (g.direction == Direction::Up) ? "up"
			: (g.direction == Direction::Down) ? "down"
			: (g.direction == Direction::Left) ? "left"
			: "right";
		o << "go." << direction << "(" << g.speed << ")";
	}

	extern const Verb goVerb;
	const Verb goVerb{
		.name = "go",
		.forms = { { "<go>", places::controls, kindOf<CmdGo>() } },
		.read = readGo,
		.make = makeGo,
		.run = runGo,
		.print = printGo,
	};
}
