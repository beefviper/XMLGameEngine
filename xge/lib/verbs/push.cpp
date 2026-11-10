// push.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <string>

namespace xge
{
	// <push>: Goes to another state, on top of this one.
	static void readPush(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "state", where);
	}

	static Command makePush(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPushState{ raw.state };
	}

	static void checkPush(const Command& command, const Checking& checking)
	{
		const std::string& name = std::get<CmdPushState>(command).name;
		checkState(checking, name, "<push state=\"" + name + "\" />");
	}

	static bool runPush(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.pushState(std::get<CmdPushState>(command).name);
		return true;
	}

	extern const Verb pushVerb;
	const Verb pushVerb{
		.name = "push",
		.forms = { { "<push>", places::flow, kindOf<CmdPushState>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readPush,
		.make = makePush,
		.check = checkPush,
		.run = runPush,
	};
}
