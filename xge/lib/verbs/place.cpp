// place.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <place>: Sets down what it picked, where it is.
	static Command makePlace(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPlace{ raw.into };
	}

	static bool runPlace(const CmdPlace& place, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		Object* const self = doer.self;
		return self && (executor.game.isHand(*self) ? executor.game.layItems(*self, place.into) : executor.game.placePiece(*self));
	}

	static void printPlace(std::ostream& o, const CmdPlace& place)
	{
		o << "place" << (place.into.empty() ? "" : "(" + place.into + ")");
	}

	extern const Verb placeVerb;
	const Verb placeVerb{
		.name = "place",
		.forms = { { "<place />", places::self, kindOf<CmdPlace>() } },
		.make = makePlace,
		.run = runAs<runPlace>,
		.print = printAs<printPlace>,
	};
}
