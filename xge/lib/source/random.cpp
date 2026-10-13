// random.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#include "random.h"

#include <random>
#include <utility>

namespace xge
{
	namespace
	{
		std::mt19937& generator()
		{
			static std::mt19937 numbers;
			return numbers;
		}

		std::optional<unsigned int>& fixedSeed()
		{
			static std::optional<unsigned int> seed;
			return seed;
		}
	}

	void seedRandom()
	{
		static std::random_device device;
		generator().seed(fixedSeed() ? *fixedSeed() : device());
	}

	void fixRandomSeed(std::optional<unsigned int> value) noexcept
	{
		fixedSeed() = value;
	}

	float randomBetween(float min, float max)
	{
		if (min > max) { std::swap(min, max); }
		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(generator());
	}

	std::mt19937 randomState()
	{
		return generator();
	}

	void setRandomState(const std::mt19937& state)
	{
		generator() = state;
	}
}
