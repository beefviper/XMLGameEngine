// collision_detector.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "collision_detector.h"

#include <algorithm>
#include <cmath>

namespace xge
{
	bool CollisionDetector::touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		const auto objectWidth = object.size.x;
		const auto objectHeight = object.size.y;

		constexpr float leftBound = 0;
		constexpr float topBound = 0;
		const float rightBound = windowDesc.width - objectWidth;
		const float bottomBound = windowDesc.height - objectHeight;

		switch (edge)
		{
		case Edge::Left:   return object.position.x < leftBound;
		case Edge::Right:  return object.position.x > rightBound - 1;
		case Edge::Top:    return object.position.y < topBound;
		case Edge::Bottom: return object.position.y > bottomBound;
		}

		return false;
	}

	std::optional<Edge> CollisionDetector::rectangleRectangle(const Object& a, const Object& b)
	{
		// How far each of a's edges has poked past the opposite edge of b.
		// A small positive value on one side means that's the side that just
		// barely made contact - i.e. the edge that was actually hit.
		const float overlapLeft = (a.position.x + a.size.x) - b.position.x;
		const float overlapRight = (b.position.x + b.size.x) - a.position.x;
		const float overlapTop = (a.position.y + a.size.y) - b.position.y;
		const float overlapBottom = (b.position.y + b.size.y) - a.position.y;

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

	std::optional<Edge> CollisionDetector::circleRectangle(const Object& circle, const Object& rect)
	{
		const auto midpoint = circle.position + Vector2f(circle.size.x / 2, circle.size.y / 2);

		const auto rectLeft = rect.position.x;
		const auto rectRight = rectLeft + rect.size.x;
		const auto rectTop = rect.position.y;
		const auto rectBottom = rectTop + rect.size.y;

		Vector2f nearestPoint;
		nearestPoint.x = std::clamp(midpoint.x, rectLeft, rectRight);
		nearestPoint.y = std::clamp(midpoint.y, rectTop, rectBottom);

		const auto rayToNearest = nearestPoint - midpoint;
		const auto distance = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);

		auto overlap = circle.size.x / 2 - distance;
		if (std::isnan(overlap))
		{
			overlap = 0;
		}

		if (overlap <= 0)
		{
			return std::nullopt;
		}

		if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectLeft)
		{
			return Edge::Left;
		}
		if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectRight)
		{
			return Edge::Right;
		}
		if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectTop)
		{
			return Edge::Top;
		}
		if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectBottom)
		{
			return Edge::Bottom;
		}

		return std::nullopt;
	}
}

