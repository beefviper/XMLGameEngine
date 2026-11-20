// go.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/maze.h"
#include "load/reading.h"
#include <variant>

namespace xge
{
	// <go>: Sets off along a maze's corridors, or cell by cell.
	static void readGo(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		requireAttribute(node, "direction", where);
		command.own = WrittenGo{ readValue(node, where) };
	}

	static Command makeGo(const WrittenCommand& raw, const ValueEvaluator& evaluate)
	{
		return CmdGo{ directionFromName(raw.direction, raw.verb), speedOf(ownOf<WrittenGo>(raw).speed, "go", evaluate) };
	}

	static bool runGo(const CmdGo& go, const Doer& doer)
	{
		Game& game = doer.game();
		if (doer.self && doer.self->onCells) { return game.goCells(*doer.self, go.direction, go.speed); }
		if (doer.self && doer.action) { if (Maze* maze = game.mazeOf(*doer.self)) { startGo(*maze, go); } }
		return true;
	}

	// The way a maze walker is asked to go, while the key asking is still
	// down: Pac-Man held right through READY! sets off right.
	static bool keepGoing(const CmdGo& go, const Doer& doer)
	{
		if (!doer.self || doer.self->onCells) { return false; }
		Maze* maze = doer.game().mazeOf(*doer.self);
		if (!maze) { return false; }
		startGo(*maze, go);
		return true;
	}

	static void printGo(std::ostream& o, const CmdGo& go)
	{
		const char* direction = (go.direction == Direction::Up) ? "up"
			: (go.direction == Direction::Down) ? "down"
			: (go.direction == Direction::Left) ? "left"
			: "right";
		o << "go." << direction << "(" << go.speed << ")";
	}

	extern const Verb goVerb;
	const Verb goVerb{
		.name = "go",
		.forms = { { "<go>", places::controls, kindOf<CmdGo>() } },
		.read = readGo,
		.make = makeGo,
		.run = runAs<runGo>,
		.hold = runAs<keepGoing>,
		.print = printAs<printGo>,
	};
}
