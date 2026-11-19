// jump.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/geometry.h"
#include "load/reading.h"

#include <stdexcept>
#include <variant>

namespace xge
{
	// <jump>: Up off the ground against the pull on it.
	static void readJump(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// The old <jump>, to a place: that is a <hop> with <seconds> now.
		if (!getAttribute(&node, "direction").empty()) { fail(where, "<jump direction=...> to a place is now <hop direction=...> with <seconds>; <jump> is up off the ground, its height as its content"); }
		command.own = RawJump{ readValue(node, where) };
	}

	static Command makeJump(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float height = evaluate(ownOf<RawJump>(raw).height);
		if (!(height > 0.0f)) { throw std::runtime_error("<jump> has a height of " + formatDisplayNumber(height) + "; expected more than 0"); }
		return CmdJump{ height };
	}

	// Up off the ground at the speed that, under the object's own pull, rises
	// `height` pixels: v * v = 2 * pull * height. Only from the ground and not
	// on a ladder; nothing without a pull down (the loader refuses that).
	static bool jumpUp(Object& object, float height)
	{
		if (!object.footing.grounded || object.footing.climbing || object.acceleration.y <= 0.0f)
		{
			return false;
		}

		object.velocity.y = -jumpSpeed(object.acceleration.y, height);
		object.footing.grounded = false;
		object.footing.jumping = true;
		return true;
	}

	static bool runJump(const CmdJump& jump, const Doer& doer)
	{
		return !(doer.self && doer.action) || jumpUp(*doer.self, jump.height);
	}

	static void printJump(std::ostream& o, const CmdJump& jump)
	{
		o << "jump(" << jump.height << ")";
	}

	extern const Verb jumpVerb;
	const Verb jumpVerb{
		.name = "jump",
		.forms = { { "<jump>", places::controls, kindOf<CmdJump>() } },
		.read = readJump,
		.make = makeJump,
		.run = runAs<runJump>,
		.print = printAs<printJump>,
	};
}
