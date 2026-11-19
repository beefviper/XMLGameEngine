// hole.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <hole>: Cuts a hole in its picture where it was touched.
	static void readHole(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Its radius as its content, then how rough its edge is.
		const std::string here = where + " > <hole>";
		command.amount = readLeadValue(node, { "rough" }, "its radius, then <rough> if wanted", where);
		if (auto roughNode = findChild(&node, "rough")) { command.distance = readValue(*roughNode, here); }
	}

	static Command makeHole(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float radius = evaluate(raw.amount);
		const float rough = valueOr(raw.distance, 0.0f, evaluate);
		if (!(radius > 0.0f)) { throw std::runtime_error("<hole> has a radius of " + formatDisplayNumber(radius) + "; expected above 0"); }
		if (!(rough >= 0.0f && rough <= 1.0f)) { throw std::runtime_error("<hole> has a <rough> of " + formatDisplayNumber(rough) + "; expected 0 up to 1"); }
		return CmdHole{ radius, rough };
	}

	static bool runHole(const CmdHole& hole, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (doer.self && doer.touch) { cutHole(executor.game.parts(), executor.game.resources(), *doer.self, *doer.other, hole.radius, hole.rough); }
		return true;
	}

	static void printHole(std::ostream& o, const CmdHole& hole)
	{
		o << "hole(" << hole.radius << ", " << hole.rough << ")";
	}

	extern const Verb holeVerb;
	const Verb holeVerb{
		.name = "hole",
		.forms = { { "<hole>", places::touch, kindOf<CmdHole>() } },
		.read = readHole,
		.make = makeHole,
		.run = runAs<runHole>,
		.print = printAs<printHole>,
	};
}
