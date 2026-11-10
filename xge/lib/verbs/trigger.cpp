// trigger.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

namespace xge
{
	// <trigger>: Runs an object's action.
	static Command makeTrigger(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdTriggerAction{ raw.object, raw.action, raw.from, raw.otherwise, raw.objClass, cellPlaceOf(raw) };
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
		.make = makeTrigger,
		.run = runTrigger,
	};
}
