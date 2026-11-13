// fire.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <fire>: Sends a copy of a piece from where it is; refused when its
	// <most> are in flight.
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
		const std::string& shot = std::get<CmdFire>(command).projectileName;
		checkObject(checking, shot, "<fire>");
		// What it sends is a copy of a piece, never an object already there.
		for (const Object& object : checking.objects)
		{
			if (object.named(shot) && object.start.exists)
			{
				throw std::runtime_error(checking.where + ": <fire object=\"" + shot + "\" />: '" + object.name + "' is in the game from the start; <fire> sends a copy of a piece (an object of the top-level <objects>), so write the shot there, with a <most> for how many may fly at once");
			}
		}
	}

	static bool runFire(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || executor.spawnProjectile(*doer.self, std::get<CmdFire>(command).projectileName);
	}

	static void printFire(std::ostream& o, const Command& command)
	{
		const CmdFire& f = std::get<CmdFire>(command);
		o << "fire(" << f.projectileName << ")";
	}

	static void printFireRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.object << ")";
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
		.print = printFire,
		.printRaw = printFireRaw,
	};
}
