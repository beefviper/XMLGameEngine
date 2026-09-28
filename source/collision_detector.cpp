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
		const auto objectWidth = object.sprite->getLocalBounds().size.x;
		const auto objectHeight = object.sprite->getLocalBounds().size.y;

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

	std::optional<Edge> CollisionDetector::circleRectangle(const Object& circle, const Object& rect)
	{
		const auto midpoint = circle.sprite->getPosition() +
			sf::Vector2f(circle.sprite->getLocalBounds().size.x / 2, circle.sprite->getLocalBounds().size.y / 2);

		const auto rectLeft = rect.sprite->getPosition().x;
		const auto rectRight = rectLeft + rect.sprite->getLocalBounds().size.x;
		const auto rectTop = rect.sprite->getPosition().y;
		const auto rectBottom = rectTop + rect.sprite->getLocalBounds().size.y;

		sf::Vector2f nearestPoint;
		nearestPoint.x = std::clamp(midpoint.x, rectLeft, rectRight);
		nearestPoint.y = std::clamp(midpoint.y, rectTop, rectBottom);

		const auto rayToNearest = nearestPoint - midpoint;
		const auto distance = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);

		auto overlap = circle.sprite->getLocalBounds().size.x / 2 - distance;
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

