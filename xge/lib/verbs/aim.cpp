// aim.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <stdexcept>

namespace xge
{
	// <aim>: Faces another object; with <lined>, refused unless it is lined
	// up.
	static Command makeAim(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const bool lined = isWritten(raw.distance);
		const float within = lined ? evaluate(raw.distance) : 0.0f;
		if (within < 0.0f) { throw std::runtime_error("<aim object=\"" + raw.object + "\">: the <lined> is below 0"); }
		return CmdAim{ raw.object, lined, within };
	}

	static bool runAim(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || executor.aim(*doer.self, std::get<CmdAim>(command));
	}

	extern const Verb aimVerb;
	const Verb aimVerb{
		.name = "aim",
		.forms = { { "<aim>", places::steering, kindOf<CmdAim>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeAim,
		.run = runAim,
	};
}
