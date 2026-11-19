// bounce.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/geometry.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <optional>
#include <stdexcept>

namespace xge
{
	// <bounce>: Off what it touched, or off the window's edge, keeping a share
	// of its speed.
	static void readBounce(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		RawBounce& bounce = command.own.emplace<RawBounce>();
		if (hasContent(node)) { bounce.share = readValue(node, where); }
	}

	static Command makeBounce(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const RawBounce& bounce = ownOf<RawBounce>(raw);
		const float share = bounce.share ? evaluate(*bounce.share) : 1.0f;
		if (!(share >= 0.0f))
		{
			throw std::runtime_error("<bounce> keeps a share of " + formatDisplayNumber(share) + " of its speed; expected 0 or more");
		}
		return CmdBounce{ share };
	}

	// Off what it touched, as off a surface standing still: mirrored about
	// the touch's normal, keeping `share` of the speed it came in at; one
	// already going away keeps on. A box's side faces straight out, so off a
	// box this is the plain flip. With <friction> it rubs along the surface
	// and may come to rest on it. (A surface that moves and lends its speed
	// is a <knock>.)
	static void bounceOff(Game& game, Object& object, const Object& other, const Touch& touch, float share)
	{
		const Vector2f way = touch.normal * -1.0f;
		const float settle = settleSpeed(object.acceleration, way);
		const Vector2f was = object.velocity;
		if (const std::optional<Knocked> after = knockedApart(object.velocity, 1.0f, Vector2f{}, 0.0f, way, share, settle))
		{
			object.velocity = after->first;
			rub(object, game.parts().rests.find(object.id), other, touch.normal, Vector2f{}, was, settle);
		}
	}

	// Off the window's edge, which faces straight in. An object in lockstep
	// (the invader block) turns as a whole block at a side edge instead.
	static void bounceOffEdge(Game& game, Object& object, Edge edge, float share)
	{
		if (object.kind->collisionData.lockstep && (edge == Edge::Left || edge == Edge::Right))
		{
			game.updateLockstepObjects(object);
			return;
		}
		object.velocity = bounced(object.velocity, edge, share);
	}

	static bool runBounce(const CmdBounce& bounce, const Doer& doer)
	{
		if (doer.touching()) { bounceOff(doer.game(), *doer.self, *doer.other, *doer.touch, bounce.share); }
		else if (doer.atEdge()) { bounceOffEdge(doer.game(), *doer.self, *doer.screenEdge, bounce.share); }
		return true;
	}

	static void printBounce(std::ostream& o, const CmdBounce& bounce) { o << "bounce(" << bounce.share << ")"; }

	extern const Verb bounceVerb;
	const Verb bounceVerb{
		.name = "bounce",
		.forms = { { "<bounce />", places::edge | places::touch, kindOf<CmdBounce>() } },
		.read = readBounce,
		.make = makeBounce,
		.run = runAs<runBounce>,
		.print = printAs<printBounce>,
	};
}
