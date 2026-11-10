// trigger.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// <trigger>: Runs an object's action.
	static void readTrigger(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "action", where);
		const std::unique_ptr<XmlNode> child = node.getFirstChild();
		if (child && child->getName() != "here" && child->getName() != "around")
		{
			fail(where + " > <trigger>", "holds <" + child->getName() + ">; for now only a trigger in a key's <input> holds an <otherwise>, and one with class= a <here /> or an <around />");
		}
		if (child)
		{
			readCellPlace(node, where + " > <trigger>", command);
			requireAttribute(node, "class", where + " > <trigger> with <" + child->getName() + ">");
			if (!command.object.empty()) { fail(where + " > <trigger>", "with <" + child->getName() + "> runs the action of pieces about the object running it, so it names no object="); }
		}
		else if (!command.objClass.empty()) { fail(where + " > <trigger class>", "class= picks the pieces in a <here /> or an <around />; give one, or leave class= out"); }
	}

	static Command makeTrigger(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdTriggerAction{ raw.object, raw.action, raw.from, raw.otherwise, raw.objClass, cellPlaceOf(raw) };
	}

	static void checkTrigger(const Command& command, const Checking& checking)
	{
		const CmdTriggerAction& trigger = std::get<CmdTriggerAction>(command);
		const std::vector<Object>& objects = checking.objects;
		if (trigger.place.any())
		{
			// About the cells round it: some piece of the class has the action.
			const bool any = std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return object.objClass == trigger.objClass && object.action.count(trigger.action); });
			if (!any) { throw std::runtime_error(checking.where + ": <trigger action=\"" + trigger.action + "\" class=\"" + trigger.objClass + "\">: no object of that class has an action of that name"); }
			return;
		}
		if (trigger.object.empty())
		{
			throw std::runtime_error(checking.where + ": <trigger action=\"" + trigger.action + "\" /> needs object=\"...\" here; only an object's own rules, actions and timers can leave it out");
		}
		const Object* object = findObject(objects, trigger.object);
		if (!object)
		{
			throw std::runtime_error(checking.where + ": an action of '" + trigger.object + "' is asked for, and there is no object of that name" + didYouMean(trigger.object, objectNames(objects)));
		}
		if (!object->action.count(trigger.action))
		{
			std::vector<std::string> names;
			for (const auto& entry : object->action) { names.push_back(entry.first); }
			throw std::runtime_error(checking.where + ": '" + trigger.object + "' has no action named '" + trigger.action + "'" + didYouMean(trigger.action, names)
				+ (names.empty() ? std::string(" (it has no <actions>)") : "; its actions are: " + listOf(names)));
		}
	}

	static bool runTrigger(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		// Another action, of this object or another, as part of this.
		const CmdTriggerAction& trigger = std::get<CmdTriggerAction>(command);
		if (!trigger.place.any()) { executor.triggerObjectAction(trigger.object, trigger.action, doer.pressed, doer.byRule, trigger.from); }
		else if (doer.self && doer.pressed) { executor.triggerAbout(*doer.self, trigger, doer.byRule); }
		return true;
	}

	extern const Verb triggerVerb;
	const Verb triggerVerb{
		.name = "trigger",
		.forms = { { "<trigger>", places::changes, kindOf<CmdTriggerAction>() } },
		.formOf = nullptr,
		.onRelease = true,
		.read = readTrigger,
		.make = makeTrigger,
		.check = checkTrigger,
		.run = runTrigger,
	};
}
