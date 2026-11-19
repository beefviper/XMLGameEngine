// reveal.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
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

	static void checkReveal(const CmdReveal& reveal, const Checking& checking)
	{
		checkObject(checking, reveal.target, "<reveal>");
	}

	static bool runReveal(const CmdReveal& reveal, const Doer& doer)
	{
		doer.game().reveal(reveal.target, reveal.count);
		return true;
	}

	static void printReveal(std::ostream& o, const CmdReveal& reveal)
	{
		o << "reveal(" << reveal.target << ", " << reveal.count << ")";
	}

	extern const Verb revealVerb;
	const Verb revealVerb{
		.name = "reveal",
		.forms = { { "<reveal>", places::changes, kindOf<CmdReveal>() } },
		.read = readReveal,
		.make = makeReveal,
		.check = checkAs<checkReveal>,
		.run = runAs<runReveal>,
		.print = printAs<printReveal>,
	};
}
