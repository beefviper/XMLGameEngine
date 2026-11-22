// think.h
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#pragma once

#include "game/command.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace xge
{
	class Game;
	struct Object;
	struct ThinkJob;

	// <think> (game/think.cpp): the computer playing a board game's pieces.
	// It searches on a thread of its own when `aside` (a game in a window),
	// else at once, in the frame (the tests: the same each run).
	class Thinker
	{
	public:
		// The button to press this frame, if any, for a <think> of `game`.
		std::optional<KeyCode> think(Game& game, const CmdThink& think, float seconds);
		// Its turn is over: the search under way gives up.
		void stop();
		void inBackground(bool on) noexcept { aside = on; }

	private:
		std::shared_ptr<ThinkJob> start(Game& game, const CmdThink& think, const Object& picker);
		static void run(ThinkJob& job);
		void forget();

		// The search under way, then the plan carried out: the cells the
		// picker goes to in turn (the piece, then each cell of its move),
		// and the seconds until its next step.
		std::shared_ptr<ThinkJob> job;
		bool planned{ false };
		std::vector<std::pair<int, int>> cells;
		std::size_t next{};
		float wait{};
		// The seconds spent thinking about this move so far.
		float thinking{};
		bool aside{ false };
	};
}
