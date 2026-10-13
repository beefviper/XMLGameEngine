// test_lockstep_bounce.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 test for a group in lockstep bouncing off the screen edges
// (Game::updateLockstepObjects), using the invader block of the first Space
// Invaders game (a copy of it kept in invaders_fixture.h) in a real xge::Game.
//
// Regression covered: on every bounce off the right edge the members stored
// before the touching one were shifted three steps and the rest one, so the
// last column of the block drifted 4 px further away each time.

#include "game.h"
#include "invaders_fixture.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <set>
#include <vector>

using namespace xge;

namespace
{
	constexpr float kAlienWidth = 50.0f;
	constexpr float kPadding = 15.0f;

	// The distinct x positions of the block's columns, left to right.
	std::vector<float> columnPositions(Game& game)
	{
		std::set<float> xs;
		for (const auto& object : game.getCurrentObjects())
		{
			if (object.groupName == "aliens")
			{
				xs.insert(object.position.x);
			}
		}
		return { xs.begin(), xs.end() };
	}

	void checkEvenlySpaced(Game& game)
	{
		const auto xs = columnPositions(game);
		REQUIRE(xs.size() == 11);
		for (std::size_t i = 1; i < xs.size(); ++i)
		{
			CHECK(xs[i] - xs[i - 1] == kAlienWidth + kPadding);
		}
	}
}

TEST_CASE("the invader block keeps its column spacing through every bounce", "[lockstep_bounce]")
{
	Game game{ invaders_fixture::path() };
	game.setCurrentState("playing");

	// Window::init() normally measures these; a Game built on its own has {0, 0}.
	for (auto& object : game.getCurrentObjects())
	{
		if (object.groupName == "aliens") { object.size = { kAlienWidth, 50.0f }; }
		else if (object.name == "player") { object.size = { 50.0f, 50.0f }; }
	}

	checkEvenlySpaced(game);

	// Right edge, left edge, right edge again.
	int bounces = 0;
	float direction = game.getObject("aliens").velocity.x;
	for (int frame = 0; frame < 3000 && bounces < 3; ++frame)
	{
		game.updateObjects();

		const float now = game.getObject("aliens").velocity.x;
		if (now != direction)
		{
			direction = now;
			++bounces;
			checkEvenlySpaced(game);
		}
	}

	CHECK(bounces == 3);
}
