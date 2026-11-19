// stick.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"

namespace xge
{
	// <stick>: Stops at the window's edge, or against what it touched.
	static Command makeStick(const RawCommand&, const ValueEvaluator&)
	{
		return CmdStick{};
	}

	// <stick /> in a rule about another object: only the velocity into
	// the side it was touched on stops, so it slides along what it met.
	static void stickAgainst(Object& object, Edge side)
	{
		Vector2f& velocity = object.velocity;
		if ((side == Edge::Top && velocity.y < 0.0f) || (side == Edge::Bottom && velocity.y > 0.0f)) { velocity.y = 0.0f; }
		if ((side == Edge::Left && velocity.x < 0.0f) || (side == Edge::Right && velocity.x > 0.0f)) { velocity.x = 0.0f; }
	}

	static bool runStick(const CmdStick&, const Doer& doer)
	{
		if (doer.self && doer.screenEdge) { xge::stick(*doer.self, doer.game().getWindowDesc(), *doer.screenEdge); }
		else if (doer.self && doer.touch) { stickAgainst(*doer.self, doer.touch->edge); }
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
