// follow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
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
	static void readFollow(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		requireAttribute(node, "path", where);
		const std::string here = where + " > <follow>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "stagger" && child->getName() != "nearest") { fail(here, "unknown <" + child->getName() + ">; expected <stagger> and <nearest>"); }
		}
		WrittenFollow& follow = command.own.emplace<WrittenFollow>();
		if (auto stagger = findChild(&node, "stagger")) { follow.stagger = readValue(*stagger, here); }
		if (auto nearest = findChild(&node, "nearest"))
		{
			if (node.getAttribute("object").empty()) { fail(here, "<nearest> picks among the objects object= names; this <follow> names none"); }
			follow.nearest = readValue(*nearest, here);
		}
	}

	static Command makeFollow(const WrittenCommand& raw, const ValueEvaluator& evaluate)
	{
		const WrittenFollow& written = ownOf<WrittenFollow>(raw);
		const float stagger = valueOr(written.stagger, 0.0f, evaluate);
		if (stagger < 0.0f) { throw std::runtime_error("<follow path=\"" + raw.path + "\">: the <stagger> is below 0"); }
		const bool all = !written.nearest;
		const int nearest = all ? 0 : static_cast<int>(std::lround(evaluate(*written.nearest)));
		if (!all && nearest < 1) { throw std::runtime_error("<follow path=\"" + raw.path + "\">: <nearest> is under 1"); }
		return CmdFollow{ raw.path, raw.object, stagger, nearest };
	}

	static void checkFollow(const CmdFollow& follow, const Checking& checking)
	{
		if (!checking.paths.count(follow.path))
		{
			std::vector<std::string> names;
			for (const auto& entry : checking.paths) { names.push_back(entry.first); }
			throw std::runtime_error(checking.where + ": <follow path=\"" + follow.path + "\" /> names no path of the game" + didYouMean(follow.path, names));
		}
		if (!follow.target.empty()) { checkObject(checking, follow.target, "<follow>"); }
	}

	// With object=, every object of that name sets off along the path
	// (Game::follow); without, the object running it does.
	static void setOff(Game& game, Object* self, const CmdFollow& follow)
	{
		if (!follow.target.empty())
		{
			game.follow(follow.target, follow.path, follow.stagger, follow.nearest, self);
		}
		else if (self)
		{
			game.follow(*self, follow.path);
		}
	}

	static bool runFollow(const CmdFollow& follow, const Doer& doer)
	{
		setOff(doer.game(), doer.self, follow);
		return true;
	}

	static void printFollow(std::ostream& o, const CmdFollow& follow)
	{
		o << "follow(" << follow.path << (follow.target.empty() ? "" : ", " + follow.target) << ")";
	}

	extern const Verb followVerb;
	const Verb followVerb{
		.name = "follow",
		.forms = { { "<follow>", places::changes, kindOf<CmdFollow>() } },
		.read = readFollow,
		.make = makeFollow,
		.check = checkAs<checkFollow>,
		.run = runAs<runFollow>,
		.print = printAs<printFollow>,
	};
}
