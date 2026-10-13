// random.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#pragma once

#include <optional>
#include <random>

namespace xge
{
	// The one source of random numbers a game draws from, a <random> in the
	// file or a generated game's draw alike, so the two draw the same numbers
	// in the same order.

	// Starts the numbers over: from the seed fixRandomSeed gave, or, with
	// none, from a new one each time. A game calls it once as it loads.
	void seedRandom();

	// Every seedRandom() after this starts from `value`, the same way each
	// time, so two copies of a game play alike (a test of a generated game
	// against its file); nullopt goes back to a new start each time.
	void fixRandomSeed(std::optional<unsigned int> value) noexcept;

	// A number from min to max (either order).
	float randomBetween(float min, float max);

	// Where the numbers are, and putting them back there: a number drawn
	// only to check it (a timer's <random> when the game loads) is then not
	// used up, and the game draws the same numbers as a program that never
	// checked it.
	std::mt19937 randomState();
	void setRandomState(const std::mt19937& state);
}
