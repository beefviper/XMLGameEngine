// place.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

namespace xge
{
	// <place>: Sets down what it picked, where it is.
	static Command makePlace(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdPlace{ written.into };
	}

	static bool runPlace(const CmdPlace& place, const Doer& doer)
	{
		Game& game = doer.game();
		Object* const self = doer.self;
		return self && (game.isHand(*self) ? game.holders().layItems(*self, place.into) : game.cellMaps().placePiece(*self));
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
