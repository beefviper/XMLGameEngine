// collision_detector.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "collision_detector.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xge
{
	namespace
	{
		struct Box
		{
			float left{};
			float top{};
			float right{};
			float bottom{};
		};

		Box boxOf(const Object& object)
		{
			return { object.position.x, object.position.y, object.position.x + object.size.x, object.position.y + object.size.y };
		}

		Vector2f centreOf(const Object& object)
		{
			return object.position + Vector2f(object.size.x / 2, object.size.y / 2);
		}

		// Where a straight path first enters a box, as a fraction of the path
		// (0 = its start, 1 = its end), and whether it came in across a
		// vertical side (moving along x) or a horizontal one.
		struct Entry
		{
			float time{};
			bool acrossX{};
		};

		std::optional<Entry> pathIntoBox(const Vector2f& origin, const Vector2f& move, const Box& box)
		{
			float enter = -std::numeric_limits<float>::infinity();
			float leave = std::numeric_limits<float>::infinity();
			bool acrossX = true;

			// One axis at a time: the stretch of the path spent between that
			// axis' two sides. The path is inside the box while it is inside both.
			const auto slab = [&](float from, float step, float low, float high, bool isX)
			{
				if (step == 0.0f)
				{
					return from > low && from < high;
				}

				float t1 = (low - from) / step;
				float t2 = (high - from) / step;
				if (t1 > t2) { std::swap(t1, t2); }

				if (t1 > enter)
				{
					enter = t1;
					acrossX = isX;
				}
				leave = std::min(leave, t2);
				return true;
			};

			if (!slab(origin.x, move.x, box.left, box.right, true) || !slab(origin.y, move.y, box.top, box.bottom, false))
			{
				return std::nullopt;
			}

			// Never inside on both axes (no motion), a miss, already past it, or
			// not reached before the step ends.
			if (!std::isfinite(enter) || enter >= leave || leave <= 0.0f || enter > 1.0f)
			{
				return std::nullopt;
			}

			return Entry{ std::max(enter, 0.0f), acrossX };
		}

		// The side of the box a path crossing it came in through.
		Edge sideEntered(const Entry& entry, const Vector2f& move)
		{
			if (entry.acrossX)
			{
				return move.x > 0 ? Edge::Left : Edge::Right;
			}

			return move.y > 0 ? Edge::Top : Edge::Bottom;
		}

		std::optional<CollisionDetector::SweepHit> sweepRectangle(const Object& mover, const Vector2f& move, const Object& target)
		{
			// The mover's top-left corner against the target grown by the
			// mover's own size: the corner is inside exactly while they overlap.
			const Box targetBox = boxOf(target);
			const Box grown{ targetBox.left - mover.size.x, targetBox.top - mover.size.y, targetBox.right, targetBox.bottom };

			const auto entry = pathIntoBox(mover.position, move, grown);
			if (!entry)
			{
				return std::nullopt;
			}

			return CollisionDetector::SweepHit{ entry->time, sideEntered(*entry, move) };
		}

		std::optional<CollisionDetector::SweepHit> sweepCircle(const Object& circle, const Vector2f& move, const Object& target)
		{
			const Vector2f start = centreOf(circle);
			const float radius = circle.size.x / 2;
			const Box box = boxOf(target);

			// The circle's centre against the target grown by the radius on every
			// side. That is exact along the flat sides; at a corner the grown box
			// is too generous, so the corner is checked properly below.
			const Box grown{ box.left - radius, box.top - radius, box.right + radius, box.bottom + radius };
			const auto entry = pathIntoBox(start, move, grown);
			if (!entry)
			{
				return std::nullopt;
			}

			const Vector2f at = start + move * entry->time;
			const bool besideSide = (at.x >= box.left && at.x <= box.right) || (at.y >= box.top && at.y <= box.bottom);
			if (besideSide)
			{
				return CollisionDetector::SweepHit{ entry->time, sideEntered(*entry, move) };
			}

			// Coming at a corner: the circle only touches when its centre gets a
			// radius away from that corner point.
			const Vector2f corner{ at.x < box.left ? box.left : box.right, at.y < box.top ? box.top : box.bottom };
			const Vector2f offset = start - corner;
			const float a = move.x * move.x + move.y * move.y;
			const float b = 2 * (offset.x * move.x + offset.y * move.y);
			const float c = offset.x * offset.x + offset.y * offset.y - radius * radius;
			const float discriminant = b * b - 4 * a * c;
			if (a == 0.0f || discriminant < 0.0f)
			{
				return std::nullopt;
			}

			const float time = (-b - std::sqrt(discriminant)) / (2 * a);
			if (time > 1.0f || time < -0.0001f)
			{
				return std::nullopt;
			}

			const Vector2f normal = (start + move * time) - corner;
			Edge edge;
			if (std::abs(normal.x) > std::abs(normal.y))
			{
				edge = normal.x < 0 ? Edge::Left : Edge::Right;
			}
			else
			{
				edge = normal.y < 0 ? Edge::Top : Edge::Bottom;
			}

			return CollisionDetector::SweepHit{ std::max(time, 0.0f), edge };
		}
	}

	namespace
	{
		// Whether the pixel of the screen at (x, y) - taken at its centre - is
		// drawn on by `object` when it is at `position`.
		bool solidAt(const Object& object, const Vector2f& position, int x, int y)
		{
			const float localX = static_cast<float>(x) + 0.5f - position.x;
			const float localY = static_cast<float>(y) + 0.5f - position.y;

			if (object.collisionData.type == CollisionType::Pixel && object.bitmap)
			{
				return object.bitmap->solidAt(static_cast<int>(std::floor(localX)), static_cast<int>(std::floor(localY)));
			}

			if (localX < 0 || localY < 0 || localX >= object.size.x || localY >= object.size.y)
			{
				return false;
			}

			if (object.shapeKind == ShapeKind::Circle)
			{
				const float radius = object.size.x / 2;
				const float dx = localX - radius;
				const float dy = localY - radius;
				return dx * dx + dy * dy <= radius * radius;
			}

			return true;
		}

		bool pixelsOverlapAt(const Object& a, const Vector2f& atA, const Object& b, const Vector2f& atB)
		{
			// Only the stretch of screen both of them cover needs looking at.
			const int left = static_cast<int>(std::floor(std::max(atA.x, atB.x)));
			const int top = static_cast<int>(std::floor(std::max(atA.y, atB.y)));
			const int right = static_cast<int>(std::ceil(std::min(atA.x + a.size.x, atB.x + b.size.x)));
			const int bottom = static_cast<int>(std::ceil(std::min(atA.y + a.size.y, atB.y + b.size.y)));

			for (int y = top; y < bottom; ++y)
			{
				for (int x = left; x < right; ++x)
				{
					if (solidAt(a, atA, x, y) && solidAt(b, atB, x, y))
					{
						return true;
					}
				}
			}

			return false;
		}

		// The side of `b` that something moving by `move` relative to it comes
		// in through, by whichever way it is mostly going.
		Edge edgeFromMotion(const Vector2f& move)
		{
			if (std::abs(move.x) >= std::abs(move.y))
			{
				return move.x > 0 ? Edge::Left : Edge::Right;
			}

			return move.y > 0 ? Edge::Top : Edge::Bottom;
		}

		// After the boxes have been found to touch at `boxHit`: walk the rest
		// of the step, half a pixel of the longer way at a time, until the
		// pixels themselves touch.
		std::optional<CollisionDetector::SweepHit> firstPixelHit(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB,
			const CollisionDetector::SweepHit& boxHit)
		{
			const Vector2f relative = moveA - moveB;
			const float reach = std::max(std::abs(relative.x), std::abs(relative.y)) * (1.0f - boxHit.time);
			const int steps = std::max(1, static_cast<int>(std::ceil(reach / 0.5f)));

			const auto touchesAt = [&](float time)
			{
				return pixelsOverlapAt(a, a.position + moveA * time, b, b.position + moveB * time);
			};

			float previous = boxHit.time;
			for (int i = 0; i <= steps; ++i)
			{
				const float time = boxHit.time + (1.0f - boxHit.time) * static_cast<float>(i) / static_cast<float>(steps);

				if (touchesAt(time))
				{
					// Somewhere between the last look (clear) and this one.
					float low = previous;
					float high = time;
					for (int refine = 0; refine < 8 && i > 0; ++refine)
					{
						const float middle = (low + high) / 2;
						if (touchesAt(middle)) { high = middle; } else { low = middle; }
					}

					return CollisionDetector::SweepHit{ high, edgeFromMotion(relative) };
				}

				previous = time;
			}

			return std::nullopt;
		}
	}

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
		const Vector2f midpoint = centreOf(circle);
		const Box box = boxOf(rect);

		const Vector2f nearest{ std::clamp(midpoint.x, box.left, box.right), std::clamp(midpoint.y, box.top, box.bottom) };
		const Vector2f away = midpoint - nearest;

		if (std::sqrt(away.x * away.x + away.y * away.y) >= circle.size.x / 2)
		{
			return std::nullopt;
		}

		// Centre outside the rectangle: the side it is furthest past is the side
		// it is touching.
		if (away.x != 0.0f || away.y != 0.0f)
		{
			if (std::abs(away.x) > std::abs(away.y))
			{
				return away.x < 0 ? Edge::Left : Edge::Right;
			}

			return away.y < 0 ? Edge::Top : Edge::Bottom;
		}

		// Centre inside it: the closest side is the one it came in through.
		const float toLeft = midpoint.x - box.left;
		const float toRight = box.right - midpoint.x;
		const float toTop = midpoint.y - box.top;
		const float toBottom = box.bottom - midpoint.y;
		const float closest = std::min({ toLeft, toRight, toTop, toBottom });

		if (closest == toLeft) { return Edge::Left; }
		if (closest == toRight) { return Edge::Right; }
		if (closest == toTop) { return Edge::Top; }
		return Edge::Bottom;
	}

	std::optional<Edge> CollisionDetector::overlap(const Object& a, const Object& b)
	{
		std::optional<Edge> edge;

		if (a.shapeKind == ShapeKind::Circle)
		{
			edge = circleRectangle(a, b);
		}
		else if (b.shapeKind == ShapeKind::Circle)
		{
			const auto edgeOfA = circleRectangle(b, a);
			edge = edgeOfA ? std::optional<Edge>(opposite(*edgeOfA)) : std::nullopt;
		}
		else
		{
			edge = rectangleRectangle(a, b);
		}

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
		if (const auto edge = overlap(a, b))
		{
			return SweepHit{ 0.0f, *edge };
		}

		// From a's point of view b is what moves - so hold b still and give a
		// the difference between their motions.
		const Vector2f relative = moveA - moveB;
		if (relative.x == 0.0f && relative.y == 0.0f)
		{
			return std::nullopt;
		}

		std::optional<SweepHit> boxHit;

		if (a.shapeKind == ShapeKind::Circle)
		{
			boxHit = sweepCircle(a, relative, b);
		}
		else if (b.shapeKind == ShapeKind::Circle)
		{
			// Same, the other way round: the circle is b, so it gets the
			// opposite motion, and the edge it hits belongs to a.
			const auto hit = sweepCircle(b, relative * -1.0f, a);
			boxHit = hit ? std::optional<SweepHit>(SweepHit{ hit->time, opposite(hit->edgeOfSecond) }) : std::nullopt;
		}
		else
		{
			boxHit = sweepRectangle(a, relative, b);
		}

		// The boxes never touched: nor did any pixels. They did, and pixels are
		// what counts: they may yet not, or only later in the step.
		if (!boxHit || !usesPixels(a, b))
		{
			return boxHit;
		}

		return firstPixelHit(a, moveA, b, moveB, *boxHit);
	}
}
