// test_pace.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Engine::loop keeps a game at its <framerate> by the clock, whatever the
// window does. These tests wait in real time, so they are part of
// xgetest-long, run on its own, never beside others with ctest -j.

#include "engine/engine.h"
#include "game/game.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	// A window that closes after a number of frames (each step() polls it
	// once), counts the pictures shown, and can take a while to show one.
	class TimedWindow : public NullWindow
	{
	public:
		TimedWindow(int framesToPlay, std::chrono::milliseconds displayTime, int* framesPlayed, int* picturesShown) :
			framesLeft(framesToPlay), slowness(displayTime), played(framesPlayed), shown(picturesShown) {}

		bool isOpen() const override { return framesLeft > 0; }
		void close() override { framesLeft = 0; }

		std::vector<std::pair<KeyCode, bool>> pollEvents() override
		{
			--framesLeft;
			++*played;
			return {};
		}

		void display() override
		{
			++*shown;
			if (slowness.count() > 0) { std::this_thread::sleep_for(slowness); }
		}

	private:
		int framesLeft;
		std::chrono::milliseconds slowness;
		int* played;
		int* shown;
	};

	struct Paced
	{
		double seconds{};
		int frames{};
		int pictures{};
	};

	// Plays pong (60 frames a second) through Engine::loop for a number of
	// frames, the window taking displayTime to show each picture.
	Paced playPong(int frames, std::chrono::milliseconds displayTime)
	{
		Paced paced;
		Game game{ "descriptions/pong.xml" };
		REQUIRE(game.getWindowDesc().framerate == 60);
		Engine engine(game, std::make_unique<TimedWindow>(frames, displayTime, &paced.frames, &paced.pictures));

		const auto start = std::chrono::steady_clock::now();
		engine.loop();
		paced.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
		return paced;
	}
}

TEST_CASE("120 frames of a 60 frame a second game take two seconds", "[long][pace]")
{
	const Paced paced = playPong(120, std::chrono::milliseconds(0));
	CHECK(paced.frames == 120);
	CHECK(paced.seconds > 1.9);
	CHECK(paced.seconds < 2.2);
	// A window that does not wait for the screen is not drawn more often than the game moves.
	CHECK(paced.pictures <= 120);
}

TEST_CASE("a window slow to show a picture still plays every frame on time, drawing fewer", "[long][pace]")
{
	// 40 ms a picture is under 25 a second: two or three frames are played for each.
	const Paced paced = playPong(120, std::chrono::milliseconds(40));
	CHECK(paced.frames == 120);
	CHECK(paced.seconds > 1.9);
	CHECK(paced.seconds < 2.3);
	CHECK(paced.pictures < 60);
}

TEST_CASE("after a stall the game goes on from now, not racing through the frames it missed", "[long][pace]")
{
	// Each picture takes half a second, far more than the few frames caught up
	// at once, so the game plays one frame a picture: 8 frames (the last closes the
	// window, so 7 pictures) take about 3.5 seconds.
	const Paced paced = playPong(8, std::chrono::milliseconds(500));
	CHECK(paced.frames == 8);
	CHECK(paced.pictures == 7);
	CHECK(paced.seconds > 3.0);
}
