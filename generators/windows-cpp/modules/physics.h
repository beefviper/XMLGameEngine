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
#include <map>
#include <set>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace physics
{
	// --- Where a thing is.

	// What a thing covers: its bounds, but a circle's is the square it is drawn
	// in, as the engine has it (an sf::CircleShape's own bounds are those of the
	// polygon that draws it, a little inside the circle).
	template <typename Thing>
	sf::FloatRect bounds(const Thing& thing)
	{
		if constexpr (std::is_same_v<Thing, sf::CircleShape>)
		{
			const float side = thing.getRadius() * 2.0f;
			return sf::FloatRect(thing.getPosition(), sf::Vector2f(side, side));
		}
		else if constexpr (std::is_same_v<Thing, sf::Text>)
		{
			// the engine measures a text to whole pixels, up
			const sf::FloatRect shown = thing.getGlobalBounds();
			return sf::FloatRect(shown.position, sf::Vector2f(std::ceil(shown.size.x), std::ceil(shown.size.y)));
		}
		else
		{
			return thing.getGlobalBounds();
		}
	}

	template <typename Thing>
	float left(const Thing& thing)
	{
		return bounds(thing).position.x;
	}

	template <typename Thing>
	float right(const Thing& thing)
	{
		const sf::FloatRect box = bounds(thing);
		return box.position.x + box.size.x;
	}

	template <typename Thing>
	float top(const Thing& thing)
	{
		return bounds(thing).position.y;
	}

	template <typename Thing>
	float bottom(const Thing& thing)
	{
		const sf::FloatRect box = bounds(thing);
		return box.position.y + box.size.y;
	}

	template <typename Thing>
	float width(const Thing& thing)
	{
		return bounds(thing).size.x;
	}

	template <typename Thing>
	float height(const Thing& thing)
	{
		return bounds(thing).size.y;
	}

	// A thing as the engine's geometry sees it (collide.h, copied in beside this
	// file as it is): where it is, how big, and whether it is a circle.
	template <typename Thing>
	xge::collide::Body bodyOf(const Thing& thing)
	{
		const sf::FloatRect box = bounds(thing);
		xge::collide::Body seen;
		seen.position = xge::Vector2f(box.position.x, box.position.y);
		seen.size = xge::Vector2f(box.size.x, box.size.y);
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
	bool overlapping(const First& first, const Second& second)
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

	// Where the last pixel touch was, for the thing that touched: toContact
	// takes it back there if it stops (a pixel touch is not the boxes').
	struct PixelContact
	{
		const void* mover = nullptr;
		const void* other = nullptr;
		xge::Vector2f position;
		xge::Vector2f otherPosition;
		xge::collide::SweepHit hit;
		float time = 1.0f;
	};

	inline PixelContact& pixelContact()
	{
		static PixelContact contact;
		return contact;
	}

	// Whether two things touch pixel by pixel (<type>pixel</type>): some
	// pixel of the screen both cover is solid in both, as the engine tests it
	// (collide::sweep). `pixels` is the picture a sprite shows, or nullptr for
	// its whole box (or circle). `step` and `otherStep` are how far each
	// moved this frame: they are looked at all along the way, half a pixel at a
	// time, so that a fast shot does not jump over a line two pixels thick.
	template <typename First, typename Second>
	bool touchingPixels(const First& first, const sf::Image* firstPixels, const Second& second, const sf::Image* secondPixels, sf::Vector2f step = {}, sf::Vector2f otherStep = {})
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
		target.position = target.position - fromSfml(otherStep);

		const auto hit = xge::collide::sweep(mover, fromSfml(step), target, fromSfml(otherStep));
		if (hit)
		{
			pixelContact() = PixelContact{ &first, &second, mover.position + fromSfml(step) * hit->time,
				target.position + fromSfml(otherStep) * hit->time, *hit, hit->time };
		}
		return hit.has_value();
	}

	// --- The edges of an area (the window).

	using Edge = xge::Edge;

	// A thing's size and where it is, as the engine's movement code
	// (motion.h) takes them.
	template <typename Thing>
	xge::Vector2f positionOf(const Thing& thing)
	{
		return fromSfml(bounds(thing).position);
	}

	template <typename Thing>
	xge::Vector2f sizeOf(const Thing& thing)
	{
		return fromSfml(bounds(thing).size);
	}

	// Moves a thing to the place the engine's code worked out for it.
	template <typename Thing>
	void put(Thing& thing, xge::Vector2f where)
	{
		thing.move(toSfml(where) - bounds(thing).position);
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
		const std::optional<xge::Vector2f> way = xge::motion::aimed(fromSfml(bounds(thing).getCenter()), fromSfml(*there));
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
		const xge::motion::Chase chased = xge::motion::chased(fromSfml(bounds(thing).getCenter()), fromSfml(*there), speed, near);
		velocity = toSfml(chased.velocity);
		if (!chased.way)
		{
			return std::nullopt;
		}
		return toSfml(*chased.way);
	}

	// --- Against another thing.

	// Where each thing began the frame it is in (`begin`, called for everything
	// that can touch as the frame starts), so that a touch is worked out as the
	// engine works it out: from where both things were and where they are now,
	// the one move each made in the frame, found where the two first meet. A thing
	// that was not asked about is taken to have stood still where it is.
	inline std::unordered_map<const void*, sf::Vector2f>& began()
	{
		static std::unordered_map<const void*, sf::Vector2f> things;
		return things;
	}

	template <typename Thing>
	void begin(const Thing& thing)
	{
		began()[&thing] = bounds(thing).position;
	}

	template <typename Thing>
	void begin(const std::vector<Thing>& things)
	{
		for (const Thing& thing : things)
		{
			begin(thing);
		}
	}

	// Where a thing began the frame: when it was asked about, else where it is.
	template <typename Thing>
	sf::Vector2f startOf(const Thing& thing)
	{
		const auto at = began().find(&thing);
		return at != began().end() ? at->second : bounds(thing).position;
	}

	// A touch: when in the frame it happened, on which edge, and where each of
	// the two things was at that moment.
	struct Touch
	{
		xge::collide::SweepHit hit;
		xge::collide::Body mover;
		xge::collide::Body other;
	};

	// When, in the frame, a thing first met `other`, by the engine's sweep
	// (collide.h): so a thing that moves more than its own size in a frame is not
	// missed, a thing that moves into it counts, and the touch is where they were
	// then and not where the frame left them. `velocity` is the step the mover
	// took, if it was not asked about when the frame began.
	template <typename Mover, typename Other>
	std::optional<Touch> meetingNow(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		const PixelContact& pixels = pixelContact();
		if (pixels.mover == &mover && pixels.other == &other)
		{
			// touched pixel by pixel just now: where the pixels first met
			Touch touch;
			touch.hit = pixels.hit;
			touch.mover = bodyOf(mover);
			touch.mover.position = pixels.position;
			touch.other = bodyOf(other);
			touch.other.position = pixels.otherPosition;
			return touch;
		}
		xge::collide::Body a = bodyOf(mover);
		xge::collide::Body b = bodyOf(other);
		const xge::Vector2f endA = a.position;
		const xge::Vector2f endB = b.position;

		const auto began_a = began().find(&mover);
		a.position = began_a != began().end() ? fromSfml(began_a->second) : endA - fromSfml(velocity);
		b.position = fromSfml(startOf(other));

		const xge::Vector2f moveA = endA - a.position;
		const xge::Vector2f moveB = endB - b.position;

		Touch touch;
		touch.mover = a;
		touch.other = b;
		if (const auto hit = xge::collide::sweep(a, moveA, b, moveB))
		{
			touch.hit = *hit;
			touch.mover.position = a.position + moveA * hit->time;
			touch.other.position = b.position + moveB * hit->time;
			return touch;
		}

		// Not by the moves they made (one was put there, or carried): where they
		// are is where they touched.
		a.position = endA;
		b.position = endB;
		const std::optional<xge::Edge> edge = xge::collide::overlap(a, b);
		if (!edge)
		{
			return std::nullopt;
		}
		touch.hit = xge::collide::SweepHit{ 1.0f, *edge };
		touch.mover = a;
		touch.other = b;
		return touch;
	}

	// The touches found this frame, so that a pair is asked about once, as the
	// engine sweeps each pair once: what an earlier rule in the frame did (put
	// the other back where it started) does not change what the later ones are
	// told about the touch. newFrame forgets them.
	inline std::map<std::pair<const void*, const void*>, std::optional<Touch>>& touchesThisFrame()
	{
		static std::map<std::pair<const void*, const void*>, std::optional<Touch>> touches;
		return touches;
	}

	// What came into play during the frame (a burst out of a missile): the engine
	// found its pairs as the frame began, so these touch nothing until the next.
	inline std::set<const void*>& cameInThisFrame()
	{
		static std::set<const void*> these;
		return these;
	}

	template <typename Thing>
	void cameIn(const Thing& thing)
	{
		cameInThisFrame().insert(&thing);
	}

	inline void newFrame()
	{
		touchesThisFrame().clear();
		cameInThisFrame().clear();
		pixelContact() = PixelContact{};
	}

	template <typename Mover, typename Other>
	std::optional<Touch> meeting(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		const std::pair<const void*, const void*> pair(&mover, &other);
		const auto found = touchesThisFrame().find(pair);
		if (found != touchesThisFrame().end())
		{
			return found->second;
		}
		return touchesThisFrame()[pair] = meetingNow(mover, velocity, other);
	}

	// Whether two things touch this frame, as the engine decides it: they
	// overlap (overlapping), or, from where each began the frame to where it is,
	// they met on the way (meeting), so a thing that moves into another, or past
	// it, touches it.
	template <typename First, typename Second>
	bool touching(const First& first, const Second& second)
	{
		if (cameInThisFrame().count(&first) > 0 || cameInThisFrame().count(&second) > 0)
		{
			return false;
		}
		return meeting(first, sf::Vector2f{}, second).has_value();
	}

	// The middle of a thing at the moment it touched `other`: where the engine
	// has it when the rules about the touch run (a burst that comes out of a
	// missile comes out there, not where the rest of the frame took the missile).
	template <typename Mover, typename Other>
	sf::Vector2f middleAtTouch(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		if (pixelContact().mover == &other && pixelContact().other == &mover)
		{
			// touched pixel by pixel: where the two had come to then
			return toSfml(pixelContact().otherPosition) + bounds(mover).size / 2.0f;
		}
		const std::optional<Touch> touch = meeting(mover, velocity, other);
		if (!touch)
		{
			return bounds(mover).getCenter();
		}
		return toSfml(touch->mover.position + touch->mover.size * 0.5f);
	}

	// What a thing that bounces or deflects does at a touch, in the engine's
	// order: back along its move to the moment of the touch, the rule (which says
	// how its velocity changes there), and then the rest of the frame, the new
	// way. `rule` is called with the edge of the mover that touched and where the
	// two were then, and gives the velocity it leaves with. What is left of its
	// frame afterwards is the move from there, for a touch later in the frame.
	template <typename Mover, typename Other, typename Rule>
	void atTouch(Mover& mover, sf::Vector2f& velocity, const Other& other, Rule rule)
	{
		const std::optional<Touch> touch = meeting(mover, velocity, other);
		if (!touch)
		{
			return;
		}

		const sf::Vector2f contact = toSfml(touch->mover.position);
		const float rest = 1.0f - touch->hit.time;
		mover.move(contact - bounds(mover).position);
		velocity = toSfml(rule(xge::opposite(touch->hit.edgeOfSecond), touch->mover, touch->other));
		mover.move(velocity * rest);
		began()[&mover] = contact;
	}

	// Back along its move to where it first touched `other`, for a thing that
	// stops there (<stop />): what was left of its move is not made.
	template <typename Mover, typename Other>
	void toContact(Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		if (pixelContact().mover == &mover)
		{
			// touched pixel by pixel: where the pixels first met
			mover.move(toSfml(pixelContact().position) - bounds(mover).position);
			pixelContact().mover = nullptr;
			return;
		}
		const std::optional<Touch> touch = meeting(mover, velocity, other);
		if (!touch)
		{
			return;
		}
		mover.move(toSfml(touch->mover.position) - bounds(mover).position);
	}

	// When, in the frame, `mover` first met `other`, from 0 to 1 (1: it did not).
	template <typename Mover, typename Other>
	float timeOf(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		const std::optional<Touch> touch = meeting(mover, velocity, other);
		return touch ? touch->hit.time : 1.0f;
	}

	// Whether two things overlap `when` of the way through the frame, from where
	// each began it to where it is: what a rule about a touch asks about the rest
	// of the field (an unless=) at the moment of the touch.
	template <typename First, typename Second>
	bool overlappingAt(const First& first, const Second& second, float when)
	{
		xge::collide::Body a = bodyOf(first);
		xge::collide::Body b = bodyOf(second);
		const xge::Vector2f startA = fromSfml(startOf(first));
		const xge::Vector2f startB = fromSfml(startOf(second));
		a.position = startA + (a.position - startA) * when;
		b.position = startB + (b.position - startB) * when;
		return xge::collide::overlap(a, b).has_value();
	}

	// What is left of the frame once `mover` has touched `other`: the part of its
	// move after the touch, from 0 to 1.
	template <typename Mover, typename Other>
	float rest(const Mover& mover, sf::Vector2f velocity, const Other& other)
	{
		const PixelContact& pixels = pixelContact();
		if ((pixels.mover == &mover && pixels.other == &other) || (pixels.mover == &other && pixels.other == &mover))
		{
			return 1.0f - pixels.time;
		}
		const std::optional<Touch> touch = meeting(mover, velocity, other);
		return touch ? 1.0f - touch->hit.time : 0.0f;
	}

	// A thing put back where it started at a touch goes on with what is left of
	// its move (`motion` the whole of it, `left` the part after the touch), from
	// there.
	template <typename Thing>
	void carryOn(Thing& thing, sf::Vector2f motion, float left)
	{
		thing.move(motion * left);
	}

	// Out of the side of `other` it went into, heading away from it, as the
	// engine does it (collide::bounced), from where it touched.
	template <typename Mover, typename Other>
	void bounceOff(Mover& mover, sf::Vector2f& velocity, const Other& other)
	{
		const sf::Vector2f before = velocity;
		atTouch(mover, velocity, other, [&](xge::Edge edge, const xge::collide::Body&, const xge::collide::Body&)
			{
				return xge::collide::bounced(fromSfml(before), edge);
			});
	}

	// Turned round at the touch: back along its move to where it touched `other`,
	// the other way, and the rest of its frame that way (<reverse /> in a rule
	// about a touch).
	template <typename Mover, typename Other>
	void reverseOff(Mover& mover, sf::Vector2f& velocity, const Other& other)
	{
		if (!meeting(mover, velocity, other))
		{
			velocity = -velocity;
			return;
		}
		const sf::Vector2f before = velocity;
		atTouch(mover, velocity, other, [&](xge::Edge, const xge::collide::Body&, const xge::collide::Body&)
			{
				return fromSfml(-before);
			});
	}

	// Out of the side of `other` it went into, at the same speed: straight out
	// from the middle of that side, and up to `maxDegrees` towards either end,
	// as the engine does it (collide::deflected), the angle worked out where the
	// two were when they touched, and the rest of the frame played the new way. A
	// Pong paddle.
	template <typename Mover, typename Other>
	void deflect(Mover& mover, sf::Vector2f& velocity, const Other& other, float maxDegrees)
	{
		const sf::Vector2f before = velocity;
		atTouch(mover, velocity, other, [&](xge::Edge edge, const xge::collide::Body& then, const xge::collide::Body& wall)
			{
				return xge::collide::deflected(fromSfml(before), then, wall, edge, maxDegrees);
			});
	}

	// Coming down onto the top of `other` (<land />), as the engine does it
	// (motion::land, with the side of the thing that touched worked out by the
	// sweep): it is put on the top of it as it was at the touch, its fall stopped,
	// and the answer is yes. Any other touch does nothing, so a platform can be
	// walked past and jumped up through.
	template <typename Thing, typename Other>
	bool land(Thing& thing, sf::Vector2f& velocity, const Other& other, std::optional<float> across = std::nullopt)
	{
		const std::optional<Touch> touch = meeting(thing, velocity, other);
		if (!touch)
		{
			return false;
		}

		xge::Vector2f where = positionOf(thing);
		xge::Vector2f going = fromSfml(velocity);
		if (!xge::motion::land(where, going, sizeOf(thing), touch->other.position.y, xge::opposite(touch->hit.edgeOfSecond), false))
		{
			return false;
		}
		if (across)
		{
			// from a leap: the way across the keys held give it takes up at the
			// landing, and the rest of the frame goes that way
			where.x += (*across - going.x) * (1.0f - touch->hit.time);
			going.x = *across;
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
