// physics.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The engine's physics, for SFML's shapes. A "thing" is anything SFML can
// bound and move (a shape, a sprite, a text), and a velocity is how far it
// moves each frame. Each function here only turns a thing into the box the
// engine works with and back: the arithmetic is the engine's own
// (engine/geometry.h, copied unchanged from lib/), so a generated game
// bounces, deflects, wraps, lands and fires exactly as the engine does.
//
// A frame is played as the engine plays it: the edges looked at before the
// move, then the move from touch to touch (xge::playFrame, with sweep() here
// saying when two things touch), and a stuck thing kept in after it.

#pragma once

#include "geometry.h"

#include <SFML/Graphics.hpp>

#include <optional>
#include <type_traits>
#include <vector>

namespace physics
{
	using Edge = xge::Edge;
	using Facing = xge::Direction;
	using SweepHit = xge::SweepHit;
	using xge::opposite;
	using xge::playFrame;

	// --- Between SFML and the engine.

	inline xge::Vector2f toXge(sf::Vector2f vector)
	{
		return { vector.x, vector.y };
	}

	inline sf::Vector2f toSf(xge::Vector2f vector)
	{
		return { vector.x, vector.y };
	}

	template <typename Thing>
	xge::Box boxOf(const Thing& thing)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		return { toXge(bounds.position), toXge(bounds.size) };
	}

	inline xge::Box boxOf(const sf::FloatRect& area)
	{
		return { toXge(area.position), toXge(area.size) };
	}

	// Whether the engine tests the thing as a circle.
	template <typename Thing>
	constexpr bool isRound = std::is_same_v<Thing, sf::CircleShape>;

	// The thing moved to where the engine put its box.
	template <typename Thing>
	void place(Thing& thing, const xge::Box& before, xge::Vector2f position)
	{
		thing.move(toSf(position - before.position));
	}

	// --- Where a thing is.

	template <typename Thing>
	float left(const Thing& thing)
	{
		return boxOf(thing).left();
	}

	template <typename Thing>
	float right(const Thing& thing)
	{
		return boxOf(thing).right();
	}

	template <typename Thing>
	float top(const Thing& thing)
	{
		return boxOf(thing).top();
	}

	template <typename Thing>
	float bottom(const Thing& thing)
	{
		return boxOf(thing).bottom();
	}

	template <typename Thing>
	float width(const Thing& thing)
	{
		return boxOf(thing).size.x;
	}

	template <typename Thing>
	float height(const Thing& thing)
	{
		return boxOf(thing).size.y;
	}

	// Whether two things touch, by their shapes (xge::shapesOverlap): a circle
	// by its round shape, so a ball can pass the corner of a box its own box
	// would clip.
	template <typename First, typename Second>
	bool touching(const First& first, const Second& second)
	{
		return xge::shapesOverlap(boxOf(first), isRound<First>, boxOf(second), isRound<Second>).has_value();
	}

	// Whether the point `local` (from the thing's top left) is solid in it: in
	// a picture (a sprite, with its `pixels`), a pixel that is not clear;
	// otherwise its shape, as the engine's.
	template <typename Thing>
	bool solidAt(const Thing& thing, const sf::Image* pixels, xge::Vector2f local)
	{
		if constexpr (std::is_same_v<Thing, sf::Sprite>)
		{
			if (pixels != nullptr)
			{
				// where it is in the picture, a flipped one read the other way
				const sf::Vector2f point = thing.getGlobalBounds().position + toSf(local);
				const sf::Vector2f inside = thing.getInverseTransform().transformPoint(point);
				const sf::IntRect rect = thing.getTextureRect();
				const float x = static_cast<float>(rect.position.x) + (rect.size.x < 0 ? -inside.x : inside.x);
				const float y = static_cast<float>(rect.position.y) + (rect.size.y < 0 ? -inside.y : inside.y);
				const sf::Vector2u size = pixels->getSize();
				if (x < 0.0f || y < 0.0f || x >= static_cast<float>(size.x) || y >= static_cast<float>(size.y))
				{
					return false;
				}
				return pixels->getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}).a != 0;
			}
		}
		return xge::solidShapeAt(boxOf(thing).size, isRound<Thing>, local);
	}

	// --- The frame's move (xge::playFrame).

	// Whether a velocity moves a thing at all.
	inline bool moving(sf::Vector2f velocity)
	{
		return velocity.x != 0.0f || velocity.y != 0.0f;
	}

	// When, through what is left of the frame (`left`), two things moving by
	// these velocities a frame first touch, as the engine's
	// CollisionDetector::sweep (xge::sweepThings): none when neither is moving
	// (nor `hopped`: one of them has just hopped or jumped), as two at rest
	// already touching do nothing about it. `pixels` are the pictures of a
	// pair tested pixel by pixel (<type>pixel</type>), nullptr for a thing's
	// whole shape.
	template <typename First, typename Second>
	std::optional<SweepHit> sweepPixels(const First& first, const sf::Image* firstPixels, sf::Vector2f firstVelocity,
		const Second& second, const sf::Image* secondPixels, sf::Vector2f secondVelocity, float left, bool hopped = false)
	{
		if (!moving(firstVelocity) && !moving(secondVelocity) && !hopped)
		{
			return std::nullopt;
		}
		const auto solidFirst = [&](xge::Vector2f local) { return solidAt(first, firstPixels, local); };
		const auto solidSecond = [&](xge::Vector2f local) { return solidAt(second, secondPixels, local); };
		return xge::sweepThings(boxOf(first), isRound<First>, solidFirst, toXge(firstVelocity * left),
			boxOf(second), isRound<Second>, solidSecond, toXge(secondVelocity * left), true);
	}

	// The same, by their shapes alone.
	template <typename First, typename Second>
	std::optional<SweepHit> sweep(const First& first, sf::Vector2f firstVelocity, const Second& second, sf::Vector2f secondVelocity, float left, bool hopped = false)
	{
		if (!moving(firstVelocity) && !moving(secondVelocity) && !hopped)
		{
			return std::nullopt;
		}
		const auto whole = [](xge::Vector2f) { return true; };
		return xge::sweepThings(boxOf(first), isRound<First>, whole, toXge(firstVelocity * left),
			boxOf(second), isRound<Second>, whole, toXge(secondVelocity * left), false);
	}

	// --- The edges of an area (the window).

	// Whether the thing has gone past that edge of the area, which is when
	// the engine runs its rules about the edge.
	template <typename Thing>
	bool past(const Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		const xge::Box box = boxOf(thing);
		return xge::pastWindowEdge({ box.position - toXge(area.position), box.size }, toXge(area.size), edge);
	}

	// <bounce />: heading away from the edge it touched, its own (of the
	// area, or against another thing), at the same speed (xge::bounced).
	inline void bounce(sf::Vector2f& velocity, Edge edge)
	{
		velocity = toSf(xge::bounced(toXge(velocity), edge));
	}

	// Back inside, and no longer heading into the edge; the other way it
	// keeps going, so a thing pressed against the floor still slides
	// (xge::stickInside).
	template <typename Thing>
	void stick(Thing& thing, sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		const xge::Box box = boxOf(thing);
		xge::Vector2f position = box.position - toXge(area.position);
		xge::Vector2f moving = toXge(velocity);
		xge::stickInside(position, moving, box.size, toXge(area.size), edge);
		place(thing, box, position + toXge(area.position));
		velocity = toSf(moving);
	}

	// The same, for a thing that has no velocity of its own (a paddle moved by
	// keys): back inside is all there is to it.
	template <typename Thing>
	void stick(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		sf::Vector2f none;
		stick(thing, none, edge, area);
	}

	// Once the thing has gone right off the area through that edge, and is
	// still heading that way, in again from the opposite side, one area and
	// one of its own sizes along, so things spaced along a lane stay spaced
	// (xge::wrapAround). Until it has gone completely it slides out as normal.
	template <typename Thing>
	void wrap(Thing& thing, const sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		const xge::Box box = boxOf(thing);
		xge::Vector2f position = box.position - toXge(area.position);
		xge::wrapAround(position, box.size, toXge(velocity), toXge(area.size), edge);
		place(thing, box, position + toXge(area.position));
	}

	// The same, for a thing moved by keys alone (no velocity of its own): it
	// can only be right off an edge by having gone that way.
	template <typename Thing>
	void wrap(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		const xge::Vector2f outwards = xge::stepToward(edge == Edge::Top ? Facing::Up : edge == Edge::Bottom ? Facing::Down
			: edge == Edge::Left ? Facing::Left : Facing::Right, 1.0f);
		wrap(thing, toSf(outwards), edge, area);
	}

	// A step at once (a frog's hop), unless it would take the thing out of
	// the area: then it stays where it is.
	template <typename Thing>
	void hop(Thing& thing, const sf::Vector2f& step, const sf::FloatRect& area)
	{
		const xge::Box box = boxOf(thing);
		if (xge::insideWindow({ box.position + toXge(step) - toXge(area.position), box.size }, toXge(area.size)))
		{
			thing.move(step);
		}
	}

	// A jump that takes time (<jump>): `distance` over `frames` frames, a step
	// each (`jumping`), begun only if it lands inside the area and the thing
	// is not in the air already.
	template <typename Thing>
	void jump(const Thing& thing, const sf::Vector2f& distance, int frames, sf::Vector2f& step, int& framesLeft, const sf::FloatRect& area)
	{
		if (framesLeft > 0)
		{
			return;
		}
		const xge::Box box = boxOf(thing);
		if (xge::insideWindow({ box.position + toXge(distance) - toXge(area.position), box.size }, toXge(area.size)))
		{
			step = distance / static_cast<float>(frames);
			framesLeft = frames;
		}
	}

	// One frame of a jump under way: the next step, if it stays inside the
	// area, and a frame fewer left.
	template <typename Thing>
	void jumping(Thing& thing, const sf::Vector2f& step, int& framesLeft, const sf::FloatRect& area)
	{
		if (framesLeft == 0)
		{
			return;
		}
		hop(thing, step, area);
		--framesLeft;
	}

	// --- Standing and climbing.

	// Coming down onto the top of `other` (<land />, xge::landOn), `edge` the
	// thing's own edge that touched: its bottom, and not going up. It is put
	// on the top, its fall stopped, and the answer is yes. Any other touch
	// does nothing, so a platform can be walked past and jumped up through.
	template <typename Thing, typename Other>
	bool land(Thing& thing, sf::Vector2f& velocity, const Other& other, Edge edge)
	{
		const xge::Box box = boxOf(thing);
		xge::Vector2f position = box.position;
		xge::Vector2f moving = toXge(velocity);
		if (!xge::landOn(position, moving, box.size, boxOf(other), edge, false))
		{
			return false;
		}
		place(thing, box, position);
		velocity = toSf(moving);
		return true;
	}

	// Whether a climber is at a ladder: its middle over it, and its feet
	// between the ladder's top and bottom (half a pixel either way).
	template <typename Climber, typename Ladder>
	bool atLadder(const Climber& climber, const Ladder& ladder)
	{
		return xge::atLadder(boxOf(climber), boxOf(ladder));
	}

	// One frame at a ladder (<climb>, xge::climbLadder), `step` the way the
	// keys held climb (up below 0). Off it, it gets on only from the ground
	// and when it can go that way, lined up with the ladder's middle. On it,
	// it goes the step, its feet kept between the ladder's ends, and its
	// velocity is 0, so nothing walks it off; at an end it gets off, standing
	// there, `across` (the way the keys held walk it) its velocity. With no
	// ladder it is off. The answer is whether it got on this frame.
	template <typename Thing>
	bool climb(Thing& thing, sf::Vector2f& velocity, float step, bool& climbing, bool grounded, float across, const std::optional<sf::FloatRect>& ladder)
	{
		if (!ladder)
		{
			climbing = false;
			return false;
		}
		const bool wasClimbing = climbing;
		const xge::Box box = boxOf(thing);
		xge::Vector2f position = box.position;
		if (!xge::climbLadder(position, box.size, step, climbing, grounded, boxOf(*ladder)))
		{
			return false;
		}
		place(thing, box, position);
		velocity = {};
		if (!climbing)
		{
			velocity.x = across;
		}
		return !wasClimbing;
	}

	// --- Keys.

	// The keys held this frame (`steps`, the way they move it on each axis)
	// set that axis of the velocity when they change, as the engine sets it
	// when a key is pressed or let go: so whatever else changed the velocity
	// since (a <stick /> against a wall) holds until they change again.
	// `across` is false while the way across is kept (a leap).
	inline void steer(sf::Vector2f& velocity, sf::Vector2f steps, sf::Vector2f& stepped, bool across = true)
	{
		if (steps.x != stepped.x && across)
		{
			velocity.x = steps.x;
		}
		if (steps.y != stepped.y)
		{
			velocity.y = steps.y;
		}
		stepped = steps;
	}

	// --- Shots.

	// The way of the four nearest to `way`: the axis it goes further along.
	inline Facing facingOf(sf::Vector2f way)
	{
		return xge::directionOf(toXge(way));
	}

	// The shot placed and moving as the engine fires it.
	template <typename Shot>
	void fire(Shot& shot, sf::Vector2f& velocity, const xge::Shot& fired)
	{
		place(shot, boxOf(shot), fired.position);
		velocity = toSf(fired.velocity);
	}

	// A shot leaving a shooter that faces that way: from the middle of that
	// side, just clear of it, moving that way at `speed`.
	template <typename Shooter, typename Shot>
	void fireFrom(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, Facing facing, float speed)
	{
		fire(shot, velocity, xge::shotFacing(boxOf(shooter), boxOf(shot).size, facing, speed));
	}

	// The way a heading points (<heading>: degrees clockwise from straight
	// up), one pixel long.
	inline sf::Vector2f ahead(float heading)
	{
		return toSf(xge::ahead(heading));
	}

	// A shot leaving a shooter with a heading: from its middle, out by half
	// its longer side along the heading, at `speed`.
	template <typename Shooter, typename Shot>
	void fireAhead(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, float heading, float speed)
	{
		fire(shot, velocity, xge::shotAhead(boxOf(shooter), boxOf(shot).size, heading, speed));
	}

	// The way from the middle of a thing to a place, one pixel long (<aim>);
	// none with no place, or at it.
	template <typename Thing>
	std::optional<sf::Vector2f> aimAt(const Thing& thing, std::optional<sf::Vector2f> there)
	{
		if (!there)
		{
			return std::nullopt;
		}
		const std::optional<xge::Vector2f> way = xge::wayTo(boxOf(thing).middle(), toXge(*there));
		if (!way)
		{
			return std::nullopt;
		}
		return toSf(*way);
	}

	// A shot leaving a shooter that has aimed: from its middle, just clear of
	// it along the aim, at `speed`.
	template <typename Shooter, typename Shot>
	void fireAlong(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, sf::Vector2f aim, float speed)
	{
		fire(shot, velocity, xge::shotAlong(boxOf(shooter), boxOf(shot).size, toXge(aim), speed));
	}

	// Straight at a place at `speed`, or still within `near` of it
	// (<chase>); with no place it goes on as it was. The way it set off, one
	// pixel long, if it did.
	template <typename Thing>
	std::optional<sf::Vector2f> chase(const Thing& thing, sf::Vector2f& velocity, std::optional<sf::Vector2f> there, float speed, float near)
	{
		if (!there)
		{
			return std::nullopt;
		}
		xge::Vector2f moving = toXge(velocity);
		const std::optional<xge::Vector2f> way = xge::chase(boxOf(thing).middle(), toXge(*there), speed, near, moving);
		velocity = toSf(moving);
		if (!way)
		{
			return std::nullopt;
		}
		return toSf(*way);
	}

	// --- Against another thing.

	// Off `other` at the same speed, `edge` its own edge that touched: straight
	// out from the middle of that side of `other`, and up to `maxDegrees`
	// towards either end (<deflect>, xge::deflected). A Pong paddle.
	template <typename Mover, typename Other>
	void deflect(const Mover& mover, sf::Vector2f& velocity, const Other& other, Edge edge, float maxDegrees)
	{
		velocity = toSf(xge::deflected(toXge(velocity), boxOf(mover), boxOf(other), edge, maxDegrees));
	}

	// --- Paths (<paths>, <follow>).

	// One leg of a path: a step by so far from where it began, or home to
	// where the thing started the game.
	struct Leg
	{
		sf::Vector2f by;
		bool home = false;
	};

	// A path: flown at `speed` pixels a frame, from `start` if it has one.
	struct Route
	{
		float speed;
		std::optional<sf::Vector2f> start;
		std::vector<Leg> legs;
	};

	// A thing on its way along one of the game's paths (Path, its enum):
	// which (none: on no path), the leg it is flying, what is left of that
	// leg's step, whether the leg has begun (what it does is done once), and
	// the frames to wait before the first leg (a <stagger>).
	template <typename Path>
	struct Flight
	{
		std::optional<Path> path;
		std::size_t leg = 0;
		sf::Vector2f left;
		bool begun = false;
		int wait = 0;
	};

	// Sets off along a path after `wait` frames, unless it is on one already:
	// put at the path's start, if it has one, and still until the first leg.
	// `routes` is every path, in the order of Path.
	template <typename Thing, typename Path>
	void follow(Thing& thing, sf::Vector2f& velocity, Flight<Path>& flight, Path path, int wait, const std::vector<Route>& routes)
	{
		if (flight.path)
		{
			return;
		}
		flight = Flight<Path>{path, 0, {}, false, std::max(0, wait)};
		velocity = {};
		if (const std::optional<sf::Vector2f> start = routes[static_cast<std::size_t>(path)].start)
		{
			thing.setPosition(*start);
		}
	}

	// A frame along its path, as the engine's Game::applyPaths: the velocity
	// that takes it on, a leg at a time (xge::pathStep), `begin(path, leg)`
	// doing what a leg does as it begins. At the end it comes to rest where it is.
	template <typename Thing, typename Path, typename Begin>
	void fly(const Thing& thing, sf::Vector2f& velocity, Flight<Path>& flight, sf::Vector2f home, const std::vector<Route>& routes, Begin begin)
	{
		if (!flight.path)
		{
			return;
		}
		if (flight.wait > 0)
		{
			--flight.wait;
			velocity = {};
			return;
		}
		const Route& route = routes[static_cast<std::size_t>(*flight.path)];
		while (flight.path && flight.leg < route.legs.size())
		{
			const Leg& leg = route.legs[flight.leg];
			if (!flight.begun)
			{
				flight.begun = true;
				flight.left = leg.by;
				begin(*flight.path, flight.leg);
				if (!flight.path)
				{
					return; // what it did ended the path
				}
			}
			const sf::Vector2f rest = leg.home ? home - thing.getPosition() : flight.left;
			const std::optional<xge::Vector2f> step = xge::pathStep(toXge(rest), route.speed);
			if (!step)
			{
				++flight.leg;
				flight.begun = false;
				continue;
			}
			velocity = toSf(*step);
			if (!leg.home)
			{
				flight.left -= velocity;
			}
			return;
		}
		velocity = {};
		flight.path.reset();
	}
}
