// follow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// <follow>: Sets off along a path.
	static void readFollow(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "path", where);
		const std::string here = where + " > <follow>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "stagger" && child->getName() != "nearest") { fail(here, "unknown <" + child->getName() + ">; expected <stagger> and <nearest>"); }
		}
		if (auto stagger = findChild(&node, "stagger")) { command.seconds = readValue(*stagger, here); }
		if (auto nearest = findChild(&node, "nearest"))
		{
			if (node.getAttribute("object").empty()) { fail(here, "<nearest> picks among the objects object= names; this <follow> names none"); }
			command.amount = readValue(*nearest, here);
		}
	}

	static Command makeFollow(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float stagger = valueOr(raw.seconds, 0.0f, evaluate);
		if (stagger < 0.0f) { throw std::runtime_error("<follow path=\"" + raw.path + "\">: the <stagger> is below 0"); }
		const bool all = !isWritten(raw.amount);
		const int nearest = all ? 0 : static_cast<int>(std::lround(evaluate(raw.amount)));
		if (!all && nearest < 1) { throw std::runtime_error("<follow path=\"" + raw.path + "\">: <nearest> is under 1"); }
		return CmdFollow{ raw.path, raw.object, stagger, nearest };
	}

	static void checkFollow(const Command& command, const Checking& checking)
	{
		const CmdFollow& follow = std::get<CmdFollow>(command);
		if (!checking.paths.count(follow.path))
		{
			std::vector<std::string> names;
			for (const auto& entry : checking.paths) { names.push_back(entry.first); }
			throw std::runtime_error(checking.where + ": <follow path=\"" + follow.path + "\" /> names no path of the game" + didYouMean(follow.path, names));
		}
		if (!follow.target.empty()) { checkObject(checking, follow.target, "<follow>"); }
	}

	static bool runFollow(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		executor.follow(doer.self, std::get<CmdFollow>(command));
		return true;
	}

	static void printFollow(std::ostream& o, const Command& command)
	{
		const CmdFollow& f = std::get<CmdFollow>(command);
		o << "follow(" << f.path << (f.target.empty() ? "" : ", " + f.target) << ")";
	}

	static void printFollowRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.path << (command.object.empty() ? "" : ", " + command.object) << ")";
	}

	extern const Verb followVerb;
	const Verb followVerb{
		.name = "follow",
		.forms = { { "<follow>", places::changes, kindOf<CmdFollow>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readFollow,
		.make = makeFollow,
		.check = checkFollow,
		.run = runFollow,
		.print = printFollow,
		.printRaw = printFollowRaw,
	};
}
