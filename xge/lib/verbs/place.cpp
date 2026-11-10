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

	static bool runPlace(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdPlace& place = std::get<CmdPlace>(command);
		Object* const self = doer.self;
		return self && (self->hand.is ? executor.game.layCards(*self, place.into) : executor.game.placePiece(*self));
	}

	extern const Verb placeVerb;
	const Verb placeVerb{
		.name = "place",
		.forms = { { "<place />", places::self, kindOf<CmdPlace>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makePlace,
		.check = nullptr,
		.run = runPlace,
	};
}
