// test_prototype_pong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Catch2 tests that the hand-written Pong (games/prototypes/pong_min, the
// example of what the windows-cpp generator is to write) plays as the engine
// plays pong_min.xml: the same keys at the same frames, the same random
// numbers, every object compared every frame. The two share the frame, the
// verbs and the numbers; only who decides what happens differs.

#include "pong_min.h"

#include "engine/engine.h"
#include "game/game.h"
#include "game/random.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <memory>
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

	// Both paddles moved up and down, overlapping, so the ball is met by a
	// paddle now and then and missed now and then.
	const KeyCode script[] = { KeyCode::W, KeyCode::Down, KeyCode::S, KeyCode::Up, KeyCode::S, KeyCode::W, KeyCode::Up, KeyCode::Down };

	constexpr int frames = 3600;

	// The key that goes down (true) or up (false) at this frame, if any.
	template <typename Press>
	void keysAt(int frame, Press press)
	{
		const KeyCode key = script[static_cast<std::size_t>(frame / 30) % std::size(script)];
		if (frame % 30 == 0) { press(key, true); }
		if (frame % 30 == 20) { press(key, false); }
	}

	std::vector<std::string> playFile()
	{
		Game game{ "descriptions/pong_min.xml" };
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

	std::vector<std::string> playByHand()
	{
		pong_min::Pong game;
		NullWindow window(game.window);
		window.init(game.objects);
		HeldKeys keys;
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			keysAt(frame, [&](KeyCode key, bool down)
				{
					if (!down) { keys.release(key); game.key(key, false); }
					else if (keys.press(key)) { game.key(key, true); }
				});
			game.frame();
			shots.push_back(snapshot(game.objects));
		}
		return shots;
	}

	struct SeedReset
	{
		std::optional<unsigned int> was{ fixedRandomSeed() };
		~SeedReset() { fixRandomSeed(was); }
	};
}

TEST_CASE("the hand-written pong_min plays frame for frame as the engine plays pong_min.xml", "[prototype]")
{
	const SeedReset reset;
	int serves = 0;
	int returns = 0;

	for (unsigned int seed = 1; seed <= 10; ++seed)
	{
		INFO("seed " << seed);

		fixRandomSeed(seed);
		const std::vector<std::string> expected = playFile();

		fixRandomSeed(seed);
		const std::vector<std::string> played = playByHand();

		REQUIRE(played.size() == expected.size());
		for (std::size_t frame = 0; frame < expected.size(); ++frame)
		{
			if (played[frame] != expected[frame])
			{
				INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\nby hand:\n" << played[frame]);
				FAIL("the hand-written game played differently");
			}
		}

		// What was played: a serve draws the ball's angle again; a return
		// turns the ball round.
		for (std::size_t frame = 1; frame < played.size(); ++frame)
		{
			const auto angleOf = [](const std::string& shot) { return shot.substr(shot.find(" angle="), shot.find(" side=") - shot.find(" angle=")); };
			const auto movingX = [](const std::string& shot) { return std::stof(shot.substr(shot.find(" moving ") + 8)); };
			if (angleOf(played[frame]) != angleOf(played[frame - 1])) { ++serves; }
			else if ((movingX(played[frame]) > 0) != (movingX(played[frame - 1]) > 0)) { ++returns; }
		}
	}

	// The keys did play it: balls were returned and points were won.
	CHECK(serves > 0);
	CHECK(returns > 0);
}
