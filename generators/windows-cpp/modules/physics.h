// physics.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The physics a generated game uses, in one place of its own: where a thing
// is, whether two things touch, what a moving thing does at an edge of the
// window (bounce, stick, wrap round) or against another thing, a hop, a jump,
// standing on a platform and climbing a ladder, heading for another thing,
// a shot leaving the side a thing faces, the way it heads or the way it has
// aimed, and flying a path. A "thing" is anything SFML can bound and move (a shape, a
// sprite, a text), and a velocity is how far it moves each frame. Header
// only, and nothing but SFML's graphics types, so it can be copied into any
// SFML 3 program.
//
// Collisions are the simple kind: move, then look at where it landed. A touch
// is told apart by the smaller overlap, which is enough for things that move
// less than their own size in a frame. Touches pixel by pixel are looked for
// all along the step, as thin lines are what they are for.

#pragma once

#include "collide.h"
#include "motion.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <type_traits>
#include <vector>

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

	// A thing as the engine's geometry sees it (collide.h, copied in beside this
	// file as it is): where it is, how big, and whether it is a circle.
	template <typename Thing>
	xge::collide::Body bodyOf(const Thing& thing)
	{
		const sf::FloatRect bounds = thing.getGlobalBounds();
		xge::collide::Body seen;
		seen.position = xge::Vector2f(bounds.position.x, bounds.position.y);
		seen.size = xge::Vector2f(bounds.size.x, bounds.size.y);
		seen.circle = std::is_same_v<Thing, sf::CircleShape>;
		return seen;
	}

	inline xge::Vector2f fromSfml(sf::Vector2f vector)
	{
		return xge::Vector2f(vector.x, vector.y);
	}

	inline sf::Vector2f toSfml(xge::Vector2f vector)
	{
		return sf::Vector2f(vector.x, vector.y);
	}

	// Whether two things overlap, as the engine decides it: two boxes by more
	// than an edge, and a circle (an sf::CircleShape) by its round shape against
	// the other's box, so a ball can pass the corner of a box its own box would
	// clip.
	template <typename First, typename Second>
	bool touching(const First& first, const Second& second)
	{
		return xge::collide::overlap(bodyOf(first), bodyOf(second)).has_value();
	}

	// A picture that is asked whether one of its pixels is solid, for the
	// engine's geometry (collide.h): an sf::Image, read the way the sprite that
	// shows it is cut from its texture (a flipped sprite is cut with a negative
	// size, and reads its picture the other way round).
	struct PixelView
	{
		const sf::Image* image = nullptr;
		sf::IntRect cut;
	};

	inline bool pixelSolid(const void* picture, int x, int y)
	{
		const PixelView& view = *static_cast<const PixelView*>(picture);
		const int column = view.cut.size.x < 0 ? view.cut.position.x - 1 - x : view.cut.position.x + x;
		const int row = view.cut.size.y < 0 ? view.cut.position.y - 1 - y : view.cut.position.y + y;
		const sf::Vector2u size = view.image->getSize();
		if (column < 0 || row < 0 || column >= static_cast<int>(size.x) || row >= static_cast<int>(size.y))
		{
			return false;
		}
		return view.image->getPixel({static_cast<unsigned int>(column), static_cast<unsigned int>(row)}).a != 0;
	}

	// What a thing shows of its picture: all of it, unless it is a sprite cut
	// from a texture.
	template <typename Thing>
	PixelView viewOf(const Thing& thing, const sf::Image* pixels)
	{
		PixelView view;
		view.image = pixels;
		if (pixels != nullptr)
		{
			view.cut = sf::IntRect({0, 0}, sf::Vector2i(pixels->getSize()));
			if constexpr (std::is_same_v<Thing, sf::Sprite>)
			{
				view.cut = thing.getTextureRect();
			}
		}
		return view;
	}

	// Whether two things touch pixel by pixel (<type>pixel</type>): some
	// pixel of the screen both cover is solid in both, as the engine tests it
	// (collide::sweep). `pixels` is the picture a sprite shows, or nullptr for
	// its whole box (or circle). `step` is how far the first moved this frame
	// against the second: it is looked at all along the way, half a pixel at a
	// time, so that a fast shot does not jump over a line two pixels thick.
	template <typename First, typename Second>
	bool touchingPixels(const First& first, const sf::Image* firstPixels, const Second& second, const sf::Image* secondPixels, sf::Vector2f step = {})
	{
		const PixelView firstView = viewOf(first, firstPixels);
		const PixelView secondView = viewOf(second, secondPixels);

		xge::collide::Body mover = bodyOf(first);
		mover.pixels = firstPixels != nullptr;
		mover.picture = &firstView;
		mover.solidAt = pixelSolid;
		mover.position = mover.position - fromSfml(step);

		xge::collide::Body target = bodyOf(second);
		target.pixels = secondPixels != nullptr;
		target.picture = &secondView;
		target.solidAt = pixelSolid;

		return xge::collide::sweep(mover, fromSfml(step), target, xge::Vector2f()).has_value();
	}

	// --- The edges of an area (the window).

	using Edge = xge::Edge;

	// A thing's size and where it is, as the engine's movement code
	// (motion.h) takes them.
	template <typename Thing>
	xge::Vector2f positionOf(const Thing& thing)
	{
		return fromSfml(thing.getGlobalBounds().position);
	}

	template <typename Thing>
	xge::Vector2f sizeOf(const Thing& thing)
	{
		return fromSfml(thing.getGlobalBounds().size);
	}

	// Moves a thing to the place the engine's code worked out for it.
	template <typename Thing>
	void put(Thing& thing, xge::Vector2f where)
	{
		thing.move(toSfml(where) - thing.getGlobalBounds().position);
	}

	// Whether the thing has gone past that edge of the area, as the engine
	// decides it (collide::touchesScreenEdge), which looks at the window.
	template <typename Thing>
	bool past(const Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		return xge::collide::touchesScreenEdge(bodyOf(thing), area.size.x, area.size.y, edge);
	}

	// Heading away from the edge, from where it is: it stays as far over as the
	// step took it, and the next step brings it back, as in the engine.
	template <typename Thing>
	void bounce(Thing&, sf::Vector2f& velocity, Edge edge, const sf::FloatRect&)
	{
		velocity = toSfml(xge::collide::bounced(fromSfml(velocity), edge));
	}

	// Back inside, and no longer heading into the edge; the other way it
	// keeps going, so a thing pressed against the floor still slides
	// (motion::stick).
	template <typename Thing>
	void stick(Thing& thing, sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		xge::Vector2f where = positionOf(thing);
		xge::Vector2f going = fromSfml(velocity);
		xge::motion::stick(where, going, sizeOf(thing), area.size.x, area.size.y, edge);
		put(thing, where);
		velocity = toSfml(going);
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
	// one of its own sizes along, so things spaced along a lane stay spaced.
	// Until it has gone completely it slides out as normal (motion::wrap).
	template <typename Thing>
	void wrap(Thing& thing, const sf::Vector2f& velocity, Edge edge, const sf::FloatRect& area)
	{
		xge::Vector2f where = positionOf(thing);
		if (xge::motion::wrap(where, sizeOf(thing), fromSfml(velocity), area.size.x, area.size.y, edge))
		{
			put(thing, where);
		}
	}

	// The same, for a thing moved by keys alone (no velocity of its own): once
	// it has gone right off through that edge, in again from the opposite side.
	template <typename Thing>
	void wrap(Thing& thing, Edge edge, const sf::FloatRect& area)
	{
		wrap(thing, toSfml(xge::motion::outward(edge)), edge, area);
	}

	// A step at once (a frog's hop), unless it would take the thing out of
	// the area: then it stays where it is.
	template <typename Thing>
	void hop(Thing& thing, const sf::Vector2f& step, const sf::FloatRect& area)
	{
		if (xge::motion::fits(positionOf(thing) + fromSfml(step), sizeOf(thing), area.size.x, area.size.y))
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
		if (xge::motion::fits(positionOf(thing) + fromSfml(distance), sizeOf(thing), area.size.x, area.size.y))
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

	// Whether a climber is at a ladder: its middle over it, and its feet
	// between the ladder's top and bottom (half a pixel either way).
	template <typename Climber, typename Ladder>
	bool atLadder(const Climber& climber, const Ladder& ladder)
	{
		return xge::motion::atLadder(positionOf(climber), sizeOf(climber), positionOf(ladder), sizeOf(ladder));
	}

	// One frame at a ladder (<climb>), `step` the way the keys held climb (up
	// below 0), as the engine does it (motion::climb). With no ladder it is off.
	template <typename Thing>
	void climb(Thing& thing, sf::Vector2f& velocity, float step, bool& climbing, bool grounded, const std::optional<sf::FloatRect>& ladder)
	{
		if (!ladder)
		{
			climbing = false;
			return;
		}
		xge::Vector2f where = positionOf(thing);
		xge::Vector2f going = fromSfml(velocity);
		const float across = velocity.x;
		const xge::motion::Climbed climbed = xge::motion::climb(where, going, sizeOf(thing), climbing, grounded, step,
			fromSfml(ladder->position), fromSfml(ladder->size));
		if (!climbed.moved)
		{
			return;
		}
		put(thing, where);
		velocity = toSfml(going);
		if (climbed.gotOff)
		{
			// standing at the end, the way across left to the keys
			velocity.x = across;
		}
	}

	// --- Shots.

	// The way a thing faces (<facing>).
	using Facing = xge::Direction;

	// The way of the four nearest to `way`: the axis it goes further along.
	inline Facing facingOf(sf::Vector2f way)
	{
		return xge::motion::directionOf(fromSfml(way));
	}

	// Puts a shot where the engine's code worked out and gives it its velocity.
	template <typename Shot>
	void launch(Shot& shot, sf::Vector2f& velocity, const xge::motion::Launch& from)
	{
		put(shot, from.position);
		velocity = toSfml(from.velocity);
	}

	// A shot leaving a shooter that faces that way: from the middle of that
	// side, just clear of it, moving that way at `speed`.
	template <typename Shooter, typename Shot>
	void fireFrom(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, Facing facing, float speed)
	{
		launch(shot, velocity, xge::motion::shotFacing(positionOf(shooter), sizeOf(shooter), sizeOf(shot), facing, speed));
	}

	// A shot leaving the top centre of a shooter that has no way it faces: the
	// shot's own middle over the shooter's, so a thin shot leaves the middle of
	// the gun. Where it goes is the shot's own velocity.
	template <typename Shooter, typename Shot>
	void fireUp(const Shooter& shooter, Shot& shot)
	{
		put(shot, xge::motion::shotUp(positionOf(shooter), sizeOf(shooter), sizeOf(shot), xge::Vector2f()).position);
	}

	// The speed up, a negative number, at which a thing leaves the ground to
	// rise `height` pixels against a pull of `pull` a frame.
	inline float leapSpeed(float pull, float height)
	{
		return xge::motion::leapSpeed(pull, height);
	}

	// The way a heading points (<heading>: degrees clockwise from straight
	// up), one pixel long.
	inline sf::Vector2f ahead(float heading)
	{
		return toSfml(xge::motion::ahead(heading));
	}

	// A shot leaving a shooter with a heading: from its middle, out by half
	// its longer side along the heading, at `speed`.
	template <typename Shooter, typename Shot>
	void fireAhead(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, float heading, float speed)
	{
		launch(shot, velocity, xge::motion::shotAhead(positionOf(shooter), sizeOf(shooter), sizeOf(shot), heading, speed));
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
		const std::optional<xge::Vector2f> way = xge::motion::aimed(fromSfml(thing.getGlobalBounds().getCenter()), fromSfml(*there));
		if (!way)
		{
			return std::nullopt;
		}
		return toSfml(*way);
	}

	// A shot leaving a shooter that has aimed: from its middle, just clear of
	// it along the aim, at `speed`.
	template <typename Shooter, typename Shot>
	void fireAlong(const Shooter& shooter, Shot& shot, sf::Vector2f& velocity, sf::Vector2f aim, float speed)
	{
		launch(shot, velocity, xge::motion::shotAlong(positionOf(shooter), sizeOf(shooter), sizeOf(shot), fromSfml(aim), speed));
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
		const xge::motion::Chase chased = xge::motion::chased(fromSfml(thing.getGlobalBounds().getCenter()), fromSfml(*there), speed, near);
		velocity = toSfml(chased.velocity);
		if (!chased.way)
		{
			return std::nullopt;
		}
		return toSfml(*chased.way);
	}

	// --- Against another thing.

	// When, in the step a thing has just taken (by `velocity`, ending where it
	// is), it first met `other`, which is taken to be where it is: the engine's
	// sweep (collide.h), so a thing that moves more than its own size in a frame
	// is not missed, and the touch is where it began and not where the step
	// ended up.
	template <typename Mover, typename Other>
	std::optional<xge::collide::SweepHit> meeting(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		xge::collide::Body begun = bodyOf(mover);
		begun.position = begun.position - fromSfml(velocity);
		return xge::collide::sweep(begun, fromSfml(velocity), bodyOf(other), xge::Vector2f());
	}

	// The middle of a thing at the moment it touched `other`, in the step it has
	// just taken: where the engine has it when the rules about the touch run
	// (a burst that comes out of a missile comes out there, not where the rest of
	// the step took the missile).
	template <typename Mover, typename Other>
	sf::Vector2f middleAtTouch(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		const sf::Vector2f middle = mover.getGlobalBounds().getCenter();
		const std::optional<xge::collide::SweepHit> hit = meeting(mover, velocity, other);
		return hit ? middle - velocity * (1.0f - hit->time) : middle;
	}

	// What a thing that bounces or deflects does at a touch, in the engine's
	// order: back along its step to the moment of the touch, the rule (which says
	// how its velocity changes there), and then the rest of the step, the new
	// way. `rule` is called with the edge of the mover that touched, and gives
	// the velocity it leaves with.
	template <typename Mover, typename Other, typename Rule>
	void atTouch(Mover& mover, sf::Vector2f& velocity, const Other& other, Rule rule)
	{
		std::optional<xge::collide::SweepHit> hit = meeting(mover, velocity, other);
		if (!hit)
		{
			// Not by a step it took (it was put there, or carried): where it is
			// is where it touched.
			const std::optional<xge::Edge> edge = xge::collide::overlap(bodyOf(mover), bodyOf(other));
			if (!edge)
			{
				return;
			}
			hit = xge::collide::SweepHit{ 1.0f, *edge };
		}

		const float rest = 1.0f - hit->time;
		mover.move(-velocity * rest);
		velocity = toSfml(rule(xge::opposite(hit->edgeOfSecond)));
		mover.move(velocity * rest);
	}

	// Out of the side of `other` it went into, heading away from it, as the
	// engine does it (collide::bounced), from where it touched.
	template <typename Mover, typename Other>
	void bounceOff(Mover& mover, sf::Vector2f& velocity, const Other& other)
	{
		const sf::Vector2f before = velocity;
		atTouch(mover, velocity, other, [&](xge::Edge edge)
			{
				return xge::collide::bounced(fromSfml(before), edge);
			});
	}

	// Out of the side of `other` it went into, at the same speed: straight out
	// from the middle of that side, and up to `maxDegrees` towards either end,
	// as the engine does it (collide::deflected), the angle worked out where it
	// touched, and the rest of the step played the new way. A Pong paddle.
	template <typename Mover, typename Other>
	void deflect(Mover& mover, sf::Vector2f& velocity, const Other& other, float maxDegrees)
	{
		const sf::Vector2f before = velocity;
		const xge::collide::Body wall = bodyOf(other);
		atTouch(mover, velocity, other, [&](xge::Edge edge)
			{
				return xge::collide::deflected(fromSfml(before), bodyOf(mover), wall, edge, maxDegrees);
			});
	}

	// Coming down onto the top of `other` (<land />), as the engine does it
	// (motion::land, with the side of the thing that touched worked out by the
	// sweep of the step it just took): it is put on the top, its fall stopped,
	// and the answer is yes. Any other touch does nothing, so a platform can be
	// walked past and jumped up through.
	template <typename Thing, typename Other>
	bool land(Thing& thing, sf::Vector2f& velocity, const Other& other)
	{
		std::optional<xge::collide::SweepHit> hit = meeting(thing, velocity, other);
		if (!hit)
		{
			const std::optional<xge::Edge> edge = xge::collide::overlap(bodyOf(thing), bodyOf(other));
			if (!edge)
			{
				return false;
			}
			hit = xge::collide::SweepHit{ 1.0f, *edge };
		}

		xge::Vector2f where = positionOf(thing);
		xge::Vector2f going = fromSfml(velocity);
		if (!xge::motion::land(where, going, sizeOf(thing), positionOf(other).y, xge::opposite(hit->edgeOfSecond), false))
		{
			return false;
		}
		put(thing, where);
		velocity = toSfml(going);
		return true;
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

	// A frame along its path: the velocity that takes it on, a leg at a time,
	// `begin(path, leg)` doing what a leg does as it begins. At the end it
	// comes to rest where it is.
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
			const xge::motion::Flight flown = xge::motion::fly(fromSfml(rest), route.speed);
			if (flown.done)
			{
				++flight.leg;
				flight.begun = false;
				continue;
			}
			velocity = toSfml(flown.velocity);
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
