// collision_detector.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "collision_detector.h"

#include <cmath>

namespace xge
{
	namespace
	{
		Box boxOf(const Object& object)
		{
			return { object.position, object.size };
		}

		bool isRound(const Object& object)
		{
			return object.shapeKind == ShapeKind::Circle;
		}

		// Whether the point `local` (from the object's top left) is drawn on by
		// `object`: its picture's pixels when its collisions are of type pixel,
		// otherwise its shape.
		bool solidAt(const Object& object, const Vector2f& local)
		{
			if (object.collisionData.type == CollisionType::Pixel && object.bitmap)
			{
				return object.bitmap->solidAt(static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)));
			}

			return solidShapeAt(object.size, isRound(object), local);
		}

		bool pixelsOverlapAt(const Object& a, const Vector2f& atA, const Object& b, const Vector2f& atB)
		{
			return xge::pixelsOverlapAt(Box{ atA, a.size }, [&](const Vector2f& local) { return solidAt(a, local); },
				Box{ atB, b.size }, [&](const Vector2f& local) { return solidAt(b, local); });
		}
	}

	bool CollisionDetector::touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		return pastWindowEdge(boxOf(object), { windowDesc.width, windowDesc.height }, edge);
	}

	std::optional<Edge> CollisionDetector::rectangleRectangle(const Object& a, const Object& b)
	{
		return boxesOverlap(boxOf(a), boxOf(b));
	}

	std::optional<Edge> CollisionDetector::circleRectangle(const Object& circle, const Object& rect)
	{
		return circleOverlapsBox(boxOf(circle), boxOf(rect));
	}

	std::optional<Edge> CollisionDetector::overlap(const Object& a, const Object& b)
	{
		const std::optional<Edge> edge = shapesOverlap(boxOf(a), isRound(a), boxOf(b), isRound(b));

		// Touching boxes are only a first look when pixels are what counts.
		if (edge && usesPixels(a, b) && !pixelsOverlap(a, b))
		{
			return std::nullopt;
		}

		return edge;
	}

	bool CollisionDetector::usesPixels(const Object& a, const Object& b) noexcept
	{
		return a.collisionData.type == CollisionType::Pixel || b.collisionData.type == CollisionType::Pixel;
	}

	bool CollisionDetector::pixelsOverlap(const Object& a, const Object& b)
	{
		return pixelsOverlapAt(a, a.position, b, b.position);
	}

	std::optional<CollisionDetector::SweepHit> CollisionDetector::sweep(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB)
	{
		return sweepThings(boxOf(a), isRound(a), [&](const Vector2f& local) { return solidAt(a, local); }, moveA,
			boxOf(b), isRound(b), [&](const Vector2f& local) { return solidAt(b, local); }, moveB, usesPixels(a, b));
	}
}
