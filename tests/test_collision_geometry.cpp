// test_collision_geometry.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// CollisionDetector::rectangleRectangle/circleRectangle take a real
// xge::Object (specifically object.sprite->getGlobalBounds()/getPosition()),
// which needs a constructed sf::Sprite bound to a real sf::Texture - more
// setup than these pure edge/sign checks need. This is a dependency-free
// numeric check of the same edge/sign math - copied from
// CollisionDetector::rectangleRectangle/circleRectangle and
// CommandExecutor::bounceOffEdge in source/collision_detector.cpp and
// source/command_executor.cpp - kept in sync with that code and re-verified
// here rather than re-derived by hand each time it changes. It only touches
// SFML's header-only Vector2/Rect types, so it builds without linking SFML's
// compiled libraries at all.

#include <catch2/catch_test_macros.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace
{
	enum class Edge { Top, Bottom, Left, Right };

	Edge opposite(Edge edge)
	{
		switch (edge)
		{
		case Edge::Left: return Edge::Right;
		case Edge::Right: return Edge::Left;
		case Edge::Top: return Edge::Bottom;
		case Edge::Bottom: return Edge::Top;
		}
		return edge;
	}

	// --- copy of CollisionDetector::rectangleRectangle, taking raw bounds ---
	std::optional<Edge> rectangleRectangle(sf::FloatRect boundsA, sf::FloatRect boundsB)
	{
		const float overlapLeft = (boundsA.position.x + boundsA.size.x) - boundsB.position.x;
		const float overlapRight = (boundsB.position.x + boundsB.size.x) - boundsA.position.x;
		const float overlapTop = (boundsA.position.y + boundsA.size.y) - boundsB.position.y;
		const float overlapBottom = (boundsB.position.y + boundsB.size.y) - boundsA.position.y;

		if (overlapLeft <= 0 || overlapRight <= 0 || overlapTop <= 0 || overlapBottom <= 0)
		{
			return std::nullopt;
		}

		const float overlapX = std::min(overlapLeft, overlapRight);
		const float overlapY = std::min(overlapTop, overlapBottom);

		if (overlapX < overlapY)
		{
			return (overlapLeft < overlapRight) ? Edge::Left : Edge::Right;
		}

		return (overlapTop < overlapBottom) ? Edge::Top : Edge::Bottom;
	}

	// --- copy of CollisionDetector::circleRectangle, taking raw center/radius/rect ---
	std::optional<Edge> circleRectangle(sf::Vector2f circleCenter, float circleRadius, sf::FloatRect rectBounds)
	{
		const auto midpoint = circleCenter;
		const auto rectLeft = rectBounds.position.x;
		const auto rectRight = rectLeft + rectBounds.size.x;
		const auto rectTop = rectBounds.position.y;
		const auto rectBottom = rectTop + rectBounds.size.y;

		sf::Vector2f nearestPoint;
		nearestPoint.x = std::clamp(midpoint.x, rectLeft, rectRight);
		nearestPoint.y = std::clamp(midpoint.y, rectTop, rectBottom);

		const auto rayToNearest = nearestPoint - midpoint;
		const auto distance = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);

		auto overlap = circleRadius - distance;
		if (std::isnan(overlap)) overlap = 0;
		if (overlap <= 0) return std::nullopt;

		if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectLeft) return Edge::Left;
		if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectRight) return Edge::Right;
		if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectTop) return Edge::Top;
		if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectBottom) return Edge::Bottom;
		return std::nullopt;
	}

	// applies bounceOffEdge's sign rule to a velocity, given a *self-relative* edge
	sf::Vector2f bounceOffEdge(sf::Vector2f v, Edge edge)
	{
		switch (edge)
		{
		case Edge::Left:   v.x = std::abs(v.x); break;
		case Edge::Right:  v.x = -std::abs(v.x); break;
		case Edge::Top:    v.y = std::abs(v.y); break;
		case Edge::Bottom: v.y = -std::abs(v.y); break;
		}
		return v;
	}
}

TEST_CASE("rectangle approaching from the left hits the target's Left edge and bounces away", "[collision_geometry]")
{
	// Rectangle A (moving right, vx=+3) approaches rectangle B from the left.
	sf::FloatRect a({ 90.f, 0.f }, { 20.f, 20.f });   // right edge at 110
	sf::FloatRect b({ 100.f, 0.f }, { 20.f, 20.f });  // left edge at 100 -> overlap of 10 on X, full 20 on Y

	auto edgeOfB = rectangleRectangle(a, b);
	REQUIRE(edgeOfB.has_value());
	CHECK(*edgeOfB == Edge::Left);

	sf::Vector2f va{ 3.f, 0.f };
	CHECK(bounceOffEdge(va, opposite(*edgeOfB)).x < 0); // mover A bounces back leftward, away from B

	// If B is ALSO moving (e.g. two bouncy objects), its own bounce correctly
	// reflects using its own current velocity...
	sf::Vector2f vbMoving{ -1.f, 0.f };
	CHECK(bounceOffEdge(vbMoving, *edgeOfB).x > 0); // a *moving* target B reflects rightward, away from A

	// ...but bounceOffEdge reflects existing velocity, it doesn't impart a
	// new one: a target sitting at rest gets no impulse from being hit. None
	// of the 4 sample games rely on a stationary object bouncing (only the
	// always-moving ball/bullet declare 'bounce'), so this is a known,
	// documented limitation rather than a bug to fix here.
	sf::Vector2f vbAtRest{ 0.f, 0.f };
	CHECK(bounceOffEdge(vbAtRest, *edgeOfB).x == 0.f);
}

TEST_CASE("rectangle approaching from the right hits the target's Right edge and bounces away", "[collision_geometry]")
{
	sf::FloatRect a({ 110.f, 0.f }, { 20.f, 20.f });  // left edge at 110
	sf::FloatRect b({ 100.f, 0.f }, { 20.f, 20.f });  // right edge at 120 -> overlap of 10 on X

	auto edgeOfB = rectangleRectangle(a, b);
	REQUIRE(edgeOfB.has_value());
	CHECK(*edgeOfB == Edge::Right);

	sf::Vector2f va{ -3.f, 0.f };
	CHECK(bounceOffEdge(va, opposite(*edgeOfB)).x > 0); // mover A bounces back rightward, away from B
}

TEST_CASE("rectangle approaching from above hits the target's Top edge and bounces away", "[collision_geometry]")
{
	sf::FloatRect a({ 0.f, 90.f }, { 20.f, 20.f });   // bottom edge at 110
	sf::FloatRect b({ 0.f, 100.f }, { 20.f, 20.f });  // top edge at 100

	auto edgeOfB = rectangleRectangle(a, b);
	REQUIRE(edgeOfB.has_value());
	CHECK(*edgeOfB == Edge::Top);

	sf::Vector2f va{ 0.f, 3.f };
	CHECK(bounceOffEdge(va, opposite(*edgeOfB)).y < 0); // mover A bounces back upward, away from B
}

TEST_CASE("far-apart rectangles don't collide", "[collision_geometry]")
{
	sf::FloatRect a({ 0.f, 0.f }, { 10.f, 10.f });
	sf::FloatRect b({ 100.f, 100.f }, { 10.f, 10.f });

	CHECK_FALSE(rectangleRectangle(a, b).has_value());
}

TEST_CASE("a circle approaching from the left hits the target's Left edge and bounces away", "[collision_geometry]")
{
	// Ball radius 10, centered at (95, 50); paddle spans x[100,120] y[30,70].
	sf::Vector2f ballCenter{ 95.f, 50.f };
	float radius = 10.f;
	sf::FloatRect paddle({ 100.f, 30.f }, { 20.f, 40.f });

	auto edgeOfPaddle = circleRectangle(ballCenter, radius, paddle);
	REQUIRE(edgeOfPaddle.has_value());
	CHECK(*edgeOfPaddle == Edge::Left);

	sf::Vector2f ballVel{ 5.f, 0.f };
	CHECK(bounceOffEdge(ballVel, opposite(*edgeOfPaddle)).x < 0); // matches original pong behaviour
}

TEST_CASE("a circle approaching from the right hits the target's Right edge and bounces away", "[collision_geometry]")
{
	// Mirrors the b-is-circle path in Game::checkObjectCollision, which
	// computes circleRectangle(circle, rect) then inverts the edge for 'a'.
	sf::Vector2f ballCenter{ 125.f, 50.f };
	float radius = 10.f;
	sf::FloatRect paddle({ 100.f, 30.f }, { 20.f, 40.f });

	auto edgeOfPaddle = circleRectangle(ballCenter, radius, paddle);
	REQUIRE(edgeOfPaddle.has_value());
	CHECK(*edgeOfPaddle == Edge::Right);

	// edgeOfA (from circleRectangle(ball, paddle)) is "edge of paddle" =
	// Right; the ball's own self-relative edge is opposite(Right) = Left.
	Edge edgeOfB = opposite(*edgeOfPaddle);
	CHECK(edgeOfB == Edge::Left);

	sf::Vector2f ballVel{ -5.f, 0.f };
	CHECK(bounceOffEdge(ballVel, edgeOfB).x > 0);
}
