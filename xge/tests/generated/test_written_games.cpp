// test_written_games.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that every game windows-cpp writes (written_games.h, from
// scripts/cmake/tests.cmake) plays as the engine plays its file: the same
// keys at the same frames, the same random numbers, every object compared
// every frame, for ten seeds. The written game decides what happens in its
// own code; the frame, the verbs and the numbers are the engine's.

#include "written_games.h"

#include "engine/engine.h"
#include "game/game.h"
#include "game/random.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	std::string snapshot(const std::vector<Object>& objects)
	{
		std::ostringstream out;
		for (const Object& object : objects)
		{
			out << object.name << " at " << object.position.x << ',' << object.position.y
				<< " moving " << object.velocity.x << ',' << object.velocity.y
				<< (object.isVisible ? " shown" : " hidden")
				<< (object.collisionsOn ? " touching" : "");
			for (const auto& [name, value] : object.variable) { out << ' ' << name << '=' << value; }
			out << '\n';
		}
		return out.str();
	}

	// The keys pressed, each held for a while in turn, so the players move.
	const KeyCode script[] = {
		KeyCode::Space, KeyCode::W, KeyCode::Down, KeyCode::S, KeyCode::Up, KeyCode::Left,
		KeyCode::S, KeyCode::W, KeyCode::A, KeyCode::Up, KeyCode::Right, KeyCode::D, KeyCode::Down,
	};

	constexpr int frames = 3600;

	template <typename Press>
	void keysAt(int frame, Press press)
	{
		const KeyCode key = script[static_cast<std::size_t>(frame / 30) % std::size(script)];
		if (frame % 30 == 0) { press(key, true); }
		if (frame % 30 == 20) { press(key, false); }
	}

	std::vector<std::string> playFile(const char* file)
	{
		Game game{ std::string(file) };
		Engine engine(game, std::make_unique<NullWindow>(game.getWindowDesc()));
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			keysAt(frame, [&](KeyCode key, bool down) { if (down) { engine.handleKeyPressed(key); } else { engine.handleKeyReleased(key); } });
			engine.step();
			shots.push_back(snapshot(game.getCurrentObjects()));
		}
		return shots;
	}

	// As the written program's main() plays it: the window measures the
	// objects, and a key goes in once while it is held.
	std::vector<std::string> playWritten(const WrittenGame& each)
	{
		const std::unique_ptr<WrittenPlay> game = each.make();
		NullWindow window(game->window());
		window.init(game->objects());
		HeldKeys keys;
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			keysAt(frame, [&](KeyCode key, bool down)
				{
					if (!down) { keys.release(key); game->key(key, false); }
					else if (keys.press(key)) { game->key(key, true); }
				});
			game->frame();
			shots.push_back(snapshot(game->objects()));
		}
		return shots;
	}

	struct SeedReset
	{
		std::optional<unsigned int> was{ fixedRandomSeed() };
		~SeedReset() { fixRandomSeed(was); }
	};
}

TEST_CASE("every game windows-cpp writes plays frame for frame as the engine plays its file", "[generate][written_games]")
{
	const SeedReset reset;
	for (const WrittenGame& each : writtenGames)
	{
		for (unsigned int seed = 1; seed <= 10; ++seed)
		{
			INFO(each.file << ", seed " << seed);

			fixRandomSeed(seed);
			const std::vector<std::string> expected = playFile(each.file);
			CHECK(expected.front() != expected.back()); // the keys did play it

			fixRandomSeed(seed);
			const std::vector<std::string> played = playWritten(each);

			REQUIRE(played.size() == expected.size());
			for (std::size_t frame = 0; frame < expected.size(); ++frame)
			{
				if (played[frame] != expected[frame])
				{
					INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\nwritten:\n" << played[frame]);
					FAIL("the written game played differently");
				}
			}
		}
	}
}
