// test_step_lengths.cpp
// XML Game Engine
// author: beefviper
// date: Nov 16, 2026
//
// A game plays the same whatever the length of its steps: each case plays a
// small game for some seconds in steps of a tenth, a sixtieth and a 240th of
// a second and in uneven ones (helpers/step_lengths.h), and what it measures
// agrees across all four.

#include "step_lengths.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace played;

TEST_CASE("a still object is in the same place however the time is cut into steps", "[steps]")
{
	const std::string game = secondsGame(box("still", 100, 200));

	const auto answers = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("still").position.x); });
	agree(answers, 0.0);
	CHECK(answers.front().value == 100.0);
}

TEST_CASE("a timer goes off as often as the time passed owes it, however the time is cut into steps", "[steps][timers]")
{
	const std::string clock = box("clock", 0, 0,
		"<variables><variable name=\"n\">0</variable><variable name=\"ticks\">0</variable></variables>"
		"<timers><timer><every>0.05</every><inc variable=\"clock.n\" /></timer>"
		"<timer name=\"left\"><after>3</after><inc variable=\"clock.n\">1000</inc></timer></timers>");
	const std::string game = secondsGame(clock,
		"<timers><timer><every>0.25</every><inc variable=\"clock.ticks\" /></timer></timers>");

	// The object's every twentieth of a second: 40 in 2 seconds.
	const auto fired = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("n")); });
	agree(fired, 0.0);
	CHECK(fired.front().value == 40.0);

	// The time left of a named one is whole seconds, rounded up.
	const auto left = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("left")); });
	agree(left, 0.0);
	CHECK(left.front().value == 1.0);

	// The state's every quarter second: 8 in 2 seconds.
	const auto ticks = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("ticks")); });
	agree(ticks, 0.0);
	CHECK(ticks.front().value == 8.0);
}
