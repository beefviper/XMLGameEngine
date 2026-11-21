// order.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

#include <memory>
#include <string>

namespace xge
{
	// <order>: Puts what a holder holds in order.
	static void readOrder(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		command.object = requireAttribute(node, "group", where);
		const std::string here = where + " > <order>";
		if (!findChild(&node, "random")) { fail(here, "orders what that holder holds at <random />"); }
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "random") { fail(here, "unknown <" + child->getName() + ">; expected <random />"); }
		}
	}

	static Command makeOrder(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdOrder{ written.object };
	}

	static bool runOrder(const CmdOrder& order, const Doer& doer)
	{
		return doer.game().holders().orderHeld(order.group);
	}

	static void printOrder(std::ostream& o, const CmdOrder& order)
	{
		o << "order(" << order.group << ", random)";
	}

	extern const Verb orderVerb;
	const Verb orderVerb{
		.name = "order",
		.forms = { { "<order>", places::changes, kindOf<CmdOrder>() } },
		.read = readOrder,
		.make = makeOrder,
		.run = runAs<runOrder>,
		.print = printAs<printOrder>,
	};
}
