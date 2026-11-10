// release.cpp
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
	// <release>: Lets some of another object go from where it is.
	static void readRelease(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		// How many is optional: a bare <release object="..." /> is one.
		command.amount = hasContent(node) ? readValue(node, where) : RawValue::expression("1");
	}

	static Command makeRelease(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const int count = static_cast<int>(std::lround(evaluate(raw.amount)));
		if (count < 1) { throw std::runtime_error("<release> of " + raw.object + " has a count under 1"); }
		return CmdRelease{ raw.object, count };
	}

	static void checkRelease(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdRelease>(command).target, "<release>");
	}

	static bool runRelease(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdRelease& release = std::get<CmdRelease>(command);
		if (doer.self) { executor.release(*doer.self, release.target, release.count); }
		return true;
	}

	extern const Verb releaseVerb;
	const Verb releaseVerb{
		.name = "release",
		.forms = { { "<release>", places::self, kindOf<CmdRelease>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readRelease,
		.make = makeRelease,
		.check = checkRelease,
		.run = runRelease,
	};
}
