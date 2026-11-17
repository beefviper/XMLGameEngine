// stop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

namespace xge
{
	// <stop>: Stands still.
	static Command makeStop(const RawCommand&, const ValueEvaluator&)
	{
		return CmdStop{};
	}

	static bool runStop(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self)
		{
			xge::stop(*doer.self);
			if (Object::Climb* climb = executor.game.parts().climbs.find(doer.self->id)) { climb->activeClimb = {}; }
			executor.game.stopDrive(*doer.self);
			if (Maze* maze = executor.game.mazeOf(*doer.self)) { maze->walk = {}; }
		}
		return true;
	}

	static void printStop(std::ostream& o, const Command&)
	{
		o << "stop";
	}

	extern const Verb stopVerb;
	const Verb stopVerb{
		.name = "stop",
		.forms = { { "<stop />", places::self, kindOf<CmdStop>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeStop,
		.check = nullptr,
		.run = runStop,
		.print = printStop,
	};
}
