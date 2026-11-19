// despawn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"

#include <string>

namespace xge
{
	// <despawn>: Takes a copy away for good: the one running it, or those
	// named.
	static Command makeDespawn(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdDespawn{ raw.object };
	}

	static void checkDespawn(const CmdDespawn& despawn, const Checking& checking)
	{
		const std::string& target = despawn.target;
		if (!target.empty()) { checkObject(checking, target, "<despawn>", "object or group"); }
	}

	static bool runDespawn(const CmdDespawn& despawnCommand, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		executor.despawn(doer.self, despawnCommand);
		return true;
	}

	// Bare, it is about the object running it; with object=, about those named.
	static std::size_t formOfDespawn(const Command& command)
	{
		return std::get<CmdDespawn>(command).target.empty() ? 0 : 1;
	}

	static void printDespawn(std::ostream& o, const CmdDespawn& despawn)
	{
		o << "despawn" << (despawn.target.empty() ? "" : "(" + despawn.target + ")");
	}

	extern const Verb despawnVerb;
	const Verb despawnVerb{
		.name = "despawn",
		.forms = { { "<despawn />", places::self, kindOf<CmdDespawn>() }, { "<despawn object>", places::changes, kindOf<CmdDespawn>() } },
		.formOf = formOfDespawn,
		.make = makeDespawn,
		.check = checkAs<checkDespawn>,
		.run = runAs<runDespawn>,
		.print = printAs<printDespawn>,
	};
}
