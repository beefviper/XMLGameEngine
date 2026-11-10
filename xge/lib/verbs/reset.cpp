// reset.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/spelling.h"

#include <stdexcept>

namespace xge
{
	// <reset>: Back to how it started: the object running it, a named one, a
	// scene, or the whole game.
	static Command makeReset(const RawCommand& raw, const ValueEvaluator&)
	{
		// <reset /> puts the object in the rule (or, in a state's input or
		// condition, the whole game) back; <reset object="name" /> that one object.
		if (!raw.scene.empty() || !raw.way.empty()) { return CmdResetScene{ raw.scene, raw.way }; }
		if (raw.object.empty()) { return CmdReset{}; }
		return CmdResetObject{ raw.object };
	}

	static void checkReset(const Command& command, const Checking& checking)
	{
		if (const auto* scene = std::get_if<CmdResetScene>(&command)) { checkScene(checking, "reset", scene->scene, scene->way); }
		else if (const auto* object = std::get_if<CmdResetObject>(&command))
		{
			if (!findObject(checking.objects, object->target))
			{
				throw std::runtime_error(checking.where + ": <reset object=\"" + object->target + "\" /> names no object of the game" + didYouMean(object->target, objectNames(checking.objects)));
			}
		}
	}

	static bool runReset(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (const auto* object = std::get_if<CmdResetObject>(&command)) { executor.game.resetObject(object->target); }
		else if (const auto* scene = std::get_if<CmdResetScene>(&command)) { executor.game.reset(*scene); }
		else if (doer.self) { executor.restart(*doer.self); }
		else { executor.game.resetAll(); }
		return true;
	}

	extern const Verb resetVerb;
	const Verb resetVerb{
		.name = "reset",
		.forms = { { "<reset />", places::changes, kindOf<CmdReset>() }, { "<reset object>", places::changes, kindOf<CmdResetObject>() }, { "<reset scene>", places::changes, kindOf<CmdResetScene>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeReset,
		.check = checkReset,
		.run = runReset,
	};
}
