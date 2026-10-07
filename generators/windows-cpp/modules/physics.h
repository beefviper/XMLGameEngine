// physics.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The physics a generated game uses, in one place of its own: where a thing
// is, whether two things touch, and what a moving thing does at an edge of
// the window or against another thing. A "thing" is anything SFML can bound
// and move (a shape, a sprite, a text), and a velocity is how far it moves
// each frame. Header only, and nothing but SFML's graphics types, so it can
// be copied into any SFML 3 program.
//
// Collisions are the simple kind: move, then look at where it landed. A touch
// is told apart by the smaller overlap, which is enough for things that move
// less than their own size in a frame.

#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>

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
