// go.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/maze.h"
#include "load/reading.h"

namespace xge
{
	// <go>: Sets off along a maze's corridors, or cell by cell.
	static void readGo(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		command.amount = readValue(node, where);
	}

	static Command makeGo(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdGo{ directionFromName(raw.direction, raw.verb), speedOf(raw, "go", evaluate) };
	}

	static bool runGo(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdGo& go = std::get<CmdGo>(command);
		if (doer.self && doer.self->onCells) { return executor.game.goCells(*doer.self, go.direction, go.speed); }
		if (doer.self && doer.action) { startGo(*doer.self, go); }
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

	static void printGoRaw(std::ostream& o, const RawCommand& command)
	{
		o << "." << command.direction << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
	}

	extern const Verb goVerb;
	const Verb goVerb{
		.name = "go",
		.forms = { { "<go>", places::controls, kindOf<CmdGo>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readGo,
		.make = makeGo,
		.check = nullptr,
		.run = runGo,
		.print = printGo,
		.printRaw = printGoRaw,
	};
}
