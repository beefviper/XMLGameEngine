// reveal.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <cmath>
#include <stdexcept>

namespace xge
{
	// <reveal>: Puts some of an object that is out of play back in.
	static void readReveal(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		// How many is optional: a bare <reveal object="..." /> is one.
		command.amount = hasContent(node) ? readValue(node, where) : RawValue::expression("1");
	}

	static Command makeReveal(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const int count = static_cast<int>(std::lround(evaluate(raw.amount)));
		if (count < 1) { throw std::runtime_error("<reveal> of " + raw.object + " has a count under 1"); }
		return CmdReveal{ raw.object, count };
	}

	static void checkReveal(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdReveal>(command).target, "<reveal>");
	}

	static bool runReveal(const Command& command, const Doer&, CommandExecutor& executor)
	{
		const CmdReveal& reveal = std::get<CmdReveal>(command);
		executor.game.reveal(reveal.target, reveal.count);
		return true;
	}

	extern const Verb revealVerb;
	const Verb revealVerb{
		.name = "reveal",
		.forms = { { "<reveal>", places::changes, kindOf<CmdReveal>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readReveal,
		.make = makeReveal,
		.check = checkReveal,
		.run = runReveal,
	};
}
