// turn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

namespace xge
{
	// <turn>: Round while held; a quarter turn for a shape of cells.
	static void readTurn(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// On a cell map a quarter turn, with nothing written.
		requireAttribute(node, "direction", where);
		if (hasContent(node)) { command.amount = readValue(node, where); }
	}

	static Command makeTurn(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// Nothing written: none (a turn on a cell map, a quarter).
		return CmdTurn{ rotationFromName(raw.direction, raw.verb), valueOr(raw.amount, 0.0f, evaluate) };
	}

	static bool runTurn(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdTurn& turn = std::get<CmdTurn>(command);
		if (!doer.self || !doer.action) { return true; }
		// A shape on a cell map: a quarter turn on the press.
		if (doer.self->isShape()) { return !doer.pressed || executor.game.turnCells(*doer.self, turn.direction == Direction::Right); }
		executor.applyActionTurn(*doer.self, turn.direction, doer.pressed ? turn.rate : 0.0f);
		return true;
	}

	static void printTurn(std::ostream& o, const Command& command)
	{
		const CmdTurn& t = std::get<CmdTurn>(command);
		o << "turn." << (t.direction == Direction::Left ? "counterclockwise" : "clockwise") << "(" << t.rate << ")";
	}

	static void printTurnRaw(std::ostream& o, const RawCommand& command)
	{
		o << "." << command.direction << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
	}

	extern const Verb turnVerb;
	const Verb turnVerb{
		.name = "turn",
		.forms = { { "<turn>", places::controls, kindOf<CmdTurn>() } },
		.formOf = nullptr,
		.onRelease = true,
		.read = readTurn,
		.make = makeTurn,
		.check = nullptr,
		.run = runTurn,
		.print = printTurn,
		.printRaw = printTurnRaw,
	};
}
