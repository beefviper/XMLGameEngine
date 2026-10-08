// physics.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The physics a generated game uses, in one place of its own: where a thing
// is, whether two things touch, what a moving thing does at an edge of the
// window (bounce, stick, wrap round) or against another thing, a hop, and a
// shot leaving the side a thing faces or the way it heads. A "thing" is
// anything SFML can bound and move (a shape, a sprite, a text), and a
// velocity is how far it moves each frame. Header only, and nothing but
// SFML's graphics types, so it can be copied into any SFML 3 program.
//
// Collisions are the simple kind: move, then look at where it landed. A touch
// is told apart by the smaller overlap, which is enough for things that move
// less than their own size in a frame. Touches pixel by pixel are looked for
// all along the step, as thin lines are what they are for.

#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <type_traits>

namespace physics
{
	// --- Where a thing is.

	template <typename Thing>
	float left(const Thing& thing)
	{
		return thing.getGlobalBounds().position.x;
	}

	template <typename Thing>
	float right(const Thing& thing)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		return bounds.position.x + bounds.size.x;
	}

	template <typename Thing>
	float top(const Thing& thing)
	{
		return thing.getGlobalBounds().position.y;
	}

	template <typename Thing>
	float bottom(const Thing& thing)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		return bounds.position.y + bounds.size.y;
	}

	template <typename Thing>
	float width(const Thing& thing)
	{
		return thing.getGlobalBounds().size.x;
	}

	template <typename Thing>
	float height(const Thing& thing)
	{
		return thing.getGlobalBounds().size.y;
	}

	template <typename First, typename Second>
	bool touching(const First& first, const Second& second)
	{
		return first.getGlobalBounds().findIntersection(second.getGlobalBounds()).has_value();
	}

	// Whether the middle of the screen's pixel at `point` is solid in the
	// thing: in a picture (a sprite, with its `pixels`), a pixel that is not
	// clear; in a circle, inside it; in anything else, inside its box.
	template <typename Thing>
	bool solidAt(const Thing& thing, const sf::Image* pixels, sf::Vector2f point)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		if (!bounds.contains(point))
		{
			return false;
		}
		if constexpr (std::is_same_v<Thing, sf::CircleShape>)
		{
			const float radius = bounds.size.x / 2.0f;
			return (point - bounds.getCenter()).lengthSquared() <= radius * radius;
		}
		else if constexpr (std::is_same_v<Thing, sf::Sprite>)
		{
			if (pixels == nullptr)
			{
				return true;
			}
			// where it is in the picture, a flipped one read the other way
			const sf::Vector2f local = thing.getInverseTransform().transformPoint(point);
			const sf::IntRect rect = thing.getTextureRect();
			const float x = static_cast<float>(rect.position.x) + (rect.size.x < 0 ? -local.x : local.x);
			const float y = static_cast<float>(rect.position.y) + (rect.size.y < 0 ? -local.y : local.y);
			const sf::Vector2u size = pixels->getSize();
			if (x < 0.0f || y < 0.0f || x >= static_cast<float>(size.x) || y >= static_cast<float>(size.y))
			{
				return false;
			}
			return pixels->getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}).a != 0;
		}
		else
		{
			return true;
		}
	}

	// Whether two things touch pixel by pixel (<type>pixel</type>): some
	// pixel of the screen both cover is solid in both. `pixels` is the picture
	// a sprite shows, or nullptr for its whole box. `step` is how far the first
	// moved this frame against the second: it is looked at all along the way,
	// half a pixel at a time, as the engine sweeps it, so that a fast shot does
	// not jump over a line two pixels thick.
	template <typename First, typename Second>
	bool touchingPixels(const First& first, const sf::Image* firstPixels, const Second& second, const sf::Image* secondPixels, sf::Vector2f step = {})
	{
		const sf::FloatRect now = first.getGlobalBounds();
		const sf::FloatRect target = second.getGlobalBounds();
		const int steps = std::max(1, static_cast<int>(std::ceil(step.length() * 2.0f)));
		for (int at = 0; at <= steps; ++at)
		{
			// where it was, that far through the step (the last is where it is)
			const sf::Vector2f back = step * (static_cast<float>(at) / static_cast<float>(steps) - 1.0f);
			const std::optional<sf::FloatRect> both = sf::FloatRect(now.position + back, now.size).findIntersection(target);
			if (!both)
			{
				continue;
			}
			const int left = static_cast<int>(std::floor(both->position.x));
			const int top = static_cast<int>(std::floor(both->position.y));
			const int right = static_cast<int>(std::ceil(both->position.x + both->size.x));
			const int bottom = static_cast<int>(std::ceil(both->position.y + both->size.y));
			for (int y = top; y < bottom; ++y)
			{
				for (int x = left; x < right; ++x)
				{
					const sf::Vector2f point(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
					if (solidAt(first, firstPixels, point - back) && solidAt(second, secondPixels, point))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	// --- The edges of an area (the window).

	enum class Edge { Top, Bottom, Left, Right };

	// Whether the thing has gone past that edge of the area.
	template <typename Thing>
	bool past(const Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		switch (edge)
		{
		case Edge::Top:    return top(thing) < area.position.y;
		case Edge::Bottom: return bottom(thing) > area.position.y + area.size.y;
		case Edge::Left:   return left(thing) < area.position.x;
		case Edge::Right:  return right(thing) > area.position.x + area.size.x;
		}
		return false;
	}

	// Back inside the area at that edge.
	template <typename Thing>
	void putBack(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		switch (edge)
		{
		case Edge::Top:    thing.move({ 0.0f, area.position.y - top(thing) }); break;
		case Edge::Bottom: thing.move({ 0.0f, area.position.y + area.size.y - bottom(thing) }); break;
		case Edge::Left:   thing.move({ area.position.x - left(thing), 0.0f }); break;
		case Edge::Right:  thing.move({ area.position.x + area.size.x - right(thing), 0.0f }); break;
		}
	}

	// Back inside, and heading away from the edge.
	template <typename Thing>
	void bounce(Thing& thing, sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		putBack(thing, edge, area);
		switch (edge)
		{
		case Edge::Top:    velocity.y = std::abs(velocity.y); break;
		case Edge::Bottom: velocity.y = -std::abs(velocity.y); break;
		case Edge::Left:   velocity.x = std::abs(velocity.x); break;
		case Edge::Right:  velocity.x = -std::abs(velocity.x); break;
		}
	}

	// Back inside, and no longer heading into the edge; the other way it
	// keeps going, so a thing pressed against the floor still slides.
	template <typename Thing>
	void stick(Thing& thing, sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		putBack(thing, edge, area);
		switch (edge)
		{
		case Edge::Top:    velocity.y = std::max(velocity.y, 0.0f); break;
		case Edge::Bottom: velocity.y = std::min(velocity.y, 0.0f); break;
		case Edge::Left:   velocity.x = std::max(velocity.x, 0.0f); break;
		case Edge::Right:  velocity.x = std::min(velocity.x, 0.0f); break;
		}
	}

	// The same, for a thing that has no velocity of its own (a paddle moved by
	// keys): back inside is all there is to it.
	template <typename Thing>
	void stick(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		putBack(thing, edge, area);
	}

	// Once the thing has gone right off the area through that edge, and is
	// still heading that way, in again from the opposite side, one area and
	// one of its own sizes along, so things spaced along a lane stay spaced.
	// Until it has gone completely it slides out as normal.
	template <typename Thing>
	void wrap(Thing& thing, const sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		switch (edge)
		{
		case Edge::Right:
			if (velocity.x > 0.0f && left(thing) >= area.position.x + area.size.x) { thing.move({ -(area.size.x + width(thing)), 0.0f }); }
			break;
		case Edge::Left:
			if (velocity.x < 0.0f && right(thing) <= area.position.x) { thing.move({ area.size.x + width(thing), 0.0f }); }
			break;
		case Edge::Bottom:
			if (velocity.y > 0.0f && top(thing) >= area.position.y + area.size.y) { thing.move({ 0.0f, -(area.size.y + height(thing)) }); }
			break;
		case Edge::Top:
			if (velocity.y < 0.0f && bottom(thing) <= area.position.y) { thing.move({ 0.0f, area.size.y + height(thing) }); }
			break;
		}
	}

	// The same, for a thing moved by keys alone (no velocity of its own): once
	// it has gone right off through that edge, in again from the opposite side.
	template <typename Thing>
	void wrap(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		switch (edge)
		{
		case Edge::Right:
			if (left(thing) >= area.position.x + area.size.x) { thing.move({ -(area.size.x + width(thing)), 0.0f }); }
			break;
		case Edge::Left:
			if (right(thing) <= area.position.x) { thing.move({ area.size.x + width(thing), 0.0f }); }
			break;
		case Edge::Bottom:
			if (top(thing) >= area.position.y + area.size.y) { thing.move({ 0.0f, -(area.size.y + height(thing)) }); }
			break;
		case Edge::Top:
			if (bottom(thing) <= area.position.y) { thing.move({ 0.0f, area.size.y + height(thing) }); }
			break;
		}
	}

	// A step at once (a frog's hop), unless it would take the thing out of
	// the area: then it stays where it is.
	template <typename Thing>
	void hop(Thing& thing, const sf::Vector2f& step, const sf::FloatRect& area)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		const sf::Vector2f from = bounds.position + step;
		const sf::Vector2f to = from + bounds.size;
		if (from.x >= area.position.x && from.y >= area.position.y
			&& to.x <= area.position.x + area.size.x && to.y <= area.position.y + area.size.y)
		{
			thing.move(step);
		}
	}

	// --- Shots.

	// The way a thing faces (<facing>).
	enum class Facing { Up, Down, Left, Right };

	// A shot leaving a shooter that faces that way: from the middle of that
	// side, just clear of it, moving that way at `speed`.
	template <typename Shooter, typename Shot>
	void fireFrom(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, Facing facing, float speed)
	{
		const sf::FloatRect from = shooter.getGlobalBounds();
		const sf::Vector2f size = shot.getGlobalBounds().size;
		const sf::Vector2f middle = from.getCenter() - size / 2.0f;
		switch (facing)
		{
		case Facing::Up:
			shot.setPosition({ middle.x, from.position.y - size.y });
			velocity = { 0.0f, -speed };
			break;
		case Facing::Down:
			shot.setPosition({ middle.x, from.position.y + from.size.y });
			velocity = { 0.0f, speed };
			break;
		case Facing::Left:
			shot.setPosition({ from.position.x - size.x, middle.y });
			velocity = { -speed, 0.0f };
			break;
		case Facing::Right:
			shot.setPosition({ from.position.x + from.size.x, middle.y });
			velocity = { speed, 0.0f };
			break;
		}
	}

	// The way a heading points (<heading>: degrees clockwise from straight
	// up), one pixel long.
	inline sf::Vector2f ahead(float heading)
	{
		return sf::Vector2f(1.0f, sf::degrees(heading - 90.0f));
	}

	// A shot leaving a shooter with a heading: from its middle, out by half
	// its longer side along the heading, at `speed`.
	template <typename Shooter, typename Shot>
	void fireAhead(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, float heading, float speed)
	{
		const sf::FloatRect from = shooter.getGlobalBounds();
		const sf::Vector2f way = ahead(heading);
		shot.setPosition(from.getCenter() + way * (std::max(from.size.x, from.size.y) / 2.0f) - shot.getGlobalBounds().size / 2.0f);
		velocity = way * speed;
	}

	// --- Against another thing.

	// Out of the side of `other` it went into, heading away from it.
	template <typename Mover, typename Other>
	void bounceOff(Mover& mover, sf::Vector2f& velocity, const Other& other)
	{
		const sf::FloatRect bounds = mover.getGlobalBounds();
		const sf::FloatRect otherBounds = other.getGlobalBounds();
		const sf::FloatRect overlap = bounds.findIntersection(otherBounds).value_or(sf::FloatRect{});
		const sf::Vector2f offset = bounds.getCenter() - otherBounds.getCenter();

		if (overlap.size.x < overlap.size.y)
		{
			const float side = offset.x < 0.0f ? -1.0f : 1.0f;
			mover.move({ side * overlap.size.x, 0.0f });
			velocity.x = side * std::abs(velocity.x);
		}
		else
		{
			const float side = offset.y < 0.0f ? -1.0f : 1.0f;
			mover.move({ 0.0f, side * overlap.size.y });
			velocity.y = side * std::abs(velocity.y);
		}
	}

	// Out of the side of `other` it went into, at the same speed: straight out
	// from the middle of that side, and up to `maxDegrees` towards either end.
	// A Pong paddle.
	template <typename Mover, typename Other>
	void deflect(Mover& mover, sf::Vector2f& velocity, const Other& other, float maxDegrees)
	{
		const sf::FloatRect bounds = mover.getGlobalBounds();
		const sf::FloatRect otherBounds = other.getGlobalBounds();
		const sf::FloatRect overlap = bounds.findIntersection(otherBounds).value_or(sf::FloatRect{});
		const sf::Vector2f offset = bounds.getCenter() - otherBounds.getCenter();
		const float speed = velocity.length();

		if (overlap.size.x < overlap.size.y)
		{
			// a left or right side: angled by how far up or down it hit
			const float angle = std::clamp(offset.y / (otherBounds.size.y / 2.0f), -1.0f, 1.0f) * maxDegrees;
			const float side = offset.x < 0.0f ? -1.0f : 1.0f;
			mover.move({ side * overlap.size.x, 0.0f });
			velocity = sf::Vector2f(speed, sf::degrees(side < 0.0f ? 180.0f - angle : angle));
		}
		else
		{
			// the top or bottom: angled by how far left or right it hit
			const float angle = std::clamp(offset.x / (otherBounds.size.x / 2.0f), -1.0f, 1.0f) * maxDegrees;
			const float side = offset.y < 0.0f ? -1.0f : 1.0f;
			mover.move({ 0.0f, side * overlap.size.y });
			velocity = sf::Vector2f(speed, sf::degrees(side < 0.0f ? angle - 90.0f : 90.0f - angle));
		}
	}
}
