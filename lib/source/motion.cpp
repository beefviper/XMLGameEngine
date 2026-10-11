// motion.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// The arithmetic of the movement verbs (motion.h). Copied into a generated game
// as it is.

#include "motion.h"

#include <algorithm>
#include <cmath>

namespace xge::motion
{
	namespace
	{
		constexpr float pi = 3.14159265358979323846f;
	}

	Direction directionOf(const Vector2f& way) noexcept
	{
		if (std::abs(way.x) >= std::abs(way.y)) { return way.x < 0.0f ? Direction::Left : Direction::Right; }
		return way.y < 0.0f ? Direction::Up : Direction::Down;
	}

	Vector2f along(Direction direction, float distance) noexcept
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

	Vector2f ahead(float degrees) noexcept
	{
		const float radians = degrees * pi / 180.0f;
		return { std::sin(radians), -std::cos(radians) };
	}

	float headingOf(const Vector2f& way) noexcept
	{
		float heading = std::atan2(way.x, -way.y) * 180.0f / pi;
		if (heading < 0.0f) { heading += 360.0f; }
		return heading;
	}

	bool fits(const Vector2f& position, const Vector2f& size, float width, float height) noexcept
	{
		return position.x >= 0 && position.y >= 0 && position.x + size.x <= width && position.y + size.y <= height;
	}

	void stick(Vector2f& position, Vector2f& velocity, const Vector2f& size, float width, float height, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:
			position.x = std::max(position.x, 0.0f);
			velocity.x = std::max(velocity.x, 0.0f);
			break;
		case Edge::Right:
			position.x = std::min(position.x, width - size.x);
			velocity.x = std::min(velocity.x, 0.0f);
			break;
		case Edge::Top:
			position.y = std::max(position.y, 0.0f);
			velocity.y = std::max(velocity.y, 0.0f);
			break;
		case Edge::Bottom:
			position.y = std::min(position.y, height - size.y);
			velocity.y = std::min(velocity.y, 0.0f);
			break;
		}
	}

	bool wrap(Vector2f& position, const Vector2f& size, const Vector2f& heading, float width, float height, Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Right:
			if (heading.x > 0.0f && position.x >= width)
			{
				position.x -= width + size.x;
				return true;
			}
			break;
		case Edge::Left:
			if (heading.x < 0.0f && position.x + size.x <= 0.0f)
			{
				position.x += width + size.x;
				return true;
			}
			break;
		case Edge::Bottom:
			if (heading.y > 0.0f && position.y >= height)
			{
				position.y -= height + size.y;
				return true;
			}
			break;
		case Edge::Top:
			if (heading.y < 0.0f && position.y + size.y <= 0.0f)
			{
				position.y += height + size.y;
				return true;
			}
			break;
		}
		return false;
	}

	Vector2f outward(Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Right:  return { 1.0f, 0.0f };
		case Edge::Left:   return { -1.0f, 0.0f };
		case Edge::Bottom: return { 0.0f, 1.0f };
		case Edge::Top:    return { 0.0f, -1.0f };
		}
		return {};
	}

	std::optional<Vector2f> jumpStep(const Vector2f& position, const Vector2f& size, Direction direction, float distance, int frames, float width, float height) noexcept
	{
		const Vector2f landing = position + along(direction, distance);
		if (!fits(landing, size, width, height))
		{
			return std::nullopt;
		}

		return along(direction, distance / static_cast<float>(frames));
	}

	bool land(Vector2f& position, Vector2f& velocity, const Vector2f& size, float topOfOther, Edge edgeOfMover, bool climbing) noexcept
	{
		if (edgeOfMover != Edge::Bottom || climbing || velocity.y < 0.0f)
		{
			return false;
		}

		position.y = topOfOther - size.y;
		velocity.y = 0.0f;
		return true;
	}

	float leapSpeed(float pull, float height) noexcept
	{
		return -std::sqrt(2.0f * pull * height);
	}

	bool atLadder(const Vector2f& climberPosition, const Vector2f& climberSize, const Vector2f& ladderPosition, const Vector2f& ladderSize) noexcept
	{
		const float middle = climberPosition.x + climberSize.x / 2.0f;
		const float feet = climberPosition.y + climberSize.y;
		return middle >= ladderPosition.x && middle <= ladderPosition.x + ladderSize.x
			&& feet >= ladderPosition.y - 0.5f && feet <= ladderPosition.y + ladderSize.y + 0.5f;
	}

	Climbed climb(Vector2f& position, Vector2f& velocity, const Vector2f& size, bool& climbing, bool grounded, float step, const Vector2f& ladderPosition, const Vector2f& ladderSize) noexcept
	{
		const float top = ladderPosition.y;
		const float bottom = ladderPosition.y + ladderSize.y;
		const float feet = position.y + size.y;

		if (!climbing)
		{
			const bool canGo = (step < 0.0f && feet > top + 0.5f) || (step > 0.0f && feet < bottom - 0.5f);
			if (!canGo || !grounded)
			{
				return {};
			}

			climbing = true;
			position.x = ladderPosition.x + ladderSize.x / 2.0f - size.x / 2.0f;
		}

		const float newFeet = std::clamp(feet + step, top, bottom);
		position.y = newFeet - size.y;
		velocity = {};

		Climbed result;
		result.moved = true;
		if (step != 0.0f && (newFeet <= top || newFeet >= bottom))
		{
			climbing = false;
			result.gotOff = true;
		}
		return result;
	}

	Launch shotFacing(const Vector2f& shooter, const Vector2f& shooterSize, const Vector2f& shotSize, Direction facing, float speed) noexcept
	{
		const float middleX = shooter.x + shooterSize.x / 2 - shotSize.x / 2;
		const float middleY = shooter.y + shooterSize.y / 2 - shotSize.y / 2;

		switch (facing)
		{
		case Direction::Up:    return { { middleX, shooter.y - shotSize.y }, { 0.0f, -speed } };
		case Direction::Down:  return { { middleX, shooter.y + shooterSize.y }, { 0.0f, speed } };
		case Direction::Left:  return { { shooter.x - shotSize.x, middleY }, { -speed, 0.0f } };
		case Direction::Right: return { { shooter.x + shooterSize.x, middleY }, { speed, 0.0f } };
		}
		return {};
	}

	Launch shotAlong(const Vector2f& shooter, const Vector2f& shooterSize, const Vector2f& shotSize, const Vector2f& aim, float speed) noexcept
	{
		const Vector2f centre = shooter + shooterSize * 0.5f;
		const float clear = std::max(std::abs(aim.x) * (shooterSize.x + shotSize.x), std::abs(aim.y) * (shooterSize.y + shotSize.y)) / 2.0f;
		return { centre + aim * clear - shotSize * 0.5f, aim * speed };
	}

	Launch shotAhead(const Vector2f& shooter, const Vector2f& shooterSize, const Vector2f& shotSize, float heading, float speed) noexcept
	{
		const Vector2f way = ahead(heading);
		const Vector2f centre = shooter + shooterSize * 0.5f;
		return { centre + way * (std::max(shooterSize.x, shooterSize.y) / 2.0f) - shotSize * 0.5f, way * speed };
	}

	Launch shotUp(const Vector2f& shooter, const Vector2f& shooterSize, const Vector2f& shotSize, const Vector2f& shotVelocity) noexcept
	{
		return { { shooter.x + shooterSize.x / 2 - shotSize.x / 2, shooter.y }, shotVelocity };
	}

	std::optional<Vector2f> aimed(const Vector2f& from, const Vector2f& to) noexcept
	{
		const Vector2f apart = to - from;
		const float distance = std::hypot(apart.x, apart.y);
		if (distance == 0.0f)
		{
			return std::nullopt;
		}

		return apart * (1.0f / distance);
	}

	Chase chased(const Vector2f& from, const Vector2f& to, float speed, float stopWithin) noexcept
	{
		const Vector2f apart = to - from;
		const float distance = std::hypot(apart.x, apart.y);
		if (distance <= stopWithin || distance == 0.0f)
		{
			return {};
		}

		const Vector2f way = apart * (1.0f / distance);
		return { way * speed, way };
	}

	Vector2f accelerated(const Vector2f& velocity, const Vector2f& change, float drag) noexcept
	{
		Vector2f result = velocity + change;
		if (drag != 0.0f)
		{
			result *= 1.0f - drag;
		}
		return result;
	}

	Flight fly(const Vector2f& left, float speed) noexcept
	{
		const float distance = std::hypot(left.x, left.y);
		if (distance < 0.001f)
		{
			return { true, {} };
		}

		return { false, distance <= speed ? left : left * (speed / distance) };
	}
}
