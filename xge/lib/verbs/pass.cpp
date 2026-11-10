// pass.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <cmath>
#include <stdexcept>

namespace xge
{
	// <pass>: The turn goes to the next player.
	static void readPass(const XmlNode& node, const std::string&, RawCommand& command)
	{
		if (!node.getAttribute("member").empty()) { command.amount = RawValue::expression(node.getAttribute("member")); }
	}

	static Command makePass(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (!isWritten(raw.amount)) { return CmdPass{ raw.object, -1 }; }
		const float number = evaluate(raw.amount);
		if (number < 0.0f || number != std::floor(number)) { throw std::runtime_error("<pass member=\"" + raw.amount.text + "\">: is " + formatDisplayNumber(number) + "; expected a member's number from 1, or 0 for nobody's turn"); }
		return CmdPass{ raw.object, static_cast<int>(number) };
	}

	static bool runPass(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdPass& pass = std::get<CmdPass>(command);
		Object* const self = doer.self;
		if (pass.member >= 0) { return executor.game.passPlayersTo(pass.group.empty() && self ? self->groupName : pass.group, pass.member); }
		if (!pass.group.empty())
		{
			Object* const on = executor.game.onTurn(pass.group);
			return !on || executor.game.passPlayers(*on);
		}
		if (!self) { executor.game.passTurn(false); }
		else if (self->takesTurns) { return executor.game.passPlayers(*self); }
		return true;
	}

	extern const Verb passVerb;
	const Verb passVerb{
		.name = "pass",
		.forms = { { "<pass />", places::flow | in(Place::Action), kindOf<CmdPass>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readPass,
		.make = makePass,
		.check = nullptr,
		.run = runPass,
	};
}
