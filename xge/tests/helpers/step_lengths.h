// step_lengths.h
// XML Game Engine
// author: beefviper
// date: Nov 16, 2026
//
// A game played in steps of different lengths, for the checks that it plays
// the same whatever the frame rate: in steps of a tenth, a sixtieth and a
// 240th of a second, and in an uneven mix of steps between those, each for
// the same number of seconds in all.

#pragma once

#include "played.h"
#include "game/random.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace played
{
	// Plays `seconds` in steps of `step`; the last one is cut short so the
	// steps add up to `seconds` exactly.
	inline void playFor(Played& play, double seconds, double step)
	{
		const int whole = static_cast<int>(std::floor(seconds / step + 1e-6));
		for (int i = 0; i < whole; ++i) { play.engine.step(static_cast<float>(step)); }
		const double rest = seconds - whole * step;
		if (rest > 1e-6) { play.engine.step(static_cast<float>(rest)); }
	}

	// Plays `seconds` in steps of any length between a 240th and a tenth of a
	// second, picked from `seed`; the last one is cut short so the steps add
	// up to `seconds` exactly.
	inline void playUneven(Played& play, double seconds, unsigned seed)
	{
		std::mt19937 pick(seed);
		std::uniform_real_distribution<double> length(1.0 / 240.0, 0.1);
		double left = seconds;
		while (left > 1e-6)
		{
			const double step = std::min(left, length(pick));
			play.engine.step(static_cast<float>(step));
			left -= step;
		}
	}

	// One measure of a game, at one way of pacing it.
	struct Paced
	{
		std::string pacing;
		double value;
	};

	// The ways a game is paced: steps of a tenth, a sixtieth and a 240th of
	// a second, and uneven ones.
	inline const std::vector<std::pair<std::string, double>>& pacings()
	{
		static const std::vector<std::pair<std::string, double>> all{
			{ "a tenth", 0.1 }, { "a sixtieth", 1.0 / 60.0 }, { "a 240th", 1.0 / 240.0 }, { "uneven", 0.0 } };
		return all;
	}

	// The game in `file` loaded afresh for each pacing, `start` done to it
	// (a key pressed, say), played for `seconds`, and `measure` asked of it.
	inline std::vector<Paced> atEveryStepLength(const std::string& file, double seconds,
		const std::function<double(Played&)>& measure, const std::function<void(Played&)>& start = {})
	{
		std::vector<Paced> answers;
		for (const auto& [pacing, step] : pacings())
		{
			xge::fixRandomSeed(1);
			Played play{ file };
			if (start) { start(play); }
			if (step > 0.0) { playFor(play, seconds, step); }
			else { playUneven(play, seconds, 7); }
			answers.push_back({ pacing, measure(play) });
		}
		return answers;
	}

	// The same for a game given as text: it is written to a scratch file in
	// descriptions/ (so it can name ../xgedef.xsd) and taken away after.
	inline std::vector<Paced> atEveryStepLengthOf(const std::string& text, double seconds,
		const std::function<double(Played&)>& measure, const std::function<void(Played&)>& start = {})
	{
		const std::string path = scratchName("descriptions/steps_scratch.xml");
		{
			std::ofstream out(path);
			out << text;
		}
		struct Remove
		{
			std::string path;
			~Remove() { std::error_code ignored; std::filesystem::remove(path, ignored); }
		} remove{ path };
		return atEveryStepLength(path, seconds, measure, start);
	}

	// Every answer is within `within` of the one for steps of a sixtieth.
	inline void agree(const std::vector<Paced>& answers, double within)
	{
		const auto sixtieth = std::find_if(answers.begin(), answers.end(), [](const Paced& each) { return each.pacing == "a sixtieth"; });
		REQUIRE(sixtieth != answers.end());
		for (const Paced& each : answers)
		{
			INFO("steps of " << each.pacing << ": " << each.value << ", steps of a sixtieth: " << sixtieth->value);
			CHECK(std::abs(each.value - sixtieth->value) <= within);
		}
	}

	// A game of one state, "playing", with these objects, in per-second
	// numbers (<game time="seconds">), and whatever else the state is given.
	inline std::string secondsGame(const std::string& objects, const std::string& more = "")
	{
		return "<game time=\"seconds\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
			"<window name=\"steps\"><width>800</width><height>600</height><background>color.black</background>"
			"<framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable></variables>"
			"<states><state name=\"playing\"><objects>" + objects + "</objects>"
			"<inputs><input button=\"space\"><pop /></input></inputs>" + more + "</state></states></game>";
	}
}
