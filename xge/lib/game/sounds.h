// sounds.h
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#pragma once

#include "game/name.h"

#include <set>
#include <string>
#include <vector>

namespace xge
{
	class Game;

	// The sounds the game asks for while it plays (game/sounds.cpp). The game
	// holds one (Game::sounds).
	class Sounds
	{
	public:
		explicit Sounds(const Game& owner) noexcept : game(owner) {}

		// <play sound="..." />: the game only asks for a sound, and Engine,
		// which owns the Audio, plays what was asked for once a frame
		// (takeRequests). Game itself never makes a noise, so it runs and
		// is tested without a sound device. A sound asked for twice in one
		// frame (two bricks broken at once) is played once.
		void request(Name name);

		// The sounds asked for since the last call, in the order asked, and
		// forgets them.
		std::vector<std::string> takeRequests(void);

		// A sound that loops (<loop />) is not asked for once but switched on:
		// request puts it among those looping, where it stays, playing over
		// and over, until silence takes it out (a <silence>, the key that
		// started it let go, or a reset of the whole game). Engine plays what
		// is in the list each frame and stops what has left it.
		const std::set<std::string>& looping(void) const noexcept;

		// The loops that should be playing now: those looping, and the
		// current state's <music> when it loops. What Engine plays.
		std::set<std::string> playing(void) const;

		// <silence sound="..." />: stops a sound, looping or not.
		void silence(Name name);

		// The sounds silenced since the last call, and forgets them.
		std::vector<std::string> takeSilenced(void);

		// Whether the sound of that name loops; false for no such sound.
		bool loops(Name name) const noexcept;

		// A new game, or a player's turn, starts quiet: every loop stops.
		void stopLoops() noexcept { looped.clear(); }

	private:
		const Game& game;
		std::vector<std::string> requested;
		std::vector<std::string> silenced;
		std::set<std::string> looped;
	};
}
