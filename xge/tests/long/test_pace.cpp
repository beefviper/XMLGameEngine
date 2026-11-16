// test_pace.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Engine::loop plays one step a frame, of the real time that passed, at no
// more frames a second than the game's <framerate>, whatever the window
// does. These tests wait in real time, so they are part of xgetest-long,
// run on its own, never beside others with ctest -j.

#include "engine/engine.h"
#include "game/game.h"
#include "window/null.h"
#include "scratch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Clock = std::chrono::steady_clock;

namespace
{
	// A window that closes once `open` has passed, counts the steps played
	// (each step() polls it once) and the pictures shown, notes when the
	// last step was, and can take a while to show a picture.
	class TimedWindow : public NullWindow
	{
	public:
		struct Seen
		{
			int steps{};
			int pictures{};
			Clock::time_point lastStep;
		};

		TimedWindow(Clock::duration openFor, std::chrono::milliseconds displayTime, Seen* seen) :
			until(Clock::now() + openFor), slowness(displayTime), noted(seen) {}

		bool isOpen() const override { return !closed && Clock::now() < until; }
		void close() override { closed = true; }

		std::vector<std::pair<KeyCode, bool>> pollEvents() override
		{
			++noted->steps;
			noted->lastStep = Clock::now();
			return {};
		}

		void display() override
		{
			++noted->pictures;
			if (slowness.count() > 0) { std::this_thread::sleep_for(slowness); }
		}

	private:
		Clock::time_point until;
		std::chrono::milliseconds slowness;
		Seen* noted;
		bool closed{ false };
	};

	struct Paced
	{
		// The time the steps covered, from the loop's start to the last step.
		double played{};
		int steps{};
		int pictures{};
		float x{};
	};

	// A mover at 100 pixels a second, in a game of at most 60 frames a
	// second, played through Engine::loop for `seconds` of real time, the
	// window taking `displayTime` to show each picture.
	Paced playMover(double seconds, std::chrono::milliseconds displayTime)
	{
		const std::string path = scratchName("descriptions/pace_scratch.xml");
		{
			std::ofstream out(path);
			out << "<game time=\"seconds\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"pace\"><width>800</width><height>600</height><background>color.black</background>"
				"<framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states><state name=\"playing\"><objects><object name=\"mover\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
				"<position><x>0</x><y>100</y></position><velocity><x>100</x><y>0</y></velocity><collisions><off /></collisions></object></objects>"
				"<inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
		}

		Paced paced;
		{
			Game game{ path };
			TimedWindow::Seen seen;
			Engine engine(game, std::make_unique<TimedWindow>(std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds)), displayTime, &seen));
			const Clock::time_point start = Clock::now();
			engine.loop();
			paced.played = std::chrono::duration<double>(seen.lastStep - start).count();
			paced.steps = seen.steps;
			paced.pictures = seen.pictures;
			paced.x = game.getObject("mover").position.x;
		}
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		return paced;
	}
}

TEST_CASE("a mover goes as far in two real seconds however slow the window is to show a picture", "[long][pace]")
{
	int lastPictures = -1;
	for (const int slowness : { 5, 20, 100 })
	{
		INFO(slowness << " ms a picture");
		const Paced paced = playMover(2.0, std::chrono::milliseconds(slowness));
		INFO("steps " << paced.steps << ", pictures " << paced.pictures << ", played " << paced.played << " s, x " << paced.x);
		// Where the time the steps covered puts it, and about 200.
		CHECK(paced.x == Approx(100.0 * paced.played).margin(5.0));
		CHECK(paced.x == Approx(200.0).margin(15.0));
		// A slower window draws fewer pictures, each step longer.
		CHECK(paced.pictures != lastPictures);
		lastPictures = paced.pictures;
	}
}

TEST_CASE("a game is played no more often than its framerate", "[long][pace]")
{
	const Paced paced = playMover(2.0, std::chrono::milliseconds(0));
	CHECK(paced.steps <= 122);
	CHECK(paced.steps > 100);
	CHECK(paced.pictures <= paced.steps);
	CHECK(paced.x == Approx(100.0 * paced.played).margin(5.0));
}

TEST_CASE("after a stall the game goes on from now, with one step a quarter second long", "[long][pace]")
{
	// Each picture takes half a second; each step after the first (a frame
	// long: no picture was shown before it) is cut to a quarter, so in 3
	// seconds the mover goes about half as far as the time passed.
	const Paced paced = playMover(3.0, std::chrono::milliseconds(500));
	INFO("steps " << paced.steps << ", x " << paced.x);
	CHECK(paced.x == Approx(25.0 * (paced.steps - 1)).margin(3.0));
	CHECK(static_cast<double>(paced.x) < 100.0 * paced.played - 50.0);
}
