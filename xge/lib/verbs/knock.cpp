// knock.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <algorithm>
#include <optional>
#include <stdexcept>

namespace xge
{
	// <knock>: Two things with a mass trade their speed.
	static void readKnock(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		if (hasContent(node)) { command.amount = readValue(node, where); }
	}

	static Command makeKnock(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float bounce = valueOr(raw.amount, 1.0f, evaluate);
		if (!(bounce >= 0.0f && bounce <= 1.0f))
		{
			throw std::runtime_error("<knock> has a bounce of " + formatDisplayNumber(bounce) + "; expected 0 up to 1");
		}
		return CmdKnock{ bounce };
	}

	// <knock>: see CmdKnock. Into a thing with no mass, through it, to
	// everything with a mass resting against its far side.
	static void knockOrThrough(CommandExecutor& executor, Object& object, Object& other, const Touch& touch, float bounce)
	{
		if (other.mass > 0.0f)
		{
			xge::knock(object, other, touch, bounce);
			return;
		}

		// Through a thing that does not move: the far side of it is the side
		// the object's own touched edge faces, and what rests there is
		// knocked as though the object, as heavy as can be, had hit it.
		const Edge farSide = touch.edge;
		const Box base{ other.position, sizeOf(other) };
		const Vector2f way = touch.normal * -1.0f;
		for (Object& thing : executor.game.getCurrentObjects())
		{
			if (&thing == &object || &thing == &other || !(thing.mass > 0.0f) || !thing.collisionsOn || thing.isAirborne() || !executor.game.isShown(thing))
			{
				continue;
			}

			const Box at{ thing.position, sizeOf(thing) };
			if (!restsAgainst(at, base, farSide))
			{
				continue;
			}

			const std::optional<Knocked> after = knockedApart(object.velocity, 0.0f, thing.velocity, thing.mass, way, bounce);
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
			felt.through = &other.objClass;
			executor.game.reactTo(thing, object, felt);
		}
	}

	static bool runKnock(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdKnock& knock = std::get<CmdKnock>(command);
		if (doer.self && doer.touch) { knockOrThrough(executor, *doer.self, *doer.other, *doer.touch, knock.bounce); }
		return true;
	}

	static void printKnock(std::ostream& o, const Command& command)
	{
		const CmdKnock& k = std::get<CmdKnock>(command);
		o << "knock(" << k.bounce << ")";
	}

	static void printKnockRaw(std::ostream& o, const RawCommand& command)
	{
		if (command.amount.kind != RawValue::Kind::Expression || !command.amount.text.empty())
		{
			o << "(" << command.amount << ")";
		}
	}

	extern const Verb knockVerb;
	const Verb knockVerb{
		.name = "knock",
		.forms = { { "<knock>", places::touch, kindOf<CmdKnock>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readKnock,
		.make = makeKnock,
		.check = nullptr,
		.run = runKnock,
		.print = printKnock,
		.printRaw = printKnockRaw,
	};
}
