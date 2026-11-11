// cycle.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <stdexcept>
#include <string>
#include <variant>

namespace xge
{
	// <cycle>: The cells of a shape change places in turn, each to the next
	// place, or with direction="backward" each to the one before.
	static Command makeCycle(const RawCommand& raw, const ValueEvaluator&)
	{
		if (raw.direction.empty() || raw.direction == "forward") { return CmdCycle{}; }
		if (raw.direction == "backward") { return CmdCycle{ true }; }
		throw std::runtime_error("<cycle> has direction=\"" + raw.direction + "\"; expected forward or backward");
	}

	static bool runCycle(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self) { executor.game.cycleCells(*doer.self, std::get<CmdCycle>(command).backward); }
		return true;
	}

	extern const Verb cycleVerb;
	const Verb cycleVerb{
		.name = "cycle",
		.forms = { { "<cycle />", places::self, kindOf<CmdCycle>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeCycle,
		.check = nullptr,
		.run = runCycle,
	};
}
