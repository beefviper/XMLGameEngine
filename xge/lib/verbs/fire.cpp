// fire.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <fire>: Sends a shot from where it is; refused when every shot is in
	// flight.
	static Command makeFire(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdFire{ raw.object };
	}

	static bool runFire(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || executor.spawnProjectile(*doer.self, std::get<CmdFire>(command).projectileName);
	}

	extern const Verb fireVerb;
	const Verb fireVerb{
		.name = "fire",
		.forms = { { "<fire>", places::self, kindOf<CmdFire>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeFire,
		.run = runFire,
	};
}
