// push.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"

#include <string>

namespace xge
{
	// <push>: Goes to another state, on top of this one.
	static void readPush(const XmlNode& node, const std::string& where, WrittenCommand&)
	{
		requireAttribute(node, "state", where);
	}

	static Command makePush(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdPushState{ written.state };
	}

	static void checkPush(const CmdPushState& pushState, const Checking& checking)
	{
		const std::string& name = pushState.name;
		checkState(checking, name, "<push state=\"" + name + "\" />");
	}

	static bool runPush(const CmdPushState& pushStateCommand, const Doer& doer)
	{
		doer.game().pushState(pushStateCommand.name);
		return true;
	}

	static void printPush(std::ostream& o, const CmdPushState& pushState)
	{
		o << "state(" << pushState.name << ")";
	}

	extern const Verb pushVerb;
	const Verb pushVerb{
		.name = "push",
		.forms = { { "<push>", places::flow, kindOf<CmdPushState>() } },
		.read = readPush,
		.make = makePush,
		.check = checkAs<checkPush>,
		.run = runAs<runPush>,
		.print = printAs<printPush>,
	};
}
