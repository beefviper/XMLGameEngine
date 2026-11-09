// players.h
// XML Game Engine
// author: beefviper
// date: Nov 9, 2026

#pragma once

namespace xge
{
	struct GameData;

	// The game's players as a group (<turns> on a group), worked out once
	// every object is laid out (game.cpp, before anything plays): each
	// member gets its live `turn` (1 for the first, which starts, else 0),
	// and a member's <think> is told the member it plays. Throws, naming
	// what is wrong, when a <think> sits on something that takes no turns
	// or the look a group's <turns sprite=> names is not one its members have.
	void settlePlayers(GameData& data);
}
