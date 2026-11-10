// order.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <memory>
#include <string>

namespace xge
{
	// <order>: Puts what a holder holds in order.
	static void readOrder(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.object = requireAttribute(node, "group", where);
		const std::string here = where + " > <order>";
		if (!findChild(&node, "random")) { fail(here, "orders what that holder holds at <random />"); }
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "random") { fail(here, "unknown <" + child->getName() + ">; expected <random />"); }
		}
	}

	static Command makeOrder(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdOrder{ raw.object };
	}

	static bool runOrder(const Command& command, const Doer&, CommandExecutor& executor)
	{
		return executor.game.orderHeld(std::get<CmdOrder>(command).group);
	}

	extern const Verb orderVerb;
	const Verb orderVerb{
		.name = "order",
		.forms = { { "<order>", places::changes, kindOf<CmdOrder>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readOrder,
		.make = makeOrder,
		.check = nullptr,
		.run = runOrder,
	};
}
