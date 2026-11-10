// put.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <put>: A new piece on the cell it is over.
	static Command makePut(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPut{ raw.objClass, raw.closeClass, raw.closeObject };
	}

	static bool runPut(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return doer.self && executor.game.putPiece(*doer.self, std::get<CmdPut>(command));
	}

	extern const Verb putVerb;
	const Verb putVerb{
		.name = "put",
		.forms = { { "<put>", places::self, kindOf<CmdPut>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makePut,
		.run = runPut,
	};
}
