// test_megamania.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/descriptions/megamania.xml, played through a real xge::Engine
// with no window and the keys a player would press: a wave wrapping round
// the sides and dropping bombs, a shot scoring, a bomb costing a ship, the
// energy running out, the cookies bouncing, the three waves (the scenes of
// the playing state) won one after another, and the pause drawing the game
// still.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	struct Megamania
	{
		Game game{ "descriptions/megamania.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Megamania()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			REQUIRE(wave() == "playing.wave1");
		}

		std::string state() { return game.getCurrentState().name; }
		std::string wave() { return game.currentScene("playing"); }
		Object& enemy(const std::string& scene, int member) { return game.getObject("playing." + scene + ".enemies." + std::to_string(member)); }
		Object& player() { return game.getObject("player"); }
		float energy() { return game.getObject("energy").variable.at("level"); }

		// Stops a wave (a scene: wave1, wave2, wave3) dropping bombs, for the
		// tests that are not about them.
		void noBombs(const std::string& scene)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.groupName == "playing." + scene + ".enemies") { object.timers.clear(); }
			}
		}

		void clear(const std::string& scene)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.groupName == "playing." + scene + ".enemies") { object.isVisible = false; }
			}
		}

		int bombsInPlay()
		{
			int n = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == "playing.bombs" && object.isVisible) { ++n; }
			}
			return n;
		}
	};
}

TEST_CASE("megamania.xml loads, and the hamburgers cross over and come round again", "[megamania]")
{
	Megamania play;
	CHECK(play.state() == "title");
	play.start();
	play.noBombs("wave1");

	Object& burger = play.enemy("wave1", 4);
	const float y = burger.position.y;
	play.frames(120);
	CHECK(burger.isVisible);
	CHECK(burger.position.x < 300.0f); // gone off the right, back in on the left
	CHECK(burger.position.y == Approx(y));
}

TEST_CASE("the wave drops bombs, and a bomb costs a ship", "[megamania]")
{
	Megamania play;
	play.start();
	int most = 0;
	for (int i = 0; i < 300; ++i)
	{
		play.frames(1);
		most = std::max(most, play.bombsInPlay());
	}
	CHECK(most > 0);

	play.noBombs("wave1");
	play.game.resetObject("bombs");
	const float lives = play.player().variable.at("lives");
	Object& bomb = play.game.getObject("bombs.1");
	bomb.position = { play.player().position.x + 10.0f, play.player().position.y - 30.0f };
	bomb.velocity = { 0.0f, 4.0f };
	bomb.isVisible = true;
	bomb.collisionData.enabled = true;
	play.frames(15);
	CHECK(play.player().variable.at("lives") == lives - 1);
	CHECK(play.bombsInPlay() == 0);
}

TEST_CASE("a shot brings one down, one shot at a time", "[megamania]")
{
	Megamania play;
	play.start();
	play.noBombs("wave1");

	Object& burger = play.enemy("wave1", 5);
	burger.velocity = { 0.0f, 0.0f };
	play.player().position.x = burger.position.x + burger.size.x / 2 - play.player().size.x / 2;
	play.tap(KeyCode::Space);
	play.frames(1);
	const Vector2f shot = play.game.getObject("laser").position;
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.game.getObject("laser").position.y < shot.y);
	play.frames(60);
	CHECK_FALSE(burger.isVisible);
	CHECK(play.player().variable.at("score") == 20);
}

TEST_CASE("the energy runs down a second at a time, and running out costs a ship", "[megamania]")
{
	Megamania play;
	play.start();
	play.noBombs("wave1");

	const float full = play.energy();
	play.frames(120);
	CHECK(play.energy() == Approx(full - 2.0f));

	play.game.getObject("energy").variable["level"] = 1;
	play.frames(61);
	CHECK(play.player().variable.at("lives") == 2);
	CHECK(play.energy() == Approx(full));
}

TEST_CASE("the cookies come down at a slant and bounce back up", "[megamania]")
{
	Megamania play;
	play.start();
	play.noBombs("wave1");
	play.clear("wave1");
	play.frames(2);
	REQUIRE(play.wave() == "playing.wave2");
	play.noBombs("wave2");

	Object& cookie = play.enemy("wave2", 1);
	CHECK(cookie.velocity.y > 0.0f);
	cookie.position.y = 450.0f;
	play.frames(20);
	CHECK(cookie.velocity.y < 0.0f);
}

TEST_CASE("each wave cleared is the next with the energy full, and the third wins", "[megamania]")
{
	Megamania play;
	play.start();
	play.frames(300);
	CHECK(play.energy() < 45.0f);

	CHECK_FALSE(play.game.isShown(play.enemy("wave2", 1)));

	play.clear("wave1");
	play.frames(2);
	CHECK(play.state() == "playing");
	CHECK(play.wave() == "playing.wave2");
	CHECK(play.energy() == Approx(45.0f));
	CHECK(play.game.isShown(play.enemy("wave2", 1)));
	CHECK(play.game.isAway(play.enemy("wave1", 1)));

	play.clear("wave2");
	play.frames(2);
	CHECK(play.wave() == "playing.wave3");
	play.clear("wave3");
	play.frames(2);
	CHECK(play.state() == "youwin");
	CHECK(play.game.isStill(play.player()));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.wave() == "playing.wave1");
	CHECK(play.enemy("wave1", 1).isVisible);
}

TEST_CASE("each wave is the kit's formation with its own speed, rows and score", "[megamania]")
{
	Megamania play;
	CHECK(play.enemy("wave1", 1).velocity.x == Approx(2.0f));
	CHECK(play.enemy("wave1", 1).velocity.y == Approx(0.0f));
	CHECK(play.enemy("wave1", 5).position.y == Approx(150.0f));
	CHECK(play.enemy("wave2", 1).velocity.x == Approx(2.0f));
	CHECK(play.enemy("wave2", 1).velocity.y == Approx(1.0f));
	CHECK(play.enemy("wave2", 1).position.y == Approx(60.0f));
	CHECK(play.enemy("wave3", 1).velocity.x == Approx(-3.0f));
	CHECK(play.enemy("wave3", 8).position.y == Approx(170.0f));

	// a bug scores 40
	play.start();
	play.noBombs("wave1");
	play.clear("wave1");
	play.frames(2);
	play.clear("wave2");
	play.frames(2);
	REQUIRE(play.wave() == "playing.wave3");
	play.noBombs("wave3");
	Object& bug = play.enemy("wave3", 5);
	bug.velocity = { 0.0f, 0.0f };
	play.player().position.x = bug.position.x + bug.size.x / 2 - play.player().size.x / 2;
	play.tap(KeyCode::Space);
	play.frames(60);
	CHECK_FALSE(bug.isVisible);
	CHECK(play.player().variable.at("score") == 40);
}

TEST_CASE("paused, the game is drawn still underneath and does not move", "[megamania]")
{
	Megamania play;
	play.start();
	play.noBombs("wave1");
	play.frames(5);

	Object& burger = play.enemy("wave1", 1);
	const float x = burger.position.x;
	const float energy = play.energy();
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	play.frames(120);
	CHECK(burger.position.x == Approx(x));
	CHECK(play.energy() == Approx(energy));
	CHECK(play.game.isStill(burger));
	CHECK(play.game.isStill(play.player()));

	play.tap(KeyCode::P);
	play.frames(1);
	CHECK(play.state() == "playing");
	CHECK(burger.position.x != Approx(x));
}
