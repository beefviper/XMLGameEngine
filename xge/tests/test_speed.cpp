// test_speed.cpp
// XML Game Engine
// author: beefviper
// date: Oct 19, 2026
//
// How long every game takes to load and to play a frame, with no window. Not
// a check (hidden behind the [.speed] tag, so a plain run skips it): run
// `xgetest "[.speed]"` to print the table, and compare a build before and
// after a change. Each game is started with Space and then played for a
// while with the movement keys held in turn, so things get going on screen.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
	using Clock = std::chrono::steady_clock;

	double millisecondsSince(Clock::time_point start)
	{
		return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	}
}

TEST_CASE("Every game's load and frame times", "[.speed]")
{
	std::vector<std::string> games;
	for (const auto& entry : std::filesystem::directory_iterator("descriptions"))
	{
		if (entry.path().extension() == ".xml" && (!std::getenv("ONLY") || entry.path().stem().string() == std::getenv("ONLY"))) { games.push_back(entry.path().stem().string()); }
	}
	std::sort(games.begin(), games.end());

	constexpr int kFrames = 3600;
	const xge::KeyCode keys[] = { xge::KeyCode::D, xge::KeyCode::W, xge::KeyCode::A, xge::KeyCode::S };

	std::printf("%-16s %10s %12s %12s %8s\n", "game", "load ms", "frame us", "worst us", "objects");
	double totalLoad = 0.0;
	double totalFrame = 0.0;
	for (const std::string& name : games)
	{
		const Clock::time_point loadStart = Clock::now();
		played::Played play{ "descriptions/" + name + ".xml" };
		const double load = millisecondsSince(loadStart);

		play.tap(xge::KeyCode::Space);
		double worst = 0.0;
		const Clock::time_point playStart = Clock::now();
		for (int frame = 0; frame < kFrames; ++frame)
		{
			// A key held for a second at a time, Space tapped now and then.
			if (frame % 60 == 0)
			{
				if (frame > 0) { play.engine.handleKeyReleased(keys[(frame / 60 - 1) % 4]); }
				play.engine.handleKeyPressed(keys[(frame / 60) % 4]);
			}
			if (frame % 45 == 0) { play.tap(xge::KeyCode::Space); }

			const Clock::time_point one = Clock::now();
			play.engine.step();
			worst = std::max(worst, millisecondsSince(one));
		}
		const double perFrame = millisecondsSince(playStart) * 1000.0 / kFrames;

		totalLoad += load;
		totalFrame += perFrame;
		std::printf("%-16s %10.1f %12.1f %12.1f %8zu\n", name.c_str(), load, perFrame, worst * 1000.0, play.game.getCurrentObjects().size());
	}
	std::printf("%-16s %10.1f %12.1f\n", "all", totalLoad, totalFrame);
	SUCCEED();
}
