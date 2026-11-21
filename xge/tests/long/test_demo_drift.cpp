// test_demo_drift.cpp
// XML Game Engine
// author: beefviper
// date: Nov 21, 2026
//
// A long test (hidden in xgetest: [.long]): every game recorded for thirty
// seconds of random keys at 1/60, its key times kept to the millisecond as
// a game file keeps them, and played back twice. On the same steps it must
// play out as recorded. On uneven steps, as a window makes them, it is only
// measured: how long it stays the same game (the state and what is shown),
// and how long nothing is more than 8 pixels from where it was, printed.

#include "cli.h"
#include "random_keys.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace
{
	constexpr int kCheckEvery = 30; // frames: half a second
	constexpr int kCheckpoints = 60; // thirty seconds

	struct Seen
	{
		float x;
		float y;
		bool shown;
	};

	struct Moment
	{
		std::string state;
		std::map<std::string, Seen> objects;
	};

	Moment seenIn(xge::Game& game)
	{
		Moment moment{ game.getCurrentState().name, {} };
		for (const xge::Object& object : game.getCurrentObjects())
		{
			moment.objects[object.name] = { object.position.x, object.position.y, game.isShown(object) };
		}
		return moment;
	}

	// Whether two moments are the same game: the same state, and the same
	// things shown.
	bool sameGame(const Moment& recorded, const Moment& played)
	{
		if (recorded.state != played.state) { return false; }
		for (const auto& [name, seen] : recorded.objects)
		{
			const auto other = played.objects.find(name);
			const bool shownThere = other != played.objects.end() && other->second.shown;
			if (seen.shown != shownThere) { return false; }
		}
		return true;
	}

	// How far the furthest thing shown in both is from where it was.
	float furthest(const Moment& recorded, const Moment& played)
	{
		float most = 0;
		for (const auto& [name, seen] : recorded.objects)
		{
			const auto other = played.objects.find(name);
			if (!seen.shown || other == played.objects.end() || !other->second.shown) { continue; }
			most = std::max({ most, std::abs(seen.x - other->second.x), std::abs(seen.y - other->second.y) });
		}
		return most;
	}

	struct Recording
	{
		xge::WrittenDemo demo;
		std::vector<Moment> checkpoints;
	};

	// Thirty seconds of `file` on random keys from `seed`, a key going down
	// or up about one frame in twenty; Pause and Escape are left out.
	Recording record(const std::string& file, unsigned seed)
	{
		std::mt19937 pick(seed);
		played::Played play{ file };
		std::vector<xge::KeyCode> keys;
		for (const xge::KeyCode key : played::keysOf(play.game))
		{
			if (key != xge::KeyCode::Pause && key != xge::KeyCode::Escape) { keys.push_back(key); }
		}

		Recording recording;
		play.engine.record();
		std::set<xge::KeyCode> down;
		for (int frame = 1; frame <= kCheckEvery * kCheckpoints; ++frame)
		{
			if (!keys.empty() && pick() % 20 == 0)
			{
				const xge::KeyCode key = keys[pick() % keys.size()];
				if (down.erase(key)) { play.engine.handleKeyReleased(key); }
				else { down.insert(key); play.engine.handleKeyPressed(key); }
			}
			play.engine.input(); play.engine.update(xge::kSixtieth);
			if (frame % kCheckEvery == 0) { recording.checkpoints.push_back(seenIn(play.game)); }
		}
		recording.demo = play.engine.recorded();
		for (xge::DemoKey& key : recording.demo.keys) { key.seconds = std::round(key.seconds * 1000) / 1000; }
		return recording;
	}

	// Half a second of play on steps from 9 to 24 milliseconds.
	void unevenHalfSecond(xge::Engine& engine, std::size_t& step)
	{
		static constexpr float steps[] = { 0.009f, 0.017f, 0.024f, 0.013f, 0.021f, 0.016f };
		float left = 0.5f;
		while (left > 1e-6f)
		{
			const float seconds = std::min(steps[step++ % std::size(steps)], left);
			engine.input(); engine.update(seconds);
			left -= seconds;
		}
	}
}

TEST_CASE("every game's demo plays back as recorded on its steps, and is measured on uneven ones", "[.long][demos]")
{
	for (const std::string& file : played::shippedGames(xge::gameFilesIn("descriptions")))
	{
		DYNAMIC_SECTION(played::gameName(file))
		{
			const Recording recording = record(file, 1);

			played::Played same{ file };
			same.engine.replay(recording.demo);
			for (const Moment& recorded : recording.checkpoints)
			{
				same.frames(kCheckEvery);
				REQUIRE(sameGame(recorded, seenIn(same.game)));
				REQUIRE(furthest(recorded, seenIn(same.game)) == 0.0f);
			}

			played::Played uneven{ file };
			uneven.engine.replay(recording.demo);
			std::size_t step = 0;
			float sameFor = 0;
			float closeFor = -1;
			for (const Moment& recorded : recording.checkpoints)
			{
				unevenHalfSecond(uneven.engine, step);
				const Moment now = seenIn(uneven.game);
				if (!sameGame(recorded, now)) { break; }
				if (closeFor < 0 && furthest(recorded, now) > 8.0f) { closeFor = sameFor; }
				sameFor += 0.5f;
			}
			if (closeFor < 0) { closeFor = sameFor; }
			std::cout << played::gameName(file) << ": on uneven steps the same game for " << sameFor
				<< " s, within 8 px for " << closeFor << " s\n";
		}
	}
}
