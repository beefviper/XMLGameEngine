// players.h
// XML Game Engine
// author: beefviper
// date: Nov 9, 2026

#pragma once

#include "game/command.h"
#include "game/name.h"
#include "game/object.h"

#include <map>
#include <vector>

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

	class Game;

	// Players as a group (game/players.cpp): the members of a group with
	// <turns /> take turns. The game holds one (Game::players).
	class Players
	{
	public:
		explicit Players(Game& owner) noexcept : game(owner) {}

		// The member of the group `name` on its turn (none: the group takes
		// no turns); whether a name is such a group; whether a key reaches
		// `member` now (always, unless it takes turns: then only on its
		// turn, and only the computer's presses for a member it thinks for;
		// the rest, played by people or by <computer> conditions, answer any
		// press).
		// <pass /> in a member's action: the next member's turn (false:
		// not its turn, or a holder waits for a wild card's call).
		Object* onTurn(Name name);
		bool takesTurns(Name name) const;
		bool answers(const Object& member) const;
		bool passPlayers(Object& member);
		// <pass member=>: the turn of the group's member of that number
		// (from 1; 0: nobody's turn). And the owner a verb's variable names:
		// players.current is the member of players on its turn.
		bool passPlayersTo(Name group, int number);
		Name turnOwner(Name owner);
		// The pieces (<owns class=>) of the player on its turn, in a board
		// game played by a group that takes turns; empty when there is none.
		Name turnClass();
		// Whether a press reaches `picker` now: while the computer thinks for
		// it (a <think object=> whose condition holds now), only the
		// computer's presses do.
		bool answersPicker(const Object& picker);
		// The engine says while it presses the computer's keys.
		void pressingForComputer(bool pressing) noexcept { computerPresses = pressing; }
		// Back to the first member's turn, the rest showing their
		// <turns sprite=>.
		void startPlayers();
		// Each member the computer plays, with its <think>.
		std::map<Name, CmdThink> memberThinks;

	private:
		Game& game;
		void handTurn(const std::vector<Object*>& members, Object* next);
		bool anyTurns{ false };
		bool computerPresses{ false };
	};
}
