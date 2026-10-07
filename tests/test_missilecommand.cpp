// test_missilecommand.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/missilecommand.xml, played through a real
// xge::Engine with no window and the keys a player would press: the missiles
// making for the cities (<chase>), the base aimed at the sight (<aim>), a
// counter-missile bursting at the sight (<release>) and the burst taking the
// missiles that fly into it, a city lost, the waves, and the end.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
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

	Vector2f middleOf(const Object& object)
	{
		return object.position + object.size * 0.5f;
	}

	struct MissileCommand
	{
		Game game{ "games/missilecommand.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		MissileCommand()
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
			REQUIRE(state() == "wave1");
		}

		std::string state() { return game.getCurrentState().name; }
		Object& sight() { return game.getObject("sight"); }
		float status(const char* name) { return game.getObject("status").variable.at(name); }

		// Puts every missile of the wave far up out of the way.
		void holdMissiles(const char* group)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.groupName == group)
				{
					object.position = { 300.0f, -100000.0f };
					object.velocity = { 0.0f, 0.0f };
					object.timers.clear();
				}
			}
		}

		int inPlay(const char* group)
		{
			int n = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == group && object.isVisible) { ++n; }
			}
			return n;
		}
	};
}

TEST_CASE("missilecommand.xml loads, and the missiles make for the cities", "[missilecommand]")
{
	MissileCommand play;
	CHECK(play.state() == "title");
	play.start();
	CHECK(play.inPlay("cities") == 6);

	play.frames(30);
	for (const auto& object : play.game.getCurrentObjects())
	{
		if (object.groupName != "missiles1") { continue; }
		INFO(object.name);
		CHECK(object.position.y < 30.0f * 0.8f);
		CHECK(object.velocity.y > 0.0f);
		CHECK(std::hypot(object.velocity.x, object.velocity.y) == Approx(0.8f));
	}
}

TEST_CASE("a counter-missile flies to the sight and bursts there", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("missiles1");

	// Up and to the left.
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::A);
	play.frames(5);
	const Vector2f aimedAt = middleOf(play.sight());
	CHECK(aimedAt.x < 240.0f);

	play.tap(KeyCode::Space);
	play.frames(1);
	REQUIRE(play.inPlay("abms") == 1);
	const Object& abm = play.game.getObject("abms.1");
	CHECK(abm.velocity.x < 0.0f);
	CHECK(abm.velocity.y < 0.0f);
	CHECK(std::hypot(abm.velocity.x, abm.velocity.y) == Approx(7.0f));

	play.frames(60);
	CHECK(play.inPlay("abms") == 0);
	REQUIRE(play.inPlay("bursts") == 1);
	const Vector2f burst = middleOf(play.game.getObject("bursts.1"));
	// It goes off where the counter-missile first touches the sight.
	CHECK(burst.x == Approx(aimedAt.x).margin(12.0f));
	CHECK(burst.y == Approx(aimedAt.y).margin(12.0f));

	// And it is gone again a moment later.
	play.frames(45);
	CHECK(play.inPlay("bursts") == 0);
}

TEST_CASE("a missile that flies into a burst scores and starts again above the sky", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("missiles1");

	play.tap(KeyCode::Space);
	play.frames(40);
	REQUIRE(play.inPlay("bursts") == 1);
	const Object& burst = play.game.getObject("bursts.1");

	Object& missile = play.game.getObject("missiles1.1");
	missile.position = { middleOf(burst).x - 2.0f, burst.position.y - 30.0f };
	missile.velocity = { 0.0f, 2.0f };
	play.frames(15);
	CHECK(play.status("score") == 25);
	CHECK(play.status("spent") == 1);
	CHECK(missile.position.y < 0.0f);
}

TEST_CASE("a missile that reaches a city destroys it, and losing them all is the end", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("missiles1");

	Object& city = play.game.getObject("cities.1");
	Object& missile = play.game.getObject("missiles1.1");
	missile.position = { middleOf(city).x, city.position.y - 20.0f };
	missile.velocity = { 0.0f, 2.0f };
	play.frames(15);
	CHECK_FALSE(city.isVisible);
	CHECK(play.inPlay("cities") == 5);
	CHECK(play.status("spent") == 1);
	CHECK(play.status("score") == 0);

	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.groupName == "cities") { object.isVisible = false; }
	}
	play.frames(2);
	CHECK(play.state() == "theend");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.inPlay("cities") == 6);
}

TEST_CASE("each wave ends when enough missiles are done with, and the third wins", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("missiles1");

	play.game.getObject("status").variable["spent"] = 12;
	play.frames(2);
	CHECK(play.state() == "wave2");
	CHECK(play.status("spent") == 0);

	play.holdMissiles("missiles2");
	play.game.getObject("status").variable["spent"] = 18;
	play.frames(2);
	CHECK(play.state() == "wave3");

	play.holdMissiles("missiles3");
	play.game.getObject("status").variable["spent"] = 24;
	play.frames(2);
	CHECK(play.state() == "youwin");
}

TEST_CASE("when its city is gone a missile turns for another", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	const auto chasing = play.game.getObject("missiles1.2").timers;
	play.holdMissiles("missiles1");

	// One missile above the first city, heading for it.
	Object& missile = play.game.getObject("missiles1.2");
	const Object& first = play.game.getObject("cities.1");
	missile.timers = chasing;
	missile.position = { middleOf(first).x - 2.0f, 100.0f };
	missile.velocity = { 0.0f, 0.8f };
	play.frames(20);
	CHECK(missile.velocity.x == Approx(0.0f).margin(0.05f));

	play.game.getObject("cities.1").isVisible = false;
	play.frames(20);
	CHECK(missile.velocity.x > 0.1f);
}
