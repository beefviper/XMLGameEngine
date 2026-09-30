// test_collision_rules.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for object-against-object <collision> rules, loaded from the
// shipped games by a real xge::Game. A rule can be written with just a
// class (or object) to apply to - <collision class="aliens"><die /></collision> -
// and a <collision> with no selector at all is the rule that matches anything.
//
// Regression covered: Space Invaders' bullet used an unfiltered collision
// rule, so it died the moment it overlapped the ship that fired it.

#include "game.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

TEST_CASE("a collision with no selector still matches anything", "[collision_rules]")
{
	Game game{ "games/pong.xml" };
	const Object& ball = game.getObject("ball");

	// pong.xml's ball: <collision><bounce /></collision>
	REQUIRE(ball.collisionData.basic.size() == 1);
	CHECK(ball.collisionData.basic[0].filterClass.empty());
	CHECK(ball.collisionData.basic[0].filterObject.empty());
}

TEST_CASE("a collision naming only a class is an object rule for that class", "[collision_rules]")
{
	Game game{ "games/spaceinvaders.xml" };
	const Object& bullet = game.getObject("bullet");

	// spaceinvaders.xml's bullet: <collision class="aliens"><die /></collision>
	REQUIRE(bullet.collisionData.basic.size() == 1);
	CHECK(bullet.collisionData.basic[0].filterClass == "aliens");
	CHECK(bullet.collisionData.basic[0].filterObject.empty());

	// ...and it did not also turn into a screen-edge rule.
	CHECK(bullet.collisionData.left.size() == 1); // only the edge="all" <die />
}

TEST_CASE("a bullet fired from the ship survives touching it", "[collision_rules]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	Object& player = game.getObject("player");
	Object& bullet = game.getObject("bullet");
	player.size = { 50.0f, 50.0f };
	bullet.size = { 8.0f, 8.0f };

	// The engine spawns a bullet with its top-left corner at the ship's top
	// edge, so it starts out overlapping the ship and only clears it a few
	// frames later (6 px a frame, 8 px tall).
	bullet.position = { player.position.x + 25.0f, player.position.y };
	bullet.velocity.y = -6.0f;
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	for (int frame = 0; frame < 4; ++frame)
	{
		game.updateObjects();
		CHECK(bullet.collisionData.enabled);
		CHECK(bullet.isVisible);
	}
	CHECK(bullet.position.y < player.position.y - bullet.size.y);
}

TEST_CASE("a bullet dies on touching an alien", "[collision_rules]")
{
	Game game{ "games/spaceinvaders.xml" };
	game.setCurrentState("playing");

	Object& bullet = game.getObject("bullet");
	Object& alien = game.getObject("aliens");
	alien.size = { 50.0f, 50.0f };
	bullet.size = { 8.0f, 8.0f };

	// Just under the alien, its centre a couple of pixels below the bottom
	// edge, so it overlaps by less than its radius.
	bullet.position = { alien.position.x + 20.0f, alien.position.y + alien.size.y - 2.0f };
	bullet.velocity.y = -6.0f;
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	game.updateObjects();

	CHECK_FALSE(bullet.collisionData.enabled);
}
