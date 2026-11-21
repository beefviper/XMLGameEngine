// stick.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/geometry.h"

namespace xge
{
	// <stick>: Stops at the window's edge, or against what it touched.
	static Command makeStick(const WrittenCommand&, const ValueEvaluator&)
	{
		return CmdStick{};
	}

	// <stick /> in a rule about another object: only the velocity into
	// the side it was touched on stops, so it slides along what it met.
	static bool runStick(const CmdStick&, const Doer& doer)
	{
		if (doer.atEdge())
		{
			const WindowDesc& window = doer.game().getWindowDesc();
			stickInside(doer.self->position, doer.self->velocity, doer.self->size, { window.width, window.height }, *doer.screenEdge);
		}
		else if (doer.self && doer.touch) { stopGoingOut(doer.self->velocity, doer.touch->edge); }
		return true;
	}

	static void printStick(std::ostream& o, const CmdStick&)
	{
		o << "stick";
	}

	extern const Verb stickVerb;
	const Verb stickVerb{
		.name = "stick",
		.forms = { { "<stick />", places::edge | places::touch, kindOf<CmdStick>() } },
		.make = makeStick,
		.run = runAs<runStick>,
		.print = printAs<printStick>,
	};
}
