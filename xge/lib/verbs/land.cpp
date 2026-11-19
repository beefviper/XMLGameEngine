// land.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <cmath>

namespace xge
{
	// <land>: Comes to rest on what it fell onto.
	static Command makeLand(const RawCommand&, const ValueEvaluator&)
	{
		return CmdLand{};
	}

	// Coming down onto the top of `other` (its own bottom edge touched, not
	// going up): it stands there. Put on the top, its fall stopped, on the
	// ground until the next frame's landing is worked out; a jump ends, and
	// the keys held now decide the way across. Any other touch does nothing,
	// so a platform can be jumped up through and walked past, and nothing
	// lands while it climbs.
	static void landOnto(Object& object, const Drive* drive, const Object& other, Edge edge)
	{
		if (!landOn(object.position, object.velocity, sizeOf(object), { other.position, sizeOf(other) }, edge, object.footing.climbing))
		{
			return;
		}

		object.footing.grounded = true;

		if (object.footing.jumping)
		{
			object.footing.jumping = false;
			// One with a <grip> that is not at once slides on from the speed it landed at.
			if (std::isinf(object.motion.grip))
			{
				object.velocity.x = drive ? drive->moveStep[static_cast<std::size_t>(Direction::Right)]
					- drive->moveStep[static_cast<std::size_t>(Direction::Left)] : 0.0f;
			}
		}
	}

	static bool runLand(const Command&, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self && doer.touch) { landOnto(*doer.self, executor.game.parts().drives.find(doer.self->id), *doer.other, doer.touch->edge); }
		return true;
	}

	static void printLand(std::ostream& o, const Command&)
	{
		o << "land()";
	}

	extern const Verb landVerb;
	const Verb landVerb{
		.name = "land",
		.forms = { { "<land />", places::touch, kindOf<CmdLand>() } },
		.make = makeLand,
		.run = runLand,
		.print = printLand,
	};
}
