// knock.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <algorithm>
#include <optional>
#include <stdexcept>

namespace xge
{
	// <knock>: Two things with a mass trade their speed.
	static void readKnock(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		WrittenKnock& own = command.own.emplace<WrittenKnock>();
		if (hasContent(node)) { own.bounce = readValue(node, where); }
	}

	static Command makeKnock(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		const std::optional<WrittenValue>& own = ownOf<WrittenKnock>(written).bounce;
		const float bounce = valueOr(own, 1.0f, evaluate);
		if (!(bounce >= 0.0f && bounce <= 1.0f))
		{
			throw std::runtime_error("<knock> has a bounce of " + formatDisplayNumber(bounce) + "; expected 0 up to 1");
		}
		return CmdKnock{ bounce };
	}

	// Both with a mass (see knockedApart); `touch` is the object's own view
	// of it. Two that went into each other are put apart along the touch,
	// the lighter moved more.
	static void knockApart(Parts& parts, Object& object, Object& other, const Touch& touch, float bounce)
	{
		// Each one's speed at the point they touch: a thing turning about
		// its pivot (a flipper swinging) moves faster the further out the
		// point lies, and lends that speed (Object::velocityAt).
		const Vector2f way = touch.normal * -1.0f;
		const Vector2f objectAt = object.velocityAt(touch.point);
		const Vector2f otherAt = other.velocityAt(touch.point);
		// Against one that does not move, the one that does settles on it.
		const float settle = other.motion.mass <= 0.0f ? settleSpeed(object.acceleration, way)
			: object.motion.mass <= 0.0f ? settleSpeed(other.acceleration, way * -1.0f) : 0.0f;
		const std::optional<Knocked> after = knockedApart(objectAt, object.motion.mass, otherAt, other.motion.mass, way, bounce, settle);

		// Gone into each other: apart again, each by its share, whether or not
		// they are still closing (a ball the frame's sweep left inside a
		// flipper turning away from it is not left there).
		if (touch.depth > 0.0f && (object.motion.mass > 0.0f || other.motion.mass > 0.0f))
		{
			const float objectShare = object.motion.mass <= 0.0f ? 0.0f : other.motion.mass <= 0.0f ? 1.0f : other.motion.mass / (object.motion.mass + other.motion.mass);
			object.position -= way * (touch.depth * objectShare);
			other.position += way * (touch.depth * (1.0f - objectShare));
		}

		if (!after)
		{
			return;
		}

		const Vector2f objectWas = object.velocity;
		const Vector2f otherWas = other.velocity;
		object.velocity += after->first - objectAt;
		other.velocity += after->second - otherAt;

		// One against something that does not move: its friction.
		if (other.motion.mass <= 0.0f && object.motion.mass > 0.0f) { rub(object, parts.rests.find(object.id), other, touch.normal, otherAt, objectWas, settle); }
		else if (object.motion.mass <= 0.0f && other.motion.mass > 0.0f) { rub(other, parts.rests.find(other.id), object, way, objectAt, otherWas, settle); }
	}

	// <knock>: see CmdKnock. Into a thing with no mass, through it, to
	// everything with a mass resting against its far side.
	static void knockOrThrough(Game& game, Object& object, Object& other, const Touch& touch, float bounce)
	{
		if (other.motion.mass > 0.0f)
		{
			knockApart(game.parts(), object, other, touch, bounce);
			return;
		}

		// Through a thing that does not move: the far side of it is the side
		// the object's own touched edge faces, and what rests there is
		// knocked as though the object, as heavy as can be, had hit it.
		const Edge farSide = touch.edge;
		const Box base{ other.position, sizeOf(other) };
		const Vector2f way = touch.normal * -1.0f;
		for (Object& thing : game.getCurrentObjects())
		{
			if (&thing == &object || &thing == &other || !(thing.motion.mass > 0.0f) || !thing.collisionsOn || thing.isAirborne() || !game.isShown(thing))
			{
				continue;
			}

			const Box at{ thing.position, sizeOf(thing) };
			if (!restsAgainst(at, base, farSide))
			{
				continue;
			}

			const std::optional<Knocked> after = knockedApart(object.velocity, 0.0f, thing.velocity, thing.motion.mass, way, bounce);
			if (!after)
			{
				continue;
			}
			thing.velocity = after->second;

			Touch felt;
			felt.edge = opposite(farSide);
			felt.normal = way;
			felt.point = { std::clamp(touch.point.x, at.left(), at.right()), std::clamp(touch.point.y, at.top(), at.bottom()) };
			felt.along = 0.5f;
			felt.time = touch.time;
			felt.through = &other.objClass.str();
			game.reactTo(thing, object, felt);
		}
	}

	static bool runKnock(const CmdKnock& knock, const Doer& doer)
	{
		if (doer.self && doer.touch) { knockOrThrough(doer.game(), *doer.self, *doer.other, *doer.touch, knock.bounce); }
		return true;
	}

	static void printKnock(std::ostream& o, const CmdKnock& knock) { o << "knock(" << knock.bounce << ")"; }

	extern const Verb knockVerb;
	const Verb knockVerb{
		.name = "knock",
		.forms = { { "<knock>", places::touch, kindOf<CmdKnock>() } },
		.read = readKnock,
		.make = makeKnock,
		.run = runAs<runKnock>,
		.print = printAs<printKnock>,
	};
}
