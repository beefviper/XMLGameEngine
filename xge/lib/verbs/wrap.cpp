// wrap.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <wrap>: Out of one edge of the window and in at the other.
	static Command makeWrap(const RawCommand&, const ValueEvaluator&)
	{
		return CmdWrap{};
	}

	// <wrap />: once right off through `edge`, back in from the other side.
	//
	// Until it has gone completely, nothing happens: an object still partly
	// in view slides out as normal. It comes back one screen and one object's
	// width along, so anything spaced along a lane keeps its spacing.
	static void wrap(Object& object, const WindowDesc& window, Edge edge)
	{
		wrapAround(object.position, object.size, object.velocity + object.riding, { window.width, window.height }, edge);
	}

	static bool runWrap(const CmdWrap&, const Doer& doer)
	{
		if (doer.self && doer.screenEdge) { xge::wrap(*doer.self, doer.game().getWindowDesc(), *doer.screenEdge); }
		return true;
	}

	static void printWrap(std::ostream& o, const CmdWrap&)
	{
		o << "wrap";
	}

	extern const Verb wrapVerb;
	const Verb wrapVerb{
		.name = "wrap",
		.forms = { { "<wrap />", places::edge, kindOf<CmdWrap>() } },
		.make = makeWrap,
		.run = runAs<runWrap>,
		.print = printAs<printWrap>,
	};
}
