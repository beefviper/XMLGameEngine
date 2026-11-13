// fire.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <cmath>
#include <optional>
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

	// Launches a copy of a piece (Game::spawn), refused when its <most> are
	// in flight. From a shooter with a heading the shot leaves its nose the
	// way it faces, at the projectile's own speed; from any other it leaves
	// the top centre, at the projectile's own velocity.
	static bool fireProjectile(CommandExecutor& executor, Object& shooter, const std::string& projectileName)
	{
		Object* projectile = executor.game.spawn(projectileName);
		if (!projectile) { return false; }

		const Box from{ shooter.position, sizeOf(shooter) };
		const Vector2f shotSize = sizeOf(*projectile);
		const float speed = std::hypot(projectile->start.velocity.x, projectile->start.velocity.y);
		std::optional<Shot> shot;
		if (shooter.hasAim)
		{
			// Straight along the aim, from the middle of the shooter, just clear
			// of it, at the projectile's own speed.
			shot = shotAlong(from, shotSize, shooter.aim, speed);
		}
		else if (shooter.hasHeading)
		{
			shot = shotAhead(from, shotSize, shooter.heading, speed);
		}
		else if (shooter.hasFacing)
		{
			// From the middle of the side it faces, just clear of it, moving that
			// way at the projectile's own speed.
			shot = shotFacing(from, shotSize, shooter.facing, speed);
		}
		else
		{
			// From the shooter's top-centre: the projectile's own middle over
			// the shooter's, so a thin shot leaves the middle of the gun.
			shot = Shot{ shotFromTop(from, shotSize), projectile->start.velocity };
		}

		projectile->position = shot->position;
		projectile->velocity = shot->velocity;
		projectile->isVisible = true;
		projectile->collisionsOn = true;
		return true;
	}

	static bool runFire(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || fireProjectile(executor, *doer.self, std::get<CmdFire>(command).projectileName);
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
