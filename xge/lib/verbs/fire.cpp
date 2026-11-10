// fire.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

namespace xge
{
	// <fire>: Sends a shot from where it is; refused when every shot is in
	// flight.
	static void readFire(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "object", where);
	}

	static Command makeFire(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdFire{ raw.object };
	}

	static void checkFire(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdFire>(command).projectileName, "<fire>");
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
		.read = readFire,
		.make = makeFire,
		.check = checkFire,
		.run = runFire,
	};
}
