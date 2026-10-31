// test_speedway.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for speedway.xml and what it brought: a car's <grip> holding
// it to its heading, <surface>s (grass slowing it, oil letting it slide),
// the computer's cars facing the way their paths go, laps counted only by
// gates crossed in order, and the first car home winning.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	struct Race : Played
	{
		Race() : Played("descriptions/speedway.xml")
		{
			tap(KeyCode::Space);
			// the moment on the grid
			frames(100);
		}

		Vector2f middle(const Object& object) { return object.position + object.size * 0.5f; }
		float speed(const Object& object) { return std::hypot(object.velocity.x, object.velocity.y); }

		// Puts the car's middle at x, y, facing heading, going at speed along it.
		void put(Object& car, float x, float y, float heading, float speed)
		{
			car.position = { x - car.size.x * 0.5f, y - car.size.y * 0.5f };
			car.heading = heading;
			car.showHeading();
			const float radians = heading * 3.14159265f / 180.0f;
			car.velocity = { std::sin(radians) * speed, -std::cos(radians) * speed };
		}
	};
}

TEST_CASE("speedway.xml: after a moment on the grid the race is on, every car on lap 1", "[speedway]")
{
	Race race;
	REQUIRE(race.state() == "playing");
	for (const char* car : { "p1", "p2", "cpu1", "cpu2" })
	{
		CHECK(race.get(car).variable.at("lap") == 1.0f);
		CHECK(race.get(car).heading == Approx(90.0f));
	}
}

TEST_CASE("speedway.xml: a car goes where it points, up to a top speed, on the tarmac", "[speedway][surface]")
{
	Race race;
	Object& car = race.get("p1");
	const Vector2f from = race.middle(car);
	race.engine.handleKeyPressed(KeyCode::W);
	race.frames(60);
	race.engine.handleKeyReleased(KeyCode::W);
	CHECK(car.velocity.x > 3.0f);
	CHECK(car.velocity.y == Approx(0.0f).margin(0.01f));
	CHECK(race.speed(car) < 0.14f / 0.03f);
	CHECK(race.middle(car).x > from.x + 100.0f);
}

TEST_CASE("speedway.xml: grip turns the car's way with it, the grass slows it, and on oil it slides on", "[speedway][surface]")
{
	// Going right at 4 down the top straight, turned to face down: grip on
	// tarmac takes most of the sideways speed off in a few frames.
	{
		Race race;
		Object& car = race.get("p1");
		race.put(car, 300.0f, 180.0f, 90.0f, 4.0f);
		car.heading = 180.0f;
		car.showHeading();
		race.frames(5);
		CHECK(std::abs(car.velocity.x) < 0.8f);
	}
	// The same on the oil: it keeps on sliding the way it was going.
	{
		Race race;
		Object& car = race.get("p1");
		race.put(car, 390.0f, 460.0f, 270.0f, 4.0f);
		car.heading = 0.0f;
		car.showHeading();
		race.frames(5);
		CHECK(car.velocity.x < -3.5f);
	}
	// On the grass, flat out is far slower than on tarmac.
	{
		Race race;
		Object& car = race.get("p1");
		race.put(car, 260.0f, 260.0f, 90.0f, 0.0f);
		race.engine.handleKeyPressed(KeyCode::W);
		race.frames(30);
		race.engine.handleKeyReleased(KeyCode::W);
		CHECK(race.speed(car) < 2.0f);
		CHECK(car.groundDrag == Approx(0.08f));
	}
}

TEST_CASE("speedway.xml: the computer's cars go round their lanes, facing the way they go", "[speedway]")
{
	Race race;
	Object& cpu = race.get("cpu1");
	race.frames(30);
	CHECK(cpu.heading == Approx(90.0f));
	CHECK(cpu.velocity.x == Approx(3.4f));
	// Into the first corner, turning down it.
	race.frames(20);
	CHECK(cpu.heading == Approx(135.0f));
	// Round the whole loop (about 1780 pixels) it keeps to the track, never
	// on the infield, and it has done a lap.
	for (int i = 0; i < 520; ++i)
	{
		race.frames(1);
		const Vector2f at = race.middle(cpu);
		CHECK_FALSE((at.x > 200.0f && at.x < 600.0f && at.y > 240.0f && at.y < 400.0f));
		CHECK((at.x > 80.0f && at.x < 720.0f && at.y > 120.0f && at.y < 520.0f));
	}
	CHECK(cpu.variable.at("lap") == 2.0f);
}

TEST_CASE("speedway.xml: a lap counts only by the gates crossed in order", "[speedway]")
{
	Race race;
	Object& car = race.get("p1");
	// Straight back over the finish line: nothing.
	race.put(car, 400.0f, 160.0f, 270.0f, 3.0f);
	race.frames(10);
	CHECK(car.variable.at("lap") == 1.0f);
	CHECK(car.variable.at("next") == 2.0f);

	// Round by each gate in turn, then the finish line: a lap.
	race.put(car, 660.0f, 300.0f, 180.0f, 3.0f);
	race.frames(12);
	CHECK(car.variable.at("next") == 3.0f);
	race.put(car, 420.0f, 460.0f, 270.0f, 3.0f);
	race.frames(12);
	CHECK(car.variable.at("next") == 4.0f);
	race.put(car, 140.0f, 340.0f, 0.0f, 3.0f);
	race.frames(12);
	CHECK(car.variable.at("next") == 1.0f);
	race.put(car, 360.0f, 160.0f, 90.0f, 3.0f);
	race.frames(12);
	CHECK(car.variable.at("lap") == 2.0f);
	CHECK(car.variable.at("next") == 2.0f);
}

TEST_CASE("speedway.xml: the first car home after three laps wins", "[speedway]")
{
	Race race;
	race.game.setVariable("playing.p2", "lap", 3.0f);
	race.game.setVariable("playing.p2", "next", 1.0f);
	race.put(race.get("p2"), 360.0f, 200.0f, 90.0f, 3.0f);
	race.frames(12);
	REQUIRE(race.state() == "over");
	CHECK(race.get("winner").lookName() == "p2");
	race.tap(KeyCode::Space);
	CHECK(race.state() == "title");
}

TEST_CASE("speedway.xml: a car meets the barriers and bounces off", "[speedway]")
{
	Race race;
	Object& car = race.get("p1");
	race.put(car, 440.0f, 100.0f, 0.0f, 4.0f);
	race.frames(10);
	CHECK(car.velocity.y > 0.0f);
	CHECK(race.middle(car).y > 85.0f);
}
