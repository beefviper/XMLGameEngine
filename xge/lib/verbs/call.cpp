// call.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <stdexcept>

namespace xge
{
	// <call>: Names what a wild card stands for.
	static Command makeCall(const RawCommand& raw, const ValueEvaluator&)
	{
		if (raw.pile.empty()) { throw std::runtime_error("<call>: needs pile=, the pile whose wild card the suit is called for"); }
		return CmdCall{ raw.pile };
	}

	static bool runCall(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return doer.self && executor.game.callSuit(*doer.self, std::get<CmdCall>(command).pile);
	}

	extern const Verb callVerb;
	const Verb callVerb{
		.name = "call",
		.forms = { { "<call />", places::self, kindOf<CmdCall>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeCall,
		.run = runCall,
	};
}
