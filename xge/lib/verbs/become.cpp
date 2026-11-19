// become.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace xge
{
	// <become>: Shows another of its looks, or wears another style's colours.
	static void readBecome(const XmlNode&, const std::string& where, RawCommand& command)
	{
		if (command.sprite.empty() && command.style.empty())
		{
			fail(where, "<become> needs a sprite= (a look), a style= (a palette), or both");
		}
	}

	static Command makeBecome(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdBecome{ raw.sprite, raw.object, raw.from, raw.style };
	}

	static void checkBecome(const CmdBecome& become, const Checking& checking)
	{
		// With object=, every object of that name has the looks it names. (Its
		// own looks, without: checked with the object, in load/check.cpp.)
		if (become.target.empty()) { return; }
		const std::vector<Object>& objects = checking.objects;
		bool any = false;
		for (const Object& object : objects)
		{
			if (!object.named(become.target)) { continue; }
			any = true;
			if (!become.style.empty()) { wantPalette(checking, object, become.style); }
			const auto hasLook = [&](const Object& of) { return std::any_of(of.looks.all().begin(), of.looks.all().end(), [&](const Object::Look& look) { return look.name == become.sprite; }); };
			// A holder may take a look of what it holds: they turn to it.
			const bool heldLook = checking.parts.holders.has(object.id) && std::any_of(objects.begin(), objects.end(), [&](const Object& item) { return checking.parts.items.has(item.id) && hasLook(item); });
			if (!become.sprite.empty() && !hasLook(object) && !heldLook)
			{
				throw std::runtime_error(checking.where + ": <become sprite=\"" + become.sprite + "\" object=\"" + become.target + "\" />: '" + object.name + "' has no look of that name");
			}
			if (!become.from.empty() && std::none_of(object.looks.all().begin(), object.looks.all().end(), [&](const Object::Look& look) { return look.name == become.from; }))
			{
				throw std::runtime_error(checking.where + ": <become from=\"" + become.from + "\" object=\"" + become.target + "\" />: '" + object.name + "' has no look of that name");
			}
		}
		if (!any) { checkObject(checking, become.target, "<become>"); }
	}

	static bool runBecome(const CmdBecome& becomeCommand, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		executor.become(doer.self, becomeCommand);
		return true;
	}

	static void printBecome(std::ostream& o, const CmdBecome& become)
	{
		o << "become(" << become.sprite << (become.target.empty() ? "" : ", " + become.target) << (become.from.empty() ? "" : ", from " + become.from) << (become.style.empty() ? "" : ", style " + become.style) << ")";
	}

	extern const Verb becomeVerb;
	const Verb becomeVerb{
		.name = "become",
		.forms = { { "<become>", places::changes, kindOf<CmdBecome>() } },
		.read = readBecome,
		.make = makeBecome,
		.check = checkAs<checkBecome>,
		.run = runAs<runBecome>,
		.print = printAs<printBecome>,
	};
}
