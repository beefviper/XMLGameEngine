// turn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
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

	// A turn is now held (rate 0 = just released);
	// playOneFrame turns the object once a frame while it is.
	static void holdTurn(Game& game, Object& object, Direction direction, float rate)
	{
		game.driveFor(object).turn[static_cast<std::size_t>(direction)] = rate;
	}

	static bool runTurn(const CmdTurn& turn, const Doer& doer)
	{
		if (!doer.self || !doer.action) { return true; }
		Game& game = doer.game();
		// A shape on a cell map: a quarter turn on the press.
		if (game.isShape(*doer.self)) { return !doer.pressed || game.turnCells(*doer.self, turn.direction == Direction::Right); }
		holdTurn(game, *doer.self, turn.direction, doer.pressed ? turn.rate : 0.0f);
		return true;
	}

	static bool keepTurning(const CmdTurn& turn, const Doer& doer)
	{
		if (!doer.self) { return false; }
		holdTurn(doer.game(), *doer.self, turn.direction, turn.rate);
		return true;
	}

	static void printTurn(std::ostream& o, const CmdTurn& turn)
	{
		o << "turn." << (turn.direction == Direction::Left ? "counterclockwise" : "clockwise") << "(" << turn.rate << ")";
	}

	extern const Verb turnVerb;
	const Verb turnVerb{
		.name = "turn",
		.forms = { { "<turn>", places::controls, kindOf<CmdTurn>() } },
		.onRelease = true,
		.read = readTurn,
		.make = makeTurn,
		.run = runAs<runTurn>,
		.hold = runAs<keepTurning>,
		.print = printAs<printTurn>,
	};
}
