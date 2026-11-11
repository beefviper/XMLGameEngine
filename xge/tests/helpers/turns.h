// turns.h
// XML Game Engine
// author: beefviper
// date: Nov 11, 2026
//
// Whose turn it is in a game whose players are a group that takes <turns>:
// the number of the member on its turn, counting from 1 in the order the
// group writes them (0: nobody's).

#pragma once

#include "game/game.h"

#include <string>

namespace turns
{
	inline float number(xge::Game& game, const std::string& group = "players")
	{
		const xge::Object* on = game.onTurn(group);
		float counted = 0.0f;
		for (const xge::Object& each : game.getCurrentObjects())
		{
			if (!on || !each.takesTurns || each.groupName != on->groupName) { continue; }
			counted += 1.0f;
			if (&each == on) { return counted; }
		}
		return 0.0f;
	}
}
