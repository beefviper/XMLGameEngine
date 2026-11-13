// numbers.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026

#pragma once

#include <cmath>

namespace xge
{
	// The operations of one number a game's values are written with
	// (<sign>, <sine>, <cosine>), worked out the one way: load/expr calls
	// these for a game file, and a generated game calls them for its own
	// numbers, so the two come out the same to the last bit.

	// <sign>: 1 above 0, -1 below, 0 at 0.
	inline float sign(float x) noexcept
	{
		return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f;
	}

	// The degrees every angle in a game is in, as radians.
	inline float radiansOf(float degrees) noexcept
	{
		constexpr float kRadiansPerDegree = 3.14159265358979323846f / 180.0f;
		return degrees * kRadiansPerDegree;
	}

	// <sine> and <cosine> of an angle in degrees.
	inline float sine(float degrees) noexcept { return std::sin(radiansOf(degrees)); }
	inline float cosine(float degrees) noexcept { return std::cos(radiansOf(degrees)); }
}
