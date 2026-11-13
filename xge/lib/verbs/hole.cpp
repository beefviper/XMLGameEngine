// hole.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
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

	static bool runHole(const Command& command, const Doer& doer, CommandExecutor&)
	{
		const CmdHole& hole = std::get<CmdHole>(command);
		if (doer.self && doer.touch) { cutHole(*doer.self, *doer.other, hole.radius, hole.rough); }
		return true;
	}

	static void printHole(std::ostream& o, const Command& command)
	{
		const CmdHole& h = std::get<CmdHole>(command);
		o << "hole(" << h.radius << ", " << h.rough << ")";
	}

	static void printHoleRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.amount << ", " << command.distance << ")";
	}

	extern const Verb holeVerb;
	const Verb holeVerb{
		.name = "hole",
		.forms = { { "<hole>", places::touch, kindOf<CmdHole>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readHole,
		.make = makeHole,
		.check = nullptr,
		.run = runHole,
		.print = printHole,
		.printRaw = printHoleRaw,
	};
}
