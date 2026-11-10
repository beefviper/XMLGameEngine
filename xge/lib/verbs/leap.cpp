// leap.cpp
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
	// <leap>: Up off the ground against the pull on it.
	static void readLeap(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.amount = readValue(node, where);
	}

	static Command makeLeap(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const float height = evaluate(raw.amount);
		if (!(height > 0.0f)) { throw std::runtime_error("<leap> has a height of " + formatDisplayNumber(height) + "; expected more than 0"); }
		return CmdLeap{ height };
	}

	static bool runLeap(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !(doer.self && doer.action) || executor.leap(*doer.self, std::get<CmdLeap>(command).height);
	}

	extern const Verb leapVerb;
	const Verb leapVerb{
		.name = "leap",
		.forms = { { "<leap>", places::controls, kindOf<CmdLeap>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readLeap,
		.make = makeLeap,
		.check = nullptr,
		.run = runLeap,
	};
}
