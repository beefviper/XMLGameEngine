// distribute.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

namespace xge
{
	// <distribute>: What one holder holds, to others. It makes a <send>'s
	// command, which runs it.
	static Command makeDistribute(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		return makeSendHeld(raw, evaluate);
	}

	extern const Verb distributeVerb;
	const Verb distributeVerb{
		.name = "distribute",
		.forms = { { "<distribute>", places::changes, kindOf<CmdSend>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeDistribute,
		.check = nullptr,
		.run = nullptr,
	};
}
