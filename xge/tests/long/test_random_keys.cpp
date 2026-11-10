// test_random_keys.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// A long test (xgetest-long): every game played for 20,000 frames of random
// presses of the keys it binds, from two seeds, on the null window. Nothing
// may throw, and no position or speed may stop being a finite number.

#include "cli.h"
#include "played.h"
#include "game/random.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace
{
	// Every key any state of the game answers, at any moment.
	std::vector<xge::KeyCode> keysOf(const xge::Game& game)
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

	bool finite(const xge::Object& object)
	{
		return std::isfinite(object.position.x) && std::isfinite(object.position.y)
			&& std::isfinite(object.velocity.x) && std::isfinite(object.velocity.y);
	}
}

TEST_CASE("every game survives 20,000 frames of random keys, from two seeds", "[long][keys]")
{
	constexpr int kFrames = 20000;
	for (const std::string& file : xge::gameFilesIn("descriptions"))
	{
		for (const unsigned seed : { 1u, 2u })
		{
			INFO(file << ", seed " << seed);
			xge::fixRandomSeed(seed);
			std::mt19937 pick(seed);
			played::Played play{ file };
			const std::vector<xge::KeyCode> keys = keysOf(play.game);
			REQUIRE_FALSE(keys.empty());
			std::set<xge::KeyCode> down;

			int frame = 0;
			try
			{
				for (; frame < kFrames; ++frame)
				{
					// About one key in ten frames goes down or up.
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
	}
}
