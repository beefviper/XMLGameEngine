// test_collision_geometry.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for CollisionDetector's geometry, called directly on real
// xge::Objects (no window or game needed): which edge is touched when two
// shapes overlap, and - the point of sweep() - when, partway through a step,
// two moving shapes first touch, including when the step is much bigger than
// the thing being hit. (This file used to test a hand-copied version of the
// overlap code against SFML types; it now tests the real thing.)

#include "collision_detector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace xge;
using Catch::Matchers::WithinAbs;

namespace
{
	Object shape(ShapeKind kind, float x, float y, float width, float height)
	{
		Object object;
		object.shapeKind = kind;
		object.position = { x, y };
		object.size = { width, height };
		return object;
	}

	Object rectangle(float x, float y, float width, float height) { return shape(ShapeKind::Rectangle, x, y, width, height); }
	Object circle(float x, float y, float diameter) { return shape(ShapeKind::Circle, x, y, diameter, diameter); }

	const Vector2f still{ 0.0f, 0.0f };
}

TEST_CASE("overlapping rectangles report the edge of the second one that was hit", "[collision_geometry]")
{
	const Object target = rectangle(100, 0, 20, 20);

	CHECK(CollisionDetector::overlap(rectangle(90, 0, 20, 20), target) == Edge::Left);
	CHECK(CollisionDetector::overlap(rectangle(110, 0, 20, 20), target) == Edge::Right);
	CHECK(CollisionDetector::overlap(rectangle(100, -10, 20, 20), target) == Edge::Top);
	CHECK(CollisionDetector::overlap(rectangle(100, 10, 20, 20), target) == Edge::Bottom);

	CHECK_FALSE(CollisionDetector::overlap(rectangle(0, 0, 10, 10), target).has_value());
	CHECK_FALSE(CollisionDetector::overlap(rectangle(80, 0, 20, 20), target).has_value()); // only touching
}

TEST_CASE("a circle reports the edge of the rectangle it is against", "[collision_geometry]")
{
	const Object paddle = rectangle(100, 30, 20, 40);

	CHECK(CollisionDetector::overlap(circle(85, 40, 20), paddle) == Edge::Left);     // centre (95, 50)
	CHECK(CollisionDetector::overlap(circle(115, 40, 20), paddle) == Edge::Right);   // centre (125, 50)
	CHECK(CollisionDetector::overlap(circle(100, 15, 20), paddle) == Edge::Top);     // centre (110, 25)
	CHECK(CollisionDetector::overlap(circle(100, 60, 20), paddle) == Edge::Bottom);  // centre (110, 70)

	// The same, with the circle as the second object: the answer is an edge of the circle.
	CHECK(CollisionDetector::overlap(paddle, circle(85, 40, 20)) == Edge::Right);

	CHECK_FALSE(CollisionDetector::overlap(circle(0, 0, 20), paddle).has_value());
}

TEST_CASE("a circle whose centre is inside the rectangle still reports an edge", "[collision_geometry]")
{
	const Object alien = rectangle(100, 100, 50, 50);

	CHECK(CollisionDetector::overlap(circle(102, 115, 8), alien) == Edge::Left);
	CHECK(CollisionDetector::overlap(circle(138, 115, 8), alien) == Edge::Right);
	CHECK(CollisionDetector::overlap(circle(120, 102, 8), alien) == Edge::Top);
	CHECK(CollisionDetector::overlap(circle(120, 138, 8), alien) == Edge::Bottom);
}

TEST_CASE("a fast rectangle cannot pass through a thin one", "[sweep]")
{
	// 4 wide, 100 px a step, at a wall 2 wide that is 46 px away: the end of the
	// step is well past the wall, so only the path in between shows the hit.
	const Object mover = rectangle(0, 0, 4, 4);
	const Object wall = rectangle(50, -10, 2, 30);

	REQUIRE_FALSE(CollisionDetector::overlap(mover, wall).has_value());

	const auto hit = CollisionDetector::sweep(mover, { 100, 0 }, wall, still);
	REQUIRE(hit.has_value());
	CHECK(hit->edgeOfSecond == Edge::Left);
	CHECK_THAT(hit->time, WithinAbs(0.46f, 1e-5f));
}

TEST_CASE("sweep finds the edge that was actually crossed, from every direction", "[sweep]")
{
	const Object target = rectangle(100, 100, 20, 20);

	CHECK(CollisionDetector::sweep(rectangle(40, 100, 10, 20), { 100, 0 }, target, still)->edgeOfSecond == Edge::Left);
	CHECK(CollisionDetector::sweep(rectangle(200, 100, 10, 20), { -200, 0 }, target, still)->edgeOfSecond == Edge::Right);
	CHECK(CollisionDetector::sweep(rectangle(100, 40, 20, 10), { 0, 100 }, target, still)->edgeOfSecond == Edge::Top);
	CHECK(CollisionDetector::sweep(rectangle(100, 200, 20, 10), { 0, -200 }, target, still)->edgeOfSecond == Edge::Bottom);
}

TEST_CASE("sweep does not report paths that miss or stop short", "[sweep]")
{
	const Object target = rectangle(100, 100, 20, 20);

	CHECK_FALSE(CollisionDetector::sweep(rectangle(0, 0, 10, 10), { 200, 0 }, target, still).has_value());   // passes above it
	CHECK_FALSE(CollisionDetector::sweep(rectangle(0, 100, 10, 20), { 50, 0 }, target, still).has_value());  // ends short of it
	CHECK_FALSE(CollisionDetector::sweep(rectangle(0, 100, 10, 20), { 0, 0 }, target, still).has_value());   // not moving
	CHECK_FALSE(CollisionDetector::sweep(rectangle(0, 100, 10, 20), { -50, 0 }, target, still).has_value()); // moving away
}

TEST_CASE("touching and moving apart is not a hit; touching and moving in is", "[sweep]")
{
	const Object target = rectangle(100, 0, 20, 20);
	const Object touching = rectangle(90, 0, 10, 20); // its right side is exactly on target's left side

	CHECK_FALSE(CollisionDetector::sweep(touching, { -5, 0 }, target, still).has_value());

	const auto in = CollisionDetector::sweep(touching, { 5, 0 }, target, still);
	REQUIRE(in.has_value());
	CHECK(in->time == 0.0f);
	CHECK(in->edgeOfSecond == Edge::Left);
}

TEST_CASE("objects that already overlap are a hit at time zero", "[sweep]")
{
	const auto hit = CollisionDetector::sweep(rectangle(90, 0, 20, 20), { 3, 0 }, rectangle(100, 0, 20, 20), still);
	REQUIRE(hit.has_value());
	CHECK(hit->time == 0.0f);
	CHECK(hit->edgeOfSecond == Edge::Left);
}

TEST_CASE("sweep uses both objects' motion", "[sweep]")
{
	// Nothing moves fast enough alone, but they close on each other: 40 apart,
	// 25 and 25 towards each other, meet after 40 / 50 of the step.
	const auto hit = CollisionDetector::sweep(rectangle(0, 0, 10, 10), { 25, 0 }, rectangle(50, 0, 10, 10), { -25, 0 });
	REQUIRE(hit.has_value());
	CHECK_THAT(hit->time, WithinAbs(0.8f, 1e-5f));
	CHECK(hit->edgeOfSecond == Edge::Left);
}

TEST_CASE("a fast small circle cannot pass through an alien", "[sweep]")
{
	// A bullet 8 px across at 60 px a step against a 50 px alien: its end
	// position is above the alien, so an overlap check sees nothing.
	const Object bullet = circle(120, 300, 8);
	const Object alien = rectangle(100, 200, 50, 50);

	REQUIRE_FALSE(CollisionDetector::overlap(bullet, alien).has_value());
	REQUIRE_FALSE(CollisionDetector::overlap(circle(120, 300 - 120, 8), alien).has_value());

	const auto hit = CollisionDetector::sweep(bullet, { 0, -120 }, alien, still);
	REQUIRE(hit.has_value());
	CHECK(hit->edgeOfSecond == Edge::Bottom);
	CHECK_THAT(hit->time, WithinAbs((300.0f - 250.0f) / 120.0f, 1e-5f));
}

TEST_CASE("a circle is swept the same whichever side of the pair it is on", "[sweep]")
{
	const Object bullet = circle(120, 300, 8);
	const Object alien = rectangle(100, 200, 50, 50);

	// alien first, bullet second: the answer is an edge of the bullet.
	const auto hit = CollisionDetector::sweep(alien, { 0, 120 }, bullet, still);
	REQUIRE(hit.has_value());
	CHECK(hit->edgeOfSecond == Edge::Top); // the alien came down onto the bullet's top
	CHECK_THAT(hit->time, WithinAbs((300.0f - 250.0f) / 120.0f, 1e-5f));
}

TEST_CASE("a circle passing a corner only hits it if it comes close enough", "[sweep]")
{
	const Object block = rectangle(100, 100, 50, 50);

	// Centre travelling along y = 96 (radius 5) is 4 from the top-left corner's
	// height: it clips the block. Along y = 90 it is clear.
	const auto clip = CollisionDetector::sweep(circle(45, 91, 10), { 200, 0 }, block, still);
	REQUIRE(clip.has_value());
	CHECK(clip->edgeOfSecond == Edge::Top); // it lands mostly on top of the corner
	CHECK_FALSE(CollisionDetector::sweep(circle(45, 85, 10), { 200, 0 }, block, still).has_value());

	// Diagonally past the corner, inside the box that is the block grown by the
	// radius but more than a radius from the corner itself: a miss. Closer in,
	// a hit.
	CHECK_FALSE(CollisionDetector::sweep(circle(55, 126, 10), { 100, -100 }, block, still).has_value());
	CHECK(CollisionDetector::sweep(circle(55, 130, 10), { 100, -100 }, block, still).has_value());
}
