// geometry.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#include "play/geometry.h"

#include <limits>
#include <utility>

namespace xge
{
	namespace
	{
		constexpr float pi = 3.14159265358979323846f;

		// The edge of a box that faces the way `away` points out of its
		// middle, by whichever way it mostly goes.
		Edge edgeFacing(const Vector2f& away) noexcept
		{
			if (std::abs(away.x) > std::abs(away.y))
			{
				return away.x < 0 ? Edge::Left : Edge::Right;
			}
			return away.y < 0 ? Edge::Top : Edge::Bottom;
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

			if (!slab(origin.x, move.x, box.left(), box.right(), true) || !slab(origin.y, move.y, box.top(), box.bottom(), false))
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

		std::optional<SweepHit> sweepBox(const Box& mover, const Vector2f& move, const Box& target)
		{
			// The mover's top-left corner against the target grown by the
			// mover's own size: the corner is inside exactly while they overlap.
			const Box grown{ { target.left() - mover.size.x, target.top() - mover.size.y }, target.size + mover.size };

			const auto entry = pathIntoBox(mover.position, move, grown);
			if (!entry)
			{
				return std::nullopt;
			}

			return SweepHit{ entry->time, sideEntered(*entry, move) };
		}

		std::optional<SweepHit> sweepCircle(const Box& circle, const Vector2f& move, const Box& box)
		{
			const Vector2f start = circle.middle();
			const float radius = circle.size.x / 2;

			// The circle's centre against the target grown by the radius on every
			// side. That is exact along the flat sides; at a corner the grown box
			// is too generous, so the corner is checked properly below.
			const Box grown{ { box.left() - radius, box.top() - radius }, { box.size.x + 2 * radius, box.size.y + 2 * radius } };
			const auto entry = pathIntoBox(start, move, grown);
			if (!entry)
			{
				return std::nullopt;
			}

			const Vector2f at = start + move * entry->time;
			const bool besideSide = (at.x >= box.left() && at.x <= box.right()) || (at.y >= box.top() && at.y <= box.bottom());
			if (besideSide)
			{
				return SweepHit{ entry->time, sideEntered(*entry, move) };
			}

			// Coming at a corner: the circle only touches when its centre gets a
			// radius away from that corner point.
			const Vector2f corner{ at.x < box.left() ? box.left() : box.right(), at.y < box.top() ? box.top() : box.bottom() };
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

			return SweepHit{ std::max(time, 0.0f), edgeFacing((start + move * time) - corner) };
		}

		// A circle moving by `move` against one standing still: when their
		// middles first come their two radii apart.
		std::optional<SweepHit> sweepCircles(const Box& mover, const Vector2f& move, const Box& still)
		{
			const Vector2f offset = mover.middle() - still.middle();
			const float reach = (mover.size.x + still.size.x) / 2;
			const float a = move.x * move.x + move.y * move.y;
			const float b = 2 * (offset.x * move.x + offset.y * move.y);
			const float c = offset.x * offset.x + offset.y * offset.y - reach * reach;
			const float discriminant = b * b - 4 * a * c;
			if (a == 0.0f || b >= 0.0f || discriminant < 0.0f)
			{
				return std::nullopt;
			}

			const float time = (-b - std::sqrt(discriminant)) / (2 * a);
			if (time > 1.0f || time < -0.0001f)
			{
				return std::nullopt;
			}

			return SweepHit{ std::max(time, 0.0f), edgeFacing(offset + move * time) };
		}
	}

	std::optional<Edge> boxesOverlap(const Box& a, const Box& b) noexcept
	{
		// How far each of a's edges has poked past the opposite edge of b.
		// A small positive value on one side means that's the side that just
		// barely made contact - i.e. the edge that was actually hit.
		const float overlapLeft = a.right() - b.left();
		const float overlapRight = b.right() - a.left();
		const float overlapTop = a.bottom() - b.top();
		const float overlapBottom = b.bottom() - a.top();

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

	std::optional<Edge> circleOverlapsBox(const Box& circle, const Box& box) noexcept
	{
		const Vector2f midpoint = circle.middle();

		const Vector2f nearest{ std::clamp(midpoint.x, box.left(), box.right()), std::clamp(midpoint.y, box.top(), box.bottom()) };
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
		const float toLeft = midpoint.x - box.left();
		const float toRight = box.right() - midpoint.x;
		const float toTop = midpoint.y - box.top();
		const float toBottom = box.bottom() - midpoint.y;
		const float closest = std::min({ toLeft, toRight, toTop, toBottom });

		if (closest == toLeft) { return Edge::Left; }
		if (closest == toRight) { return Edge::Right; }
		if (closest == toTop) { return Edge::Top; }
		return Edge::Bottom;
	}

	std::optional<Edge> circlesOverlap(const Box& a, const Box& b) noexcept
	{
		const Vector2f away = a.middle() - b.middle();
		const float reach = (a.size.x + b.size.x) / 2;
		if (away.x * away.x + away.y * away.y >= reach * reach)
		{
			return std::nullopt;
		}

		// One right on top of the other: by their boxes.
		if (away.x == 0.0f && away.y == 0.0f)
		{
			return boxesOverlap(a, b);
		}

		return edgeFacing(away);
	}

	std::optional<Edge> shapesOverlap(const Box& a, bool aRound, const Box& b, bool bRound) noexcept
	{
		if (aRound && bRound)
		{
			return circlesOverlap(a, b);
		}

		if (aRound)
		{
			return circleOverlapsBox(a, b);
		}

		if (bRound)
		{
			const auto edgeOfA = circleOverlapsBox(b, a);
			return edgeOfA ? std::optional<Edge>(opposite(*edgeOfA)) : std::nullopt;
		}

		return boxesOverlap(a, b);
	}

	bool sweptBoxesMeet(const Box& a, const Vector2f& moveA, const Box& b, const Vector2f& moveB) noexcept
	{
		constexpr float spare = 1.0f;
		const float aLeft = a.left() + std::min(moveA.x, 0.0f) - spare;
		const float aRight = a.right() + std::max(moveA.x, 0.0f) + spare;
		const float aTop = a.top() + std::min(moveA.y, 0.0f) - spare;
		const float aBottom = a.bottom() + std::max(moveA.y, 0.0f) + spare;
		const float bLeft = b.left() + std::min(moveB.x, 0.0f);
		const float bRight = b.right() + std::max(moveB.x, 0.0f);
		const float bTop = b.top() + std::min(moveB.y, 0.0f);
		const float bBottom = b.bottom() + std::max(moveB.y, 0.0f);
		return aLeft <= bRight && bLeft <= aRight && aTop <= bBottom && bTop <= aBottom;
	}

	std::optional<SweepHit> sweepShapes(const Box& a, bool aRound, const Vector2f& moveA, const Box& b, bool bRound, const Vector2f& moveB) noexcept
	{
		if (const auto edge = shapesOverlap(a, aRound, b, bRound))
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

		if (aRound && bRound)
		{
			return sweepCircles(a, relative, b);
		}

		if (aRound)
		{
			return sweepCircle(a, relative, b);
		}

		if (bRound)
		{
			// Same, the other way round: the circle is b, so it gets the
			// opposite motion, and the edge it hits belongs to a.
			const auto hit = sweepCircle(b, relative * -1.0f, a);
			return hit ? std::optional<SweepHit>(SweepHit{ hit->time, opposite(hit->edgeOfSecond) }) : std::nullopt;
		}

		return sweepBox(a, relative, b);
	}

	bool solidShapeAt(const Vector2f& size, bool round, const Vector2f& local) noexcept
	{
		if (local.x < 0 || local.y < 0 || local.x >= size.x || local.y >= size.y)
		{
			return false;
		}

		if (round)
		{
			const float radius = size.x / 2;
			const float dx = local.x - radius;
			const float dy = local.y - radius;
			return dx * dx + dy * dy <= radius * radius;
		}

		return true;
	}

	Edge edgeFromMotion(const Vector2f& relative) noexcept
	{
		if (std::abs(relative.x) >= std::abs(relative.y))
		{
			return relative.x > 0 ? Edge::Left : Edge::Right;
		}

		return relative.y > 0 ? Edge::Top : Edge::Bottom;
	}

	Vector2f outOf(Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:   return { -1.0f, 0.0f };
		case Edge::Right:  return { 1.0f, 0.0f };
		case Edge::Top:    return { 0.0f, -1.0f };
		case Edge::Bottom: return { 0.0f, 1.0f };
		}

		return {};
	}

	namespace
	{
		// Where along that side of the box the point is, 0 at its left or
		// top end and 1 at its right or bottom end.
		float alongSide(const Box& box, Edge edge, const Vector2f& point) noexcept
		{
			const bool across = edge == Edge::Top || edge == Edge::Bottom;
			const float length = across ? box.size.x : box.size.y;
			if (length <= 0.0f) { return 0.5f; }
			return std::clamp(across ? (point.x - box.left()) / length : (point.y - box.top()) / length, 0.0f, 1.0f);
		}

		// The point of the box nearest to `point`.
		Vector2f nearestIn(const Box& box, const Vector2f& point) noexcept
		{
			return { std::clamp(point.x, box.left(), box.right()), std::clamp(point.y, box.top(), box.bottom()) };
		}

		float lengthOf(const Vector2f& v) noexcept
		{
			return std::hypot(v.x, v.y);
		}

		// A circle (its middle and radius) against a box: the point on the box
		// they meet at, the way out of the box towards the circle, and how far
		// in the circle is. With the middle inside the box, out through
		// `edge`, the side of the box it came in by.
		void circleAgainstBox(const Vector2f& middle, float radius, const Box& box, Edge edge, Vector2f& point, Vector2f& normal, float& depth) noexcept
		{
			point = nearestIn(box, middle);
			const Vector2f away = middle - point;
			const float distance = lengthOf(away);
			if (distance > 0.0001f)
			{
				normal = away * (1.0f / distance);
				depth = std::max(0.0f, radius - distance);
				return;
			}

			normal = outOf(edge);
			switch (edge)
			{
			case Edge::Left:   point.x = box.left(); break;
			case Edge::Right:  point.x = box.right(); break;
			case Edge::Top:    point.y = box.top(); break;
			case Edge::Bottom: point.y = box.bottom(); break;
			}
			depth = radius + lengthOf(middle - point);
		}
	}

	Contact contactOf(const Box& a, bool aRound, const Box& b, bool bRound, Edge edgeOfSecond, float time) noexcept
	{
		Contact contact;
		contact.edgeOfSecond = edgeOfSecond;
		contact.time = time;

		if (aRound && bRound)
		{
			const float radiusA = a.size.x / 2;
			const float radiusB = b.size.x / 2;
			const Vector2f away = a.middle() - b.middle();
			const float distance = lengthOf(away);
			contact.normal = distance > 0.0001f ? away * (1.0f / distance) : outOf(edgeOfSecond);
			contact.point = b.middle() + contact.normal * radiusB;
			contact.depth = std::max(0.0f, radiusA + radiusB - distance);
		}
		else if (aRound)
		{
			circleAgainstBox(a.middle(), a.size.x / 2, b, edgeOfSecond, contact.point, contact.normal, contact.depth);
		}
		else if (bRound)
		{
			// The circle is the second: the way out of it is the way into the
			// box, turned round.
			Vector2f intoBox;
			circleAgainstBox(b.middle(), b.size.x / 2, a, opposite(edgeOfSecond), contact.point, intoBox, contact.depth);
			contact.normal = intoBox * -1.0f;
		}
		else
		{
			// Two boxes meet along a stretch of the touched side: its middle.
			contact.normal = outOf(edgeOfSecond);
			const float left = std::max(a.left(), b.left());
			const float right = std::min(a.right(), b.right());
			const float top = std::max(a.top(), b.top());
			const float bottom = std::min(a.bottom(), b.bottom());
			switch (edgeOfSecond)
			{
			case Edge::Top:
				contact.point = { (left + right) / 2, b.top() };
				contact.depth = a.bottom() - b.top();
				break;
			case Edge::Bottom:
				contact.point = { (left + right) / 2, b.bottom() };
				contact.depth = b.bottom() - a.top();
				break;
			case Edge::Left:
				contact.point = { b.left(), (top + bottom) / 2 };
				contact.depth = a.right() - b.left();
				break;
			case Edge::Right:
				contact.point = { b.right(), (top + bottom) / 2 };
				contact.depth = b.right() - a.left();
				break;
			}
			contact.depth = std::max(0.0f, contact.depth);
		}

		contact.alongSecond = alongSide(b, edgeOfSecond, contact.point);
		contact.alongFirst = alongSide(a, opposite(edgeOfSecond), contact.point);
		return contact;
	}

	Touch touchOfFirst(const Contact& contact) noexcept
	{
		return { opposite(contact.edgeOfSecond), contact.point, contact.normal, contact.alongSecond, contact.depth, contact.time };
	}

	Touch touchOfSecond(const Contact& contact) noexcept
	{
		return { contact.edgeOfSecond, contact.point, contact.normal * -1.0f, contact.alongFirst, contact.depth, contact.time };
	}

	bool pastWindowEdge(const Box& thing, const Vector2f& window, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:   return thing.position.x < 0;
		case Edge::Right:  return thing.position.x > window.x - thing.size.x - 1;
		case Edge::Top:    return thing.position.y < 0;
		case Edge::Bottom: return thing.position.y > window.y - thing.size.y;
		}

		return false;
	}

	bool insideWindow(const Box& at, const Vector2f& window) noexcept
	{
		return at.left() >= 0 && at.top() >= 0 && at.right() <= window.x && at.bottom() <= window.y;
	}

	Vector2f bounced(const Vector2f& velocity, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:   return { std::abs(velocity.x), velocity.y };
		case Edge::Right:  return { -std::abs(velocity.x), velocity.y };
		case Edge::Top:    return { velocity.x, std::abs(velocity.y) };
		case Edge::Bottom: return { velocity.x, -std::abs(velocity.y) };
		}

		return velocity;
	}

	Vector2f deflected(const Vector2f& velocity, const Box& mover, const Box& other, Edge edge, float maxDegrees) noexcept
	{
		const float speed = std::hypot(velocity.x, velocity.y);

		const Vector2f middle = mover.middle();
		const Vector2f otherMiddle = other.middle();

		const bool side = (edge == Edge::Left || edge == Edge::Right);
		const float offset = side ? middle.y - otherMiddle.y : middle.x - otherMiddle.x;
		const float half = (side ? other.size.y : other.size.x) * 0.5f;
		const float along = (half > 0.0f) ? std::clamp(offset / half, -1.0f, 1.0f) : 0.0f;

		const float radians = maxDegrees * along * pi / 180.0f;
		const float away = speed * std::cos(radians);
		const float across = speed * std::sin(radians);

		switch (edge)
		{
		case Edge::Left:   return { away, across };
		case Edge::Right:  return { -away, across };
		case Edge::Top:    return { across, away };
		case Edge::Bottom: return { across, -away };
		}

		return velocity;
	}

	std::optional<Knocked> knockedApart(const Vector2f& first, float firstMass, const Vector2f& second, float secondMass, const Vector2f& way, float bounce) noexcept
	{
		const auto dot = [](const Vector2f& a, const Vector2f& b) { return a.x * b.x + a.y * b.y; };
		const float firstAlong = dot(first, way);
		const float secondAlong = dot(second, way);
		const float closing = firstAlong - secondAlong;
		if (!(closing > 0.0f))
		{
			return std::nullopt;
		}

		float firstAfter = firstAlong;
		float secondAfter = secondAlong;
		if (firstMass > 0.0f && secondMass > 0.0f)
		{
			const float carried = firstMass * firstAlong + secondMass * secondAlong;
			firstAfter = (carried - secondMass * bounce * closing) / (firstMass + secondMass);
			secondAfter = (carried + firstMass * bounce * closing) / (firstMass + secondMass);
		}
		else if (secondMass > 0.0f)
		{
			secondAfter = firstAlong + bounce * closing;
		}
		else if (firstMass > 0.0f)
		{
			firstAfter = secondAlong - bounce * closing;
		}

		return Knocked{ first + way * (firstAfter - firstAlong), second + way * (secondAfter - secondAlong) };
	}

	bool restsAgainst(const Box& thing, const Box& base, Edge side) noexcept
	{
		const auto within = [](float a, float b) { return std::abs(a - b) <= 1.0f; };
		const bool acrossOverlap = thing.right() > base.left() && thing.left() < base.right();
		const bool downOverlap = thing.bottom() > base.top() && thing.top() < base.bottom();
		switch (side)
		{
		case Edge::Top:    return acrossOverlap && within(thing.bottom(), base.top());
		case Edge::Bottom: return acrossOverlap && within(thing.top(), base.bottom());
		case Edge::Left:   return downOverlap && within(thing.right(), base.left());
		case Edge::Right:  return downOverlap && within(thing.left(), base.right());
		}

		return false;
	}

	void stickInside(Vector2f& position, Vector2f& velocity, const Vector2f& size, const Vector2f& window, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:
			position.x = std::max(position.x, 0.0f);
			velocity.x = std::max(velocity.x, 0.0f);
			break;
		case Edge::Right:
			position.x = std::min(position.x, window.x - size.x);
			velocity.x = std::min(velocity.x, 0.0f);
			break;
		case Edge::Top:
			position.y = std::max(position.y, 0.0f);
			velocity.y = std::max(velocity.y, 0.0f);
			break;
		case Edge::Bottom:
			position.y = std::min(position.y, window.y - size.y);
			velocity.y = std::min(velocity.y, 0.0f);
			break;
		}
	}

	void wrapAround(Vector2f& position, const Vector2f& size, const Vector2f& heading, const Vector2f& window, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Right:
			if (heading.x > 0.0f && position.x >= window.x) { position.x -= window.x + size.x; }
			break;
		case Edge::Left:
			if (heading.x < 0.0f && position.x + size.x <= 0.0f) { position.x += window.x + size.x; }
			break;
		case Edge::Bottom:
			if (heading.y > 0.0f && position.y >= window.y) { position.y -= window.y + size.y; }
			break;
		case Edge::Top:
			if (heading.y < 0.0f && position.y + size.y <= 0.0f) { position.y += window.y + size.y; }
			break;
		}
	}

	bool landOn(Vector2f& position, Vector2f& velocity, const Vector2f& size, const Box& other, Edge edge, bool climbing) noexcept
	{
		if (edge != Edge::Bottom || climbing || velocity.y < 0.0f)
		{
			return false;
		}

		position.y = other.top() - size.y;
		velocity.y = 0.0f;
		return true;
	}

	float leapSpeed(float pull, float height) noexcept
	{
		return std::sqrt(2.0f * pull * height);
	}

	Vector2f stepToward(Direction direction, float distance) noexcept
	{
		switch (direction)
		{
		case Direction::Up:    return { 0.0f, -distance };
		case Direction::Down:  return { 0.0f, distance };
		case Direction::Left:  return { -distance, 0.0f };
		case Direction::Right: return { distance, 0.0f };
		}

		return {};
	}

	Direction directionOf(const Vector2f& way) noexcept
	{
		if (std::abs(way.x) >= std::abs(way.y)) { return way.x < 0.0f ? Direction::Left : Direction::Right; }
		return way.y < 0.0f ? Direction::Up : Direction::Down;
	}

	std::optional<Vector2f> wayTo(const Vector2f& from, const Vector2f& to) noexcept
	{
		const Vector2f apart = to - from;
		const float distance = std::hypot(apart.x, apart.y);
		if (distance == 0.0f) { return std::nullopt; }
		return apart * (1.0f / distance);
	}

	Vector2f ahead(float heading) noexcept
	{
		const float radians = heading * pi / 180.0f;
		return { std::sin(radians), -std::cos(radians) };
	}

	float headingOf(const Vector2f& way) noexcept
	{
		const float heading = std::atan2(way.x, -way.y) * 180.0f / pi;
		return heading < 0.0f ? heading + 360.0f : heading;
	}

	Vector2f turnedToward(const Vector2f& velocity, const Vector2f& way, float degrees) noexcept
	{
		const float speed = std::hypot(velocity.x, velocity.y);
		if (speed == 0.0f || (way.x == 0.0f && way.y == 0.0f)) { return velocity; }

		const float now = std::atan2(velocity.y, velocity.x);
		float turn = std::atan2(way.y, way.x) - now;
		while (turn > pi) { turn -= 2.0f * pi; }
		while (turn < -pi) { turn += 2.0f * pi; }
		const float most = degrees * pi / 180.0f;
		const float after = now + std::clamp(turn, -most, most);
		return { speed * std::cos(after), speed * std::sin(after) };
	}

	std::optional<Vector2f> chase(const Vector2f& from, const Vector2f& to, float speed, float stopWithin, Vector2f& velocity) noexcept
	{
		const Vector2f apart = to - from;
		const float distance = std::hypot(apart.x, apart.y);
		if (distance <= stopWithin || distance == 0.0f)
		{
			velocity = {};
			return std::nullopt;
		}

		const Vector2f way = apart * (1.0f / distance);
		velocity = way * speed;
		return way;
	}

	bool atLadder(const Box& climber, const Box& ladder) noexcept
	{
		const float middle = climber.middle().x;
		const float feet = climber.bottom();
		return middle >= ladder.left() && middle <= ladder.right()
			&& feet >= ladder.top() - 0.5f && feet <= ladder.bottom() + 0.5f;
	}

	bool climbLadder(Vector2f& position, const Vector2f& size, float step, bool& climbing, bool grounded, const Box& ladder) noexcept
	{
		const float top = ladder.top();
		const float bottom = ladder.bottom();
		const float feet = position.y + size.y;

		if (!climbing)
		{
			const bool canGo = (step < 0.0f && feet > top + 0.5f) || (step > 0.0f && feet < bottom - 0.5f);
			if (!canGo || !grounded) { return false; }

			climbing = true;
			position.x = ladder.middle().x - size.x / 2.0f;
		}

		const float newFeet = std::clamp(feet + step, top, bottom);
		position.y = newFeet - size.y;

		if (step != 0.0f && (newFeet <= top || newFeet >= bottom))
		{
			climbing = false;
		}

		return true;
	}

	Shot shotAlong(const Box& shooter, const Vector2f& shotSize, const Vector2f& aim, float speed) noexcept
	{
		const float clear = std::max(std::abs(aim.x) * (shooter.size.x + shotSize.x), std::abs(aim.y) * (shooter.size.y + shotSize.y)) / 2.0f;
		return { shooter.middle() + aim * clear - shotSize * 0.5f, aim * speed };
	}

	Shot shotAhead(const Box& shooter, const Vector2f& shotSize, float heading, float speed) noexcept
	{
		const Vector2f way = ahead(heading);
		return { shooter.middle() + way * (std::max(shooter.size.x, shooter.size.y) / 2.0f) - shotSize * 0.5f, way * speed };
	}

	Shot shotFacing(const Box& shooter, const Vector2f& shotSize, Direction facing, float speed) noexcept
	{
		const float middleX = shooter.left() + shooter.size.x / 2 - shotSize.x / 2;
		const float middleY = shooter.top() + shooter.size.y / 2 - shotSize.y / 2;

		switch (facing)
		{
		case Direction::Up:    return { { middleX, shooter.top() - shotSize.y }, { 0.0f, -speed } };
		case Direction::Down:  return { { middleX, shooter.bottom() }, { 0.0f, speed } };
		case Direction::Left:  return { { shooter.left() - shotSize.x, middleY }, { -speed, 0.0f } };
		case Direction::Right: return { { shooter.right(), middleY }, { speed, 0.0f } };
		}

		return {};
	}

	Vector2f shotFromTop(const Box& shooter, const Vector2f& shotSize) noexcept
	{
		return { shooter.left() + shooter.size.x / 2 - shotSize.x / 2, shooter.top() };
	}

	std::optional<Vector2f> pathStep(const Vector2f& left, float speed) noexcept
	{
		const float distance = std::hypot(left.x, left.y);
		if (distance < 0.001f) { return std::nullopt; }
		return distance <= speed ? left : left * (speed / distance);
	}
}
