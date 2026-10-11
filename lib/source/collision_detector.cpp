// collision_detector.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "collision_detector.h"

namespace xge
{
	// What Objects are to the geometry. The geometry itself, and all of the
	// arithmetic, is collide.cpp.

	collide::Body CollisionDetector::bodyOf(const Object& object)
	{
		collide::Body body;
		body.position = object.position;
		body.size = object.size;
		body.circle = object.shapeKind == ShapeKind::Circle;
		body.pixels = object.collisionData.type == CollisionType::Pixel;
		body.picture = object.bitmap.get();
		return body;
	}

	bool CollisionDetector::touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		return collide::touchesScreenEdge(bodyOf(object), windowDesc.width, windowDesc.height, edge);
	}

	std::optional<Edge> CollisionDetector::rectangleRectangle(const Object& a, const Object& b)
	{
		return collide::rectangleRectangle(bodyOf(a), bodyOf(b));
	}

	std::optional<Edge> CollisionDetector::circleRectangle(const Object& circle, const Object& rect)
	{
		return collide::circleRectangle(bodyOf(circle), bodyOf(rect));
	}

	std::optional<Edge> CollisionDetector::overlap(const Object& a, const Object& b)
	{
		return collide::overlap(bodyOf(a), bodyOf(b));
	}

	bool CollisionDetector::usesPixels(const Object& a, const Object& b) noexcept
	{
		return a.collisionData.type == CollisionType::Pixel || b.collisionData.type == CollisionType::Pixel;
	}

	bool CollisionDetector::pixelsOverlap(const Object& a, const Object& b)
	{
		return collide::pixelsOverlap(bodyOf(a), bodyOf(b));
	}

	std::optional<CollisionDetector::SweepHit> CollisionDetector::sweep(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB)
	{
		return collide::sweep(bodyOf(a), moveA, bodyOf(b), moveB);
	}
}
