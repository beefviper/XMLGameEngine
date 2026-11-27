// test_prismfall.cpp
// XML Game Engine
// author: beefviper
// date: Nov 27, 2026
//
// Catch2 tests for games/descriptions/prismfall.xml, played frame by frame by a
// real xge::Game with no window: the lattice of shards marching in lockstep,
// the lens's beam breaking one for its worth, sparks shed on timers, and the
// lens cracked and back.

#include "play/executor.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

namespace
{
	// A game in the "playing" state, ready to be stepped a frame at a time.
	struct Sky
	{
		Game game{ "descriptions/prismfall.xml" };
		CommandExecutor executor{ game };

		Sky()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.drawn.picture);
			}
			game.setCurrentState("playing");
		}

		Object& lens() { return game.object("lens"); }
		float score() { return lens().variable.at("score"); }

		int count(const std::string& objClass)
		{
			int found = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && object.exists && object.isVisible) { ++found; }
			}
			return found;
		}

		void press(const char* action)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "lens", action } }, true);
			executor.executeInput(Command{ CmdTriggerAction{ "lens", action } }, false);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(xge::kSixtieth); }
		}
	};
}

TEST_CASE("prismfall.xml loads with four rows of nine shards, three pylons and three lenses", "[prismfall]")
{
	Sky sky;

	CHECK(sky.count("shards") == 36);
	CHECK(sky.count("pylon") == 3);
	CHECK(sky.lens().variable.at("lives") == 3.0f);
	CHECK(sky.game.object("shards.1.1").variable.at("worth") == 40.0f);
	CHECK(sky.game.object("shards.1.2").variable.at("worth") == 25.0f);
	CHECK(sky.game.object("shards.9.4").variable.at("worth") == 15.0f);
}

TEST_CASE("the shards march as one lattice", "[prismfall]")
{
	Sky sky;
	const float gap = sky.game.object("shards.2.1").position.x - sky.game.object("shards.1.1").position.x;
	const float startX = sky.game.object("shards.1.1").position.x;

	sky.frames(120);
	CHECK(sky.game.object("shards.1.1").position.x > startX);
	CHECK(sky.game.object("shards.2.1").position.x - sky.game.object("shards.1.1").position.x == gap);
	CHECK(sky.game.object("shards.9.4").position.x - sky.game.object("shards.8.4").position.x == gap);
}

TEST_CASE("a beam breaks the shard it meets, for that shard's worth", "[prismfall]")
{
	Sky sky;
	Object& lens = sky.lens();
	Object& target = sky.game.object("shards.5.4");

	// The lens right under a cross, the lattice far off the pylons.
	lens.position.x = target.position.x + target.size.x / 2 - lens.size.x / 2;
	sky.press("shine");
	for (int i = 0; i < 120 && sky.count("shards") == 36; ++i) { sky.frames(1); }

	CHECK(sky.count("shards") == 35);
	CHECK(sky.score() == 15.0f);
	CHECK(sky.count("beam") == 0);
}

TEST_CASE("a spark on the lens cracks it, holds the game a second, then brings it back", "[prismfall]")
{
	Sky sky;
	sky.lens().variable.at("struck") = 1;
	sky.lens().variable.at("lives") = 2;
	sky.frames(1);
	CHECK(sky.game.getCurrentState().name == "cracked");

	sky.frames(70);
	CHECK(sky.game.getCurrentState().name == "playing");
	CHECK(sky.lens().variable.at("lives") == 2.0f);
}
