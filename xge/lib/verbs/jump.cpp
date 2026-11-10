// jump.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <jump>: Through the air to a place, touching nothing until it lands.
	static void readJump(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// How far as its content, then how long and how high.
		requireAttribute(node, "direction", where);
		const std::string here = where + " > <jump>";
		command.amount = readLeadValue(node, { "seconds", "arc" }, "its distance, then <seconds> and <arc>, each if wanted", where);
		if (auto seconds = findChild(&node, "seconds")) { command.seconds = readValue(*seconds, here); }
		if (auto arc = findChild(&node, "arc")) { command.arc = readValue(*arc, here); }
	}

	static Command makeJump(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		CmdJump jump{ directionFromName(raw.direction, raw.verb), evaluate(raw.amount) };
		if (isWritten(raw.seconds)) { jump.seconds = evaluate(raw.seconds); }
		if (!(jump.seconds > 0.0f)) { throw std::runtime_error("<jump> has <seconds> of " + std::to_string(jump.seconds) + "; expected more than 0"); }
		jump.arc = arcOf(raw, "jump", jump.seconds, evaluate);
		return jump;
	}

	static bool runJump(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !(doer.self && doer.action) || executor.startJump(*doer.self, std::get<CmdJump>(command));
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
