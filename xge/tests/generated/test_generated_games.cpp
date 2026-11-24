// test_generated_games.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// Catch2 tests that a generated program plays its game as xgecli does: each
// game in games/ is generated as the build runs (tests.cmake), by the
// stylesheets in games/generators/engine-cpp, and its tree compiled in here.
// Played beside the game read from its file, the same keys pressed at the
// same frames and the same random numbers drawn, every object is compared
// every frame.

#include "engine/engine.h"
#include "game/game.h"
#include "load/expr.h"
#include "generated_games.h"
#include "window/window.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// Everything about a frame that playing shows: the state, and each
	// object's place, motion, whether it is in play, its look and variables.
	std::string snapshot(Game& game)
	{
		std::ostringstream out;
		out << "state " << game.getCurrentState().name << '\n';
		for (const Object& object : game.getCurrentObjects())
		{
			out << object.name << " at " << object.position.x << ',' << object.position.y
				<< " moving " << object.velocity.x << ',' << object.velocity.y
				<< (object.isVisible ? " shown" : " hidden")
				<< (object.collisionsOn ? " touching" : "")
				<< " look " << object.lookName() << " heading " << object.heading;
			for (const auto& [name, value] : object.variable)
			{
				out << ' ' << name << '=' << value;
			}
			out << '\n';
		}
		return out.str();
	}

	// The keys pressed: 1 first (it starts every game, one player), then
	// each of the others held for a while in turn, so the players move,
	// jump and fire.
	const KeyCode script[] = {
		KeyCode::Num1, KeyCode::Left, KeyCode::Space, KeyCode::Right, KeyCode::Up, KeyCode::A,
		KeyCode::Space, KeyCode::D, KeyCode::Down, KeyCode::W, KeyCode::S, KeyCode::Space,
	};

	constexpr int frames = 900;

	// The game played for `frames` frames, a snapshot a frame.
	std::vector<std::string> play(Game& game)
	{
		Engine engine(game, std::make_unique<NullWindow>());
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			const KeyCode key = script[static_cast<std::size_t>(frame / 40) % std::size(script)];
			if (frame % 40 == 0) { engine.handleKeyPressed(key); }
			if (frame % 40 == 25) { engine.handleKeyReleased(key); }
			engine.input(); engine.update(xge::kSixtieth);
			shots.push_back(snapshot(game));
		}
		return shots;
	}

	struct SeedReset
	{
		~SeedReset() { Evaluator::fixRandomSeed(std::nullopt); }
	};
}

TEST_CASE("every generated game plays frame for frame as the engine plays its file", "[generate][generated_games]")
{
	const SeedReset reset;
	for (const GeneratedGame& each : generatedGames)
	{
		DYNAMIC_SECTION(std::filesystem::path(each.file).stem().string())
		{
			INFO(each.file);

			Evaluator::fixRandomSeed(20261011u);
			Game fromFile{ std::string(each.file) };
			const std::vector<std::string> expected = play(fromFile);
			CHECK(std::any_of(expected.begin(), expected.end(), [&](const std::string& shot) { return shot != expected.front(); })); // the keys did play it

			Evaluator::fixRandomSeed(20261011u);
			Game generated{ each.read() };
			const std::vector<std::string> played = play(generated);

			REQUIRE(played.size() == expected.size());
			for (std::size_t frame = 0; frame < expected.size(); ++frame)
			{
				if (played[frame] != expected[frame])
				{
					INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\ngenerated:\n" << played[frame]);
					FAIL("the generated game played differently");
				}
			}
		}
	}
}
