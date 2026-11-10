// think.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "engine/keys.h"

#include <cmath>
#include <stdexcept>

namespace xge
{
	// <think>: A <computer> looks ahead for its best move: run by Game
	// (game/think.cpp).
	static Command makeThink(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (raw.object.empty()) { throw std::runtime_error("<think>: names the object it plays with object= (a <think> on a member of a group that takes <turns> plays that member)"); }
		const KeyCode button = keyCodeFromString(raw.button);
		if (button == KeyCode::Unknown)
		{
			throw std::runtime_error("<think button=\"" + raw.button + "\">: there is no key named '" + raw.button + "'");
		}
		const float depth = valueOr(raw.amount, 4.0f, evaluate);
		if (!(depth >= 1.0f && depth <= 9.0f) || depth != std::floor(depth))
		{
			throw std::runtime_error("<think> looks " + formatDisplayNumber(depth) + " moves ahead; expected a whole number from 1 to 9");
		}
		return CmdThink{ raw.objClass, raw.object, button, static_cast<int>(depth), raw.worths, raw.avoid };
	}

	extern const Verb thinkVerb;
	const Verb thinkVerb{
		.name = "think",
		.forms = { { "<think>", places::press, kindOf<CmdThink>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeThink,
		.run = nullptr,
	};
}
