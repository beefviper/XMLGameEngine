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
#include <variant>

namespace xge
{
	// <reveal>: Puts some of an object that is out of play back in.
	static void readReveal(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		// How many is optional: a bare <reveal object="..." /> is one.
		RawReveal& own = command.own.emplace<RawReveal>();
		if (hasContent(node)) { own.count = readValue(node, where); }
	}

	static Command makeReveal(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const std::optional<RawValue>& written = ownOf<RawReveal>(raw).count;
		const int count = written ? static_cast<int>(std::lround(evaluate(*written))) : 1;
		if (count < 1) { throw std::runtime_error("<reveal> of " + raw.object + " has a count under 1"); }
		return CmdReveal{ raw.object, count };
	}

	static void checkReveal(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdReveal>(command).target, "<reveal>");
	}

	static bool runReveal(const Command& command, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		const CmdReveal& reveal = std::get<CmdReveal>(command);
		executor.game.reveal(reveal.target, reveal.count);
		return true;
	}

	static void printReveal(std::ostream& o, const Command& command)
	{
		const CmdReveal& r = std::get<CmdReveal>(command);
		o << "reveal(" << r.target << ", " << r.count << ")";
	}

	extern const Verb revealVerb;
	const Verb revealVerb{
		.name = "reveal",
		.forms = { { "<reveal>", places::changes, kindOf<CmdReveal>() } },
		.read = readReveal,
		.make = makeReveal,
		.check = checkReveal,
		.run = runReveal,
		.print = printReveal,
	};
}
