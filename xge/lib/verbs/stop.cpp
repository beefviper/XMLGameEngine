// stop.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <stop>: Stands still.
	static Command makeStop(const RawCommand&, const ValueEvaluator&)
	{
		return CmdStop{};
	}

	// <stop />: at rest, with nothing left to move it (what its keys drive
	// is let go by the caller: Game::stopDrive).
	//
	// Neither its own pull nor a thruster still held is left (a reset gives
	// them back). Being in motion is what makes collisions run, so an object
	// that has stopped is no longer asked about what it is resting on.
	static void stop(Object& object)
	{
		object.velocity = {};
		object.acceleration = {};
		object.footing.climbing = false;
		object.footing.jumping = false;
		object.path.name.clear();
	}

	static bool runStop(const CmdStop&, const Doer& doer)
	{
		Game& game = doer.game();
		if (doer.self)
		{
			xge::stop(*doer.self);
			if (Object::Climb* climb = game.parts().climbs.find(doer.self->id)) { climb->activeClimb = {}; }
			game.stopDrive(*doer.self);
			if (Maze* maze = game.mazeOf(*doer.self)) { maze->walk = {}; }
		}
		return true;
	}

	static void printStop(std::ostream& o, const CmdStop&)
	{
		o << "stop";
	}

	extern const Verb stopVerb;
	const Verb stopVerb{
		.name = "stop",
		.forms = { { "<stop />", places::self, kindOf<CmdStop>() } },
		.make = makeStop,
		.run = runAs<runStop>,
		.print = printAs<printStop>,
	};
}
