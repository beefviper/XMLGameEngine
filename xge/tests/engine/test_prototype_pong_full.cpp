// test_prototype_pong_full.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that the hand-written pong.xml (games/prototypes/pong, the
// example of what the windows-cpp generator is to write for a game of
// states, texts, sounds and a computer player) plays as the engine plays
// the file: the same keys at the same frames, the same random numbers,
// every object compared every frame, from the title through play, a pause,
// a game over and the next game.

#include "pong.h"

#include "engine/engine.h"
#include "game/game.h"
#include "game/random.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

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
			if (object.picture.kind == ShapeKind::Text) { out << " '" << object.picture.text << "'"; }
			for (const auto& [name, value] : object.variable) { out << ' ' << name << '=' << value; }
			out << '\n';
		}
		return out.str();
	}

	struct Press
	{
		KeyCode key;
		int every;
		int at;
		int held;
	};

	// The player's keys: one of the paddles' keys at a time, each held for
	// 20 frames; Space now and then (to play, and to play again after a game
	// over), a pause and back, the computer made smart and easy again, and a
	// game for two players.
	const KeyCode paddleKeys[] = { KeyCode::W, KeyCode::Down, KeyCode::S, KeyCode::Up, KeyCode::S, KeyCode::W, KeyCode::Up, KeyCode::Down };
	const Press others[] = {
		{ KeyCode::D, 4000, 5, 3 },
		{ KeyCode::Space, 450, 12, 3 },
		{ KeyCode::Pause, 1000, 997, 3 },
		{ KeyCode::Pause, 1000, 37, 3 },
		{ KeyCode::A, 4000, 2005, 3 },
		{ KeyCode::Num2, 6000, 4511, 3 },
	};

	constexpr int frames = 9000;

	template <typename Send>
	void keysAt(int frame, Send send)
	{
		const KeyCode paddle = paddleKeys[static_cast<std::size_t>(frame / 30) % std::size(paddleKeys)];
		if (frame % 30 == 0) { send(paddle, true); }
		if (frame % 30 == 20) { send(paddle, false); }
		for (const Press& press : others)
		{
			if (frame % press.every == press.at) { send(press.key, true); }
			if (frame % press.every == press.at + press.held) { send(press.key, false); }
		}
	}

	std::vector<std::string> playFile(int& overs)
	{
		Game game{ "descriptions/pong.xml" };
		Engine engine(game, std::make_unique<NullWindow>(game.getWindowDesc()));
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			keysAt(frame, [&](KeyCode key, bool down) { if (down) { engine.handleKeyPressed(key); } else { engine.handleKeyReleased(key); } });
			engine.step();
			if (game.getCurrentState().name == "over" && shots.size() > 0 && shots.back().find("state over") == std::string::npos) { ++overs; }
			shots.push_back(snapshot(game.getCurrentObjects()) + (game.getCurrentState().name == "over" ? "state over\n" : ""));
		}
		return shots;
	}

	std::vector<std::string> playByHand()
	{
		pong::Pong game;
		NullWindow window(game.window);
		window.init(game.objects);
		game.place();
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
			for (Object& object : game.objects) { window.prepare(object); }
			game.place();
			(void)game.takeSounds();
			shots.push_back(snapshot(game.objects));
		}
		return shots;
	}

	struct SeedReset
	{
		std::optional<unsigned int> was{ fixedRandomSeed() };
		~SeedReset() { fixRandomSeed(was); }
	};

	std::string withoutState(const std::string& shot)
	{
		const std::size_t at = shot.find("state over\n");
		return at == std::string::npos ? shot : shot.substr(0, at);
	}
}

TEST_CASE("the hand-written pong plays frame for frame as the engine plays pong.xml", "[prototype]")
{
	const SeedReset reset;
	int overs = 0;
	for (unsigned int seed = 1; seed <= 5; ++seed)
	{
		INFO("seed " << seed);

		fixRandomSeed(seed);
		const std::vector<std::string> expected = playFile(overs);

		fixRandomSeed(seed);
		const std::vector<std::string> played = playByHand();

		REQUIRE(played.size() == expected.size());
		for (std::size_t frame = 0; frame < expected.size(); ++frame)
		{
			if (played[frame] != withoutState(expected[frame]))
			{
				INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\nby hand:\n" << played[frame]);
				FAIL("the hand-written game played differently");
			}
		}
	}

	// The keys did play it through: games were won and lost.
	CHECK(overs > 0);
}
