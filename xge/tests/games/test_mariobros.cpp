// test_mariobros.cpp
// XML Game Engine
// author: beefviper
// date: Oct 28, 2026
//
// Catch2 tests for Mario Bros. beyond the knock through a floor (which
// test_knock.cpp plays): the phases one after another, a sidestepper's two
// bumps, a fighter fly's hops, the third colour of a pest that got up
// twice, the coin out of a pipe and the bonus phase's coins, and Luigi as a
// second player.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace xge;
using played::Played;
using Catch::Approx;

namespace
{
	constexpr float kLow = 160.0f;      // the top of the low floors
	constexpr float kGround = 208.0f;

	struct Mario : Played
	{
		explicit Mario(KeyCode start = KeyCode::Space) : Played("descriptions/mariobros.xml")
		{
			tap(start);
			REQUIRE(state() == "playing");
			frames(2);
		}

		Object& mario() { return get("mario"); }
		Object& luigi() { return get("luigi"); }
		float kicked() { return get("tally").variable.at("kicked"); }

		void phase(const std::string& name) { game.enterScene("playing." + name); }
		std::string scene() { return game.currentScene("playing"); }

		// One of a phase's pests let out at once, standing still on the low
		// left floor.
		Object& pest(const std::string& name)
		{
			Object& one = get(name);
			game.reveal(one.name, 1);
			one.position = { 40.0f, kLow - one.size.y };
			one.velocity.x = 0.0f;
			frames(5);
			return one;
		}

		// Mario under it on the ground, and up into the floor.
		void bump(const Object& under)
		{
			mario().position = { under.position.x, kGround - mario().size.y };
			mario().velocity = {};
			frames(2);
			tap(KeyCode::Space);
			frames(40);
		}
	};
}

TEST_CASE("mariobros.xml: each phase cleared leads to the next, the bonus phase after two, and the sixth wins", "[mariobros]")
{
	Mario play;
	CHECK(play.scene() == "playing.phase1");
	CHECK(play.get("phase1.label").isVisible);
	play.frames(160);
	CHECK_FALSE(play.get("phase1.label").isVisible);     // PHASE 1 for a moment

	play.game.setVariable("tally", "kicked", 4.0f);
	play.frames(2);
	CHECK(play.scene() == "playing.phase2");
	play.game.setVariable("tally", "kicked", 10.0f);
	play.frames(2);
	CHECK(play.scene() == "playing.phase3");

	// The bonus phase ends when its twenty seconds run out.
	play.frames(19 * 60);
	CHECK(play.scene() == "playing.phase3");
	CHECK(play.get("phase3.clock").variable.at("left") == Approx(1.0f));
	play.frames(70);
	CHECK(play.scene() == "playing.phase4");

	play.game.setVariable("tally", "kicked", 14.0f);
	play.frames(2);
	CHECK(play.scene() == "playing.phase5");
	play.game.setVariable("tally", "kicked", 20.0f);
	play.frames(2);
	CHECK(play.scene() == "playing.phase6");
	play.game.setVariable("tally", "kicked", 24.0f);
	play.frames(2);
	CHECK(play.state() == "youwin");
}

TEST_CASE("mariobros.xml: catching all ten coins of the bonus phase ends it at once", "[mariobros]")
{
	Mario play;
	play.phase("phase3");
	play.frames(2);
	for (int i = 1; i <= 10; ++i)
	{
		Object& coin = play.get("phase3.coins." + std::to_string(i));
		REQUIRE(coin.isVisible);
		play.mario().position = coin.position;
		play.mario().velocity = {};
		play.frames(1);
	}
	play.frames(2);
	CHECK(play.mario().variable.at("score") == Approx(8000.0f));
	CHECK(play.scene() == "playing.phase4");
}

TEST_CASE("mariobros.xml: a sidestepper takes two bumps: the first makes it mad and quicker", "[mariobros]")
{
	Mario play;
	play.phase("phase4");
	Object& crab = play.pest("phase4.sidesteppers.3");
	REQUIRE(crab.lookName() == "walking");
	crab.velocity.x = 0.5f;
	play.frames(1);

	play.bump(crab);
	CHECK(crab.lookName() == "mad");
	CHECK(crab.velocity.x == Approx(1.0f));
	CHECK(play.mario().variable.at("lives") == Approx(3.0f));

	// Back over Mario's head, and the second bump turns it over.
	crab.position.x = 40.0f;
	crab.velocity.x = 0.0f;
	play.frames(30);
	REQUIRE(crab.lookName() == "mad");
	play.bump(crab);
	CHECK(crab.lookName() == "flipped");
}

TEST_CASE("mariobros.xml: a fighter fly hops along, and only one standing on a floor is turned over", "[mariobros]")
{
	Mario play;
	play.phase("phase6");
	Object& fly = play.pest("phase6.fighterflies.3");
	const float standing = fly.position.y;

	// Bumped while held in the air: left alone.
	play.mario().position = { fly.position.x, kGround - play.mario().size.y };
	play.mario().velocity = {};
	play.frames(2);
	play.tap(KeyCode::Space);
	for (int i = 0; i < 40; ++i)
	{
		fly.position.y = standing - 20.0f;
		fly.velocity = { 0.0f, -1.0f };
		play.frames(1);
	}
	CHECK(fly.lookName() == "walking");

	// Let go, it lands and hops: up off the floor and down again.
	play.frames(30);
	float highest = fly.position.y;
	for (int i = 0; i < 90; ++i)
	{
		play.frames(1);
		highest = std::min(highest, fly.position.y);
	}
	CHECK(highest < standing - 10.0f);

	// Standing, it goes over.
	fly.position = { 40.0f, standing };
	fly.velocity = {};
	play.game.become(fly.name, "walking");      // its hop starts over
	play.frames(2);
	play.bump(fly);
	CHECK(fly.lookName() == "flipped");
}

TEST_CASE("mariobros.xml: a pest that gets up by itself turns red, and blue the second time", "[mariobros]")
{
	Mario play;
	Object& creeper = play.pest("phase1.shellcreepers.3");
	play.mario().position.x = 200.0f;

	for (const char* colour : { "angry", "furious" })
	{
		play.game.become(creeper.name, "flipped");
		play.frames(3);
		creeper.timers.at(2).framesLeft = 1;
		play.frames(3);
		REQUIRE(creeper.lookName() == "walking");
		REQUIRE_FALSE(creeper.palettes.empty());
		CHECK(creeper.palettes.at(creeper.palette).name == colour);
		creeper.position.x = 40.0f;
	}
}

TEST_CASE("mariobros.xml: a pest kicked off sends a coin out of a top pipe, worth 800 to whoever catches it", "[mariobros]")
{
	Mario play;
	Object& creeper = play.pest("phase1.shellcreepers.3");
	play.game.become(creeper.name, "flipped");
	play.frames(3);
	play.mario().position = { creeper.position.x - 30.0f, kLow - play.mario().size.y };
	play.mario().velocity = {};
	play.frames(2);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(50);
	play.engine.handleKeyReleased(KeyCode::D);
	REQUIRE_FALSE(creeper.isVisible);
	CHECK(play.kicked() == Approx(1.0f));
	CHECK(play.mario().variable.at("score") == Approx(800.0f));

	REQUIRE(play.has("coin.1"));
	Object& coin = play.get("playing.phase1.coin.1");
	CHECK(coin.position.x < 60.0f);
	CHECK(coin.position.y < 64.0f);
	CHECK(coin.velocity.x > 0.0f);

	play.mario().position = coin.position;
	play.mario().velocity = {};
	play.frames(2);
	CHECK(play.mario().variable.at("score") == Approx(1600.0f));
	CHECK_FALSE(play.has("coin.1"));
}

TEST_CASE("mariobros.xml: one player leaves Luigi out; 2 on the title brings him in on the arrows", "[mariobros]")
{
	Mario one;
	CHECK_FALSE(one.luigi().isVisible);
	CHECK_FALSE(one.get("score2").isVisible);
	one.game.setVariable("mario", "lives", 0.0f);
	one.frames(2);
	CHECK(one.state() == "gameover");

	Mario two(KeyCode::Num2);
	REQUIRE(two.luigi().isVisible);
	CHECK(two.get("score2").isVisible);
	CHECK(two.luigi().variable.at("lives") == Approx(3.0f));
	CHECK(two.luigi().palettes.at(two.luigi().palette).name == "luigi");

	// The arrows walk Luigi, not Mario.
	const float marioWas = two.mario().position.x;
	const float luigiWas = two.luigi().position.x;
	two.engine.handleKeyPressed(KeyCode::Right);
	two.frames(30);
	two.engine.handleKeyReleased(KeyCode::Right);
	CHECK(two.luigi().position.x > luigiWas + 10.0f);
	CHECK(two.mario().position.x == Approx(marioWas));

	// Mario out of lives leaves the game to Luigi; both out is the end.
	two.game.setVariable("mario", "lives", 0.0f);
	two.frames(2);
	CHECK(two.state() == "playing");
	CHECK_FALSE(two.has("mario"));
	two.game.setVariable("luigi", "lives", 0.0f);
	two.frames(2);
	CHECK(two.state() == "gameover");
}

TEST_CASE("mariobros.xml: Luigi's kicks and coins are his own score", "[mariobros]")
{
	Mario play(KeyCode::Num2);
	Object& creeper = play.pest("phase1.shellcreepers.3");
	play.game.become(creeper.name, "flipped");
	play.frames(3);
	play.mario().position.x = 200.0f;
	play.luigi().position = { creeper.position.x - 30.0f, kLow - play.luigi().size.y };
	play.luigi().velocity = {};
	play.frames(2);
	play.engine.handleKeyPressed(KeyCode::Right);
	play.frames(50);
	play.engine.handleKeyReleased(KeyCode::Right);
	REQUIRE_FALSE(creeper.isVisible);
	CHECK(play.luigi().variable.at("score") == Approx(800.0f));
	CHECK(play.mario().variable.at("score") == Approx(0.0f));
}
