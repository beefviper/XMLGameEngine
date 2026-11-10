// follow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/spelling.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// <follow>: Sets off along a path.
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

	extern const Verb followVerb;
	const Verb followVerb{
		.name = "follow",
		.forms = { { "<follow>", places::changes, kindOf<CmdFollow>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeFollow,
		.check = checkFollow,
		.run = runFollow,
	};
}
