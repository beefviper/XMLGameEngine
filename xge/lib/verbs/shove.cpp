// shove.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <shove>: Pushes the piece in its way one cell on.
	static Command makeShove(const RawCommand&, const ValueEvaluator&)
	{
		return CmdShove{};
	}

	static bool runShove(const CmdShove&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		return doer.self && doer.touch && doer.other && executor.game.shoveCells(*doer.other, doer.touch->edge);
	}

	static void printShove(std::ostream& o, const CmdShove&)
	{
		o << "shove";
	}

	extern const Verb shoveVerb;
	const Verb shoveVerb{
		.name = "shove",
		.forms = { { "<shove />", places::touch, kindOf<CmdShove>() } },
		.make = makeShove,
		.run = runAs<runShove>,
		.print = printAs<printShove>,
	};
}
