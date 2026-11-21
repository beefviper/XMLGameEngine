// pass.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

#include <cmath>
#include <stdexcept>
#include <variant>

namespace xge
{
	// <pass>: The turn goes to the next player.
	static void readPass(const XmlNode& node, const std::string&, WrittenCommand& command)
	{
		WrittenPass& own = command.own.emplace<WrittenPass>();
		if (!node.getAttribute("member").empty()) { own.member = WrittenValue::expression(node.getAttribute("member")); }
	}

	static Command makePass(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		const std::optional<WrittenValue>& member = ownOf<WrittenPass>(written).member;
		if (!member) { return CmdPass{ written.object, -1 }; }
		const float number = evaluate(*member);
		if (number < 0.0f || number != std::floor(number)) { throw std::runtime_error("<pass member=\"" + member->text + "\">: is " + formatDisplayNumber(number) + "; expected a member's number from 1, or 0 for nobody's turn"); }
		return CmdPass{ written.object, static_cast<int>(number) };
	}

	static bool runPass(const CmdPass& pass, const Doer& doer)
	{
		Game& game = doer.game();
		Object* const self = doer.self;
		if (pass.member >= 0) { return game.players().passPlayersTo(pass.group.empty() && self ? self->groupName : pass.group, pass.member); }
		if (!pass.group.empty())
		{
			Object* const on = game.players().onTurn(pass.group);
			return !on || game.players().passPlayers(*on);
		}
		if (!self) { game.passTurn(false); }
		else if (game.turnsOf(*self).takes) { return game.players().passPlayers(*self); }
		return true;
	}

	static void printPass(std::ostream& o, const CmdPass&)
	{
		o << "pass()";
	}

	extern const Verb passVerb;
	const Verb passVerb{
		.name = "pass",
		.forms = { { "<pass />", places::flow | in(Place::Action), kindOf<CmdPass>() } },
		.read = readPass,
		.make = makePass,
		.run = runAs<runPass>,
		.print = printAs<printPass>,
	};
}
