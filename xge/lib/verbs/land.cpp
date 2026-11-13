// land.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/geometry.h"
#include "play/verbs.h"

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
	static void landOnto(Object& object, const Object& other, Edge edge)
	{
		if (!landOn(object.position, object.velocity, sizeOf(object), { other.position, sizeOf(other) }, edge, object.climbing))
		{
			return;
		}

		object.grounded = true;

		if (object.jumping)
		{
			object.jumping = false;
			// One with a <grip> below 1 slides on from the speed it landed at.
			if (object.grip >= 1.0f)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
	}

	static bool runLand(const Command&, const Doer& doer, CommandExecutor&)
	{
		if (doer.self && doer.touch) { landOnto(*doer.self, *doer.other, doer.touch->edge); }
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
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeLand,
		.check = nullptr,
		.run = runLand,
		.print = printLand,
	};
}
