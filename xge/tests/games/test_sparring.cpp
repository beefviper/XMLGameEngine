// test_sparring.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026
//
// Catch2 tests for games/descriptions/sparring.xml, played frame by frame by
// a real xge::Game with no window: two boxers whose jab's middle frame holds
// a fist (a hitbox on a frame), scored by the rules of the one hit (by= and
// hitbox=).

#include "play/executor.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace xge;

namespace
{
	struct Ring
	{
		Game game{ "descriptions/sparring.xml" };
		CommandExecutor executor{ game };

		Ring()
		{
			game.setCurrentState("playing");
			frames(2);
		}

		Object& boxer(int player) { return game.getObject("boxer" + std::to_string(player)); }
		float points(int player) { return boxer(player).variable["points"]; }

		// The two boxers `gap` apart, the first at 200.
		void apart(float gap)
		{
			boxer(1).position.x = 200.0f;
			boxer(2).position.x = 200.0f + gap;
			frames(1);
		}

		void act(int player, const char* action, bool down = true)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "boxer" + std::to_string(player), action } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(xge::kSixtieth); }
		}
	};
}

TEST_CASE("sparring.xml loads two boxers whose fists are out only mid-jab", "[sparring]")
{
	Ring ring;
	CHECK(ring.boxer(1).objClass == "boxer");
	CHECK(ring.boxer(2).objClass == "boxer");
	CHECK(ring.boxer(1).lookName() == "guard");
	REQUIRE(ring.boxer(1).hitboxes().size() == 1);
	CHECK(ring.boxer(1).hitboxes()[0].name == "body");
	CHECK(ring.boxer(1).frameHitboxes().empty());
	const auto names = ring.boxer(2).hitboxNames();
	CHECK(names.size() == 2);
}

TEST_CASE("sparring.xml: a jab in reach scores, sets the other reeling and knocks him back", "[sparring]")
{
	Ring ring;
	ring.apart(30.0f);
	const float before = ring.boxer(2).position.x;
	ring.act(1, "jab");
	ring.frames(10);
	CHECK(ring.points(1) == 1.0f);
	CHECK(ring.points(2) == 0.0f);
	CHECK(ring.boxer(2).lookName() == "reeling");
	CHECK(ring.boxer(2).position.x == before + 30.0f);

	// The jab over, and the reeling: both back on guard, one point only.
	ring.frames(60);
	CHECK(ring.points(1) == 1.0f);
	CHECK(ring.boxer(1).lookName() == "guard");
	CHECK(ring.boxer(2).lookName() == "guard");

	// Player 2's fist goes the other way.
	ring.apart(30.0f);
	ring.act(2, "jab");
	ring.frames(10);
	CHECK(ring.points(2) == 1.0f);
	CHECK(ring.boxer(1).position.x == 170.0f);
}

TEST_CASE("sparring.xml: a guarded jab scores nothing but still knocks back, and one out of reach misses", "[sparring]")
{
	{
		Ring ring;
		ring.apart(30.0f);
		ring.act(2, "guard");
		ring.frames(1);
		ring.act(1, "jab");
		ring.frames(10);
		CHECK(ring.points(1) == 0.0f);
		CHECK(ring.boxer(2).position.x == 260.0f);
	}
	{
		Ring ring;
		ring.apart(60.0f);
		ring.act(1, "jab");
		ring.frames(30);
		CHECK(ring.points(1) == 0.0f);
		CHECK(ring.boxer(2).position.x == 260.0f);
	}
}

TEST_CASE("sparring.xml: two bodies never pass through each other", "[sparring]")
{
	Ring ring;
	ring.apart(80.0f);
	ring.act(1, "right");
	ring.frames(60);
	ring.act(1, "right", false);
	CHECK(ring.boxer(1).position.x < ring.boxer(2).position.x);
	CHECK(ring.boxer(2).position.x - ring.boxer(1).position.x >= 16.0f);
}

TEST_CASE("sparring.xml: the tenth point wins", "[sparring]")
{
	Ring ring;
	ring.boxer(2).variable["points"] = 9.0f;
	ring.apart(30.0f);
	ring.act(2, "jab");
	ring.frames(10);
	CHECK(ring.points(2) == 10.0f);
	CHECK(ring.game.getCurrentState().name == "over");
	CHECK(ring.game.getObject("over.winner").lookName() == "player2");
}
