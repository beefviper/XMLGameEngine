// test_prototype_pong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Catch2 tests that the prototype of a generated game (games/prototypes/pong_min,
// pong_min.xml written as code) plays exactly as the engine plays the file:
// the same keys at the same frames, the same random numbers, every object
// compared every frame. The two share the frame, the verbs and the
// collisions; only who decides what happens differs.

#include "engine.h"
#include "game.h"
#include "game_expr.h"
#include "pong_min.h"
#include "window.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// A window that draws nothing but measures every object as a real one
	// would, so the shapes have their sizes (and so collide) on both sides.
	class MeasuringWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>& objects) override
		{
			for (Object& object : objects)
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
				object.sizeKnown = true;
			}
		}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	std::string snapshot(const std::vector<Object>& objects)
	{
		std::ostringstream out;
		for (const Object& object : objects)
		{
			out << object.name << " at " << object.position.x << ',' << object.position.y
				<< " moving " << object.velocity.x << ',' << object.velocity.y
				<< (object.isVisible ? " shown" : " hidden")
				<< (object.collisionData.enabled ? " touching" : "");
			for (const auto& [name, value] : object.variable)
			{
				out << ' ' << name << '=' << value;
			}
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
		Engine engine(game, std::make_unique<MeasuringWindow>());
		std::vector<std::string> shots;
		for (int frame = 0; frame < frames; ++frame)
		{
			keysAt(frame, [&](KeyCode key, bool down) { if (down) { engine.handleKeyPressed(key); } else { engine.handleKeyReleased(key); } });
			engine.step();
			shots.push_back(snapshot(game.getCurrentObjects()));
		}
		return shots;
	}

	std::vector<std::string> playPrototype()
	{
		pong_min::Pong game;
		MeasuringWindow window;
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
		~SeedReset() { game_expr::fixRandomSeed(std::nullopt); }
	};
}

TEST_CASE("the pong_min prototype plays frame for frame as the engine plays pong_min.xml", "[prototype]")
{
	const SeedReset reset;
	int serves = 0;
	int returns = 0;

	for (unsigned int seed = 1; seed <= 10; ++seed)
	{
		INFO("seed " << seed);

		game_expr::fixRandomSeed(seed);
		const std::vector<std::string> expected = playFile();

		game_expr::fixRandomSeed(seed);
		const std::vector<std::string> played = playPrototype();

		REQUIRE(played.size() == expected.size());
		for (std::size_t frame = 0; frame < expected.size(); ++frame)
		{
			if (played[frame] != expected[frame])
			{
				INFO("frame " << frame << "\nfrom the file:\n" << expected[frame] << "\nprototype:\n" << played[frame]);
				FAIL("the prototype played differently");
			}
		}

		// What was played: a serve redraws the ball's angle; a return turns
		// the ball round while it is in the middle of the field.
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
