// wrap.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"

namespace xge
{
	// <wrap>: Out of one edge of the window and in at the other.
	static Command makeWrap(const RawCommand&, const ValueEvaluator&)
	{
		return CmdWrap{};
	}

	static bool runWrap(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && doer.screenEdge) { xge::wrap(*doer.self, executor.game.getWindowDesc(), *doer.screenEdge); }
		return true;
	}

	extern const Verb wrapVerb;
	const Verb wrapVerb{
		.name = "wrap",
		.forms = { { "<wrap />", places::edge, kindOf<CmdWrap>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeWrap,
		.check = nullptr,
		.run = runWrap,
	};
}
