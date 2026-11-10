// jump.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <stdexcept>

namespace xge
{
	// <jump>: Up off the ground against the pull on it.
	static void readJump(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// The old <jump>, to a place: that is a <hop> with <seconds> now.
		if (!getAttribute(&node, "direction").empty()) { fail(where, "<jump direction=...> to a place is now <hop direction=...> with <seconds>; <jump> is up off the ground, its height as its content"); }
		command.amount = readValue(node, where);
	}

	static Command makeJump(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float height = evaluate(raw.amount);
		if (!(height > 0.0f)) { throw std::runtime_error("<jump> has a height of " + formatDisplayNumber(height) + "; expected more than 0"); }
		return CmdJump{ height };
	}

	static bool runJump(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !(doer.self && doer.action) || executor.jump(*doer.self, std::get<CmdJump>(command).height);
	}

	extern const Verb jumpVerb;
	const Verb jumpVerb{
		.name = "jump",
		.forms = { { "<jump>", places::controls, kindOf<CmdJump>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readJump,
		.make = makeJump,
		.check = nullptr,
		.run = runJump,
	};
}
