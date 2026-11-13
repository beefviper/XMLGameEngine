// random_keys.h
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// A game played on random presses of the keys it binds, on the null window,
// for the random-keys tests (engine/test_vocabulary.cpp's short one, and
// long/test_random_keys.cpp's long one).

#pragma once

#include "played.h"
#include "game/random.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <exception>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace played
{
	// Every key any state of the game answers, at any moment.
	inline std::vector<xge::KeyCode> keysOf(const xge::Game& game)
	{
		std::set<xge::KeyCode> keys;
		for (const xge::State& state : game.getStates())
		{
			for (const auto* map : state.keyMaps())
			{
				for (const auto& [key, commands] : *map) { keys.insert(key); }
			}
		}
		return { keys.begin(), keys.end() };
	}

	inline bool finite(const xge::Object& object)
	{
		return std::isfinite(object.position.x) && std::isfinite(object.position.y)
			&& std::isfinite(object.velocity.x) && std::isfinite(object.velocity.y);
	}

	// `frames` frames of the game in `file`, a key going down or up about one
	// frame in ten, picked from `seed`. Nothing may throw, and every object
	// must end at a finite place and speed.
	inline void playRandomKeys(const std::string& file, unsigned seed, int frames)
	{
		INFO(file << ", seed " << seed);
		xge::fixRandomSeed(seed);
		std::mt19937 pick(seed);
		Played play{ file };
		const std::vector<xge::KeyCode> keys = keysOf(play.game);
		REQUIRE_FALSE(keys.empty());
		std::set<xge::KeyCode> down;

		int frame = 0;
		try
		{
			for (; frame < frames; ++frame)
			{
				if (pick() % 10 == 0)
				{
					const xge::KeyCode key = keys[pick() % keys.size()];
					if (down.erase(key)) { play.engine.handleKeyReleased(key); }
					else { down.insert(key); play.engine.handleKeyPressed(key); }
				}
				play.engine.step();
			}
		}
		catch (const std::exception& error)
		{
			FAIL("threw at frame " << frame << ": " << error.what());
		}

		int broken = 0;
		for (const xge::Object& object : play.game.getCurrentObjects())
		{
			if (!finite(object)) { UNSCOPED_INFO(object.name << " is not at a finite place or speed"); ++broken; }
		}
		CHECK(broken == 0);
	}

	// The shipped games: every .xml in descriptions/, less the scratch files
	// other tests write there while they run.
	inline std::vector<std::string> shippedGames(const std::vector<std::string>& files)
	{
		std::vector<std::string> games;
		for (const std::string& file : files)
		{
			if (file.find("_scratch") == std::string::npos) { games.push_back(file); }
		}
		return games;
	}

	// Every `parts`-th of `files`, starting at the `part`-th (1 to parts):
	// a long test over every game split into cases of a like size, which
	// ctest runs side by side.
	inline std::vector<std::string> share(const std::vector<std::string>& files, std::size_t part, std::size_t parts)
	{
		std::vector<std::string> mine;
		for (std::size_t i = part - 1; i < files.size(); i += parts) { mine.push_back(files[i]); }
		return mine;
	}
}
