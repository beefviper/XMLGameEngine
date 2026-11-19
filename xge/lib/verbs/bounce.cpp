// bounce.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <stdexcept>
#include <variant>

namespace xge
{
	// <bounce>: Off what it touched, or off the window's edge, keeping a share
	// of its speed.
	static void readBounce(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		RawBounce& bounce = command.own.emplace<RawBounce>();
		if (hasContent(node)) { bounce.share = readValue(node, where); }
	}

	static Command makeBounce(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const RawBounce& bounce = ownOf<RawBounce>(raw);
		const float share = bounce.share ? evaluate(*bounce.share) : 1.0f;
		if (!(share >= 0.0f))
		{
			throw std::runtime_error("<bounce> keeps a share of " + formatDisplayNumber(share) + " of its speed; expected 0 or more");
		}
		return CmdBounce{ share };
	}

	static bool runBounce(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdBounce& bounce = std::get<CmdBounce>(command);
		if (doer.self && doer.touch && doer.other) { xge::bounce(executor.game.parts(), *doer.self, *doer.other, *doer.touch, bounce.share); }
		else if (doer.self && doer.screenEdge) { executor.bounceScreenEdge(*doer.self, *doer.screenEdge, bounce.share); }
		return true;
	}

	static void printBounce(std::ostream& o, const Command& command)
	{
		const CmdBounce& b = std::get<CmdBounce>(command);
		o << "bounce(" << b.share << ")";
	}

	static void printBounceRaw(std::ostream& o, const RawCommand& command)
	{
		const RawBounce& bounce = ownOf<RawBounce>(command);
		if (bounce.share) { o << "(" << *bounce.share << ")"; }
	}

	extern const Verb bounceVerb;
	const Verb bounceVerb{
		.name = "bounce",
		.forms = { { "<bounce />", places::edge | places::touch, kindOf<CmdBounce>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readBounce,
		.make = makeBounce,
		.check = nullptr,
		.run = runBounce,
		.print = printBounce,
		.printRaw = printBounceRaw,
	};
}
