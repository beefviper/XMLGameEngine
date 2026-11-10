// despawn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <despawn>: Takes a copy away for good: the one running it, or those
	// named.
	static Command makeDespawn(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdDespawn{ raw.object };
	}

	static bool runDespawn(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		executor.despawn(doer.self, std::get<CmdDespawn>(command));
		return true;
	}

	// Bare, it is about the object running it; with object=, about those named.
	static std::size_t formOfDespawn(const Command& command)
	{
		return std::get<CmdDespawn>(command).target.empty() ? 0 : 1;
	}

	extern const Verb despawnVerb;
	const Verb despawnVerb{
		.name = "despawn",
		.forms = { { "<despawn />", places::self, kindOf<CmdDespawn>() }, { "<despawn object>", places::changes, kindOf<CmdDespawn>() } },
		.formOf = formOfDespawn,
		.onRelease = false,
		.make = makeDespawn,
		.run = runDespawn,
	};
}
