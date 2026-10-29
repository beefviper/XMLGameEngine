// collision.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "play/collision.h"

#include <cmath>

namespace xge
{
	namespace
	{
		// What the object touches other objects with: its <hitbox>, or else
		// its picture's box.
		Box boxOf(const Object& object) noexcept
		{
			if (object.collisionData.hasHitbox)
			{
				return { object.position + object.collisionData.hitboxOffset, object.collisionData.hitboxSize };
			}
			return { object.position, object.size };
		}

		bool isRound(const Object& object)
		{
			if (object.collisionData.hasHitbox) { return object.collisionData.hitboxRound; }
			return object.picture.kind == ShapeKind::Circle;
		}

		// Whether the point `local` (from the top left of boxOf) is touched
		// with by `object`: its picture's pixels when its collisions are of
		// type pixel (those its <solid char>s name), otherwise its shape.
		bool solidAt(const Object& object, const Vector2f& local)
		{
			if (object.collisionData.type == CollisionType::Pixel && object.bitmap)
			{
				return object.bitmap->touchesAt(static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)));
			}

			return solidShapeAt(object.size, isRound(object), local);
		}

		bool pixelsOverlapAt(const Object& a, const Object& b)
		{
			return xge::pixelsOverlapAt(boxOf(a), [&](const Vector2f& local) { return solidAt(a, local); },
				boxOf(b), [&](const Vector2f& local) { return solidAt(b, local); });
		}
	}

	bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		// The window's edges are met by the picture, hitbox or not.
		return pastWindowEdge({ object.position, object.size }, { windowDesc.width, windowDesc.height }, edge);
	}

	std::optional<Edge> objectsOverlap(const Object& a, const Object& b)
	{
		const std::optional<Edge> edge = shapesOverlap(boxOf(a), isRound(a), boxOf(b), isRound(b));

		// Touching boxes are only a first look when pixels are what counts.
		if (edge && usesPixels(a, b) && !pixelsOverlap(a, b))
		{
			return std::nullopt;
		}

		return edge;
	}

	bool usesPixels(const Object& a, const Object& b) noexcept
	{
		return a.collisionData.type == CollisionType::Pixel || b.collisionData.type == CollisionType::Pixel;
	}

	bool pixelsOverlap(const Object& a, const Object& b)
	{
		return pixelsOverlapAt(a, b);
	}

	Contact contactBetween(const Object& a, const Object& b, Edge edgeOfB, float time)
	{
		return contactOf(boxOf(a), isRound(a), boxOf(b), isRound(b), edgeOfB, time);
	}

	std::optional<SweepHit> sweepObjects(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB)
	{
		return sweepThings(boxOf(a), isRound(a), [&](const Vector2f& local) { return solidAt(a, local); }, moveA,
			boxOf(b), isRound(b), [&](const Vector2f& local) { return solidAt(b, local); }, moveB, usesPixels(a, b));
	}

	Box reachOver(const Object& object, const Vector2f& move) noexcept
	{
		return sweptBox(boxOf(object), move);
	}
}
