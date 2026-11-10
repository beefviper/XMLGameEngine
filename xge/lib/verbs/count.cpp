// count.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <count>: How many of a class there are, or lie about it, into a
	// variable.
	static Command makeCount(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdCount{ raw.objClass, VariableName::of(raw.into), cellPlaceOf(raw) };
	}

	static bool runCount(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdCount& count = std::get<CmdCount>(command);
		if (!count.place.any()) { executor.game.setNumber(count.target, executor.game.countClass(count.objClass)); }
		else if (doer.self) { executor.game.setNumber(count.target, static_cast<float>(executor.game.piecesAbout(*doer.self, count.objClass, count.place).size())); }
		return true;
	}

	extern const Verb countVerb;
	const Verb countVerb{
		.name = "count",
		.forms = { { "<count>", places::changes, kindOf<CmdCount>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCount,
		.check = nullptr,
		.run = runCount,
	};
}
