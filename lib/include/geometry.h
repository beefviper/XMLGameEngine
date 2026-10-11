// geometry.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#pragma once

#include "types.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace xge
{
	// Where things are, whether they touch, and where a touch or a verb sends
	// them, on plain numbers: a box is a top left and a size, and a velocity is
	// how far a thing goes in a frame. Nothing here knows about Object, a game
	// file or a window library, so the engine (CollisionDetector,
	// CommandExecutor, Game) and a program xgecli generates (whose physics.h
	// hands these SFML's shapes) work a game out with the same code.

	struct Box
	{
		Vector2f position;
		Vector2f size;

		float left() const noexcept { return position.x; }
		float top() const noexcept { return position.y; }
		float right() const noexcept { return position.x + size.x; }
		float bottom() const noexcept { return position.y + size.y; }
		Vector2f middle() const noexcept { return position + size * 0.5f; }
	};

	// --- Touching. Every test answers with the edge of its *second* box that
	// the first touched; the first's own edge is opposite() of it.

	// Two boxes overlapping by more than an edge. The edge is the side the
	// first has gone into the least, the one that has only just been touched.
	std::optional<Edge> boxesOverlap(const Box& a, const Box& b) noexcept;

	// The circle that fills the box `circle` (its width the diameter) against
	// a box, by the box's nearest point to the circle's middle: so a ball can
	// pass the corner of a box its own box would clip. With the middle inside
	// the box, the edge is the side nearest to it.
	std::optional<Edge> circleOverlapsBox(const Box& circle, const Box& box) noexcept;

	// The right test for the shapes: a circle against the other one's box,
	// whichever way round (two circles are a circle and a box), or two boxes.
	std::optional<Edge> shapesOverlap(const Box& a, bool aRound, const Box& b, bool bRound) noexcept;

	// When, within one step of motion, two shapes first touch.
	struct SweepHit
	{
		// How far through the step it happens: 0 is the start (they were
		// already touching), 1 is the end of the move.
		float time{};

		// The edge of the second shape that was hit.
		Edge edgeOfSecond{ Edge::Top };
	};

	// Moves `a` by moveA and `b` by moveB over one step and finds the first
	// moment they touch, if they do at all, so a small or fast thing cannot
	// pass straight through a thin one between two frames. Two that already
	// overlap are a hit at time 0; two that merely touch and are moving apart
	// are not a hit.
	std::optional<SweepHit> sweepShapes(const Box& a, bool aRound, const Vector2f& moveA, const Box& b, bool bRound, const Vector2f& moveB) noexcept;

	// Whether the point local (measured from the box's top left) is inside the
	// shape: a box, or the circle that fills it.
	bool solidShapeAt(const Vector2f& size, bool round, const Vector2f& local) noexcept;

	// Whether any pixel of the screen, taken at its middle, is solid in both
	// things where both cover it. solidA and solidB are asked about a point
	// measured from their own box's top left (see solidShapeAt; a picture
	// answers from its pixels).
	template <typename SolidA, typename SolidB>
	bool pixelsOverlapAt(const Box& a, SolidA solidA, const Box& b, SolidB solidB)
	{
		// Only the stretch of screen both of them cover needs looking at.
		const int left = static_cast<int>(std::floor(std::max(a.left(), b.left())));
		const int top = static_cast<int>(std::floor(std::max(a.top(), b.top())));
		const int right = static_cast<int>(std::ceil(std::min(a.right(), b.right())));
		const int bottom = static_cast<int>(std::ceil(std::min(a.bottom(), b.bottom())));

		for (int y = top; y < bottom; ++y)
		{
			for (int x = left; x < right; ++x)
			{
				const Vector2f point{ static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f };
				if (solidA(point - a.position) && solidB(point - b.position))
				{
					return true;
				}
			}
		}

		return false;
	}

	// The side of the second thing that something moving by `relative`
	// against it comes in through, by whichever way it is mostly going.
	Edge edgeFromMotion(const Vector2f& relative) noexcept;

	// After the boxes of two pixel-tested things have been found to touch at
	// `boxTime` through a step (sweepShapes), walks the rest of the step, half
	// a pixel of the longer way at a time, until touchesAt(time) says the
	// pixels themselves touch, then narrows it down; the hit's edge comes from
	// the motion. `relative` is the first's move less the second's.
	template <typename TouchesAt>
	std::optional<SweepHit> firstPixelHit(const Vector2f& relative, float boxTime, TouchesAt touchesAt)
	{
		const float reach = std::max(std::abs(relative.x), std::abs(relative.y)) * (1.0f - boxTime);
		const int steps = std::max(1, static_cast<int>(std::ceil(reach / 0.5f)));

		float previous = boxTime;
		for (int i = 0; i <= steps; ++i)
		{
			const float time = boxTime + (1.0f - boxTime) * static_cast<float>(i) / static_cast<float>(steps);

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

				return SweepHit{ high, edgeFromMotion(relative) };
			}

			previous = time;
		}

		return std::nullopt;
	}

	// When, within one step, two things first touch, whatever they are tested
	// by: two already touching are a hit at 0, by their shapes, and, when
	// `pixels` (either is tested pixel by pixel), by their pixels too, solidA
	// and solidB saying which are solid (see pixelsOverlapAt); otherwise the
	// sweep of their shapes, and, with pixels, the walk on from where their
	// boxes first meet to where pixels of both do (firstPixelHit).
	template <typename SolidA, typename SolidB>
	std::optional<SweepHit> sweepThings(const Box& a, bool aRound, SolidA solidA, const Vector2f& moveA,
		const Box& b, bool bRound, SolidB solidB, const Vector2f& moveB, bool pixels)
	{
		if (const std::optional<Edge> edge = shapesOverlap(a, aRound, b, bRound))
		{
			if (!pixels || pixelsOverlapAt(a, solidA, b, solidB))
			{
				return SweepHit{ 0.0f, *edge };
			}
		}

		const std::optional<SweepHit> boxHit = sweepShapes(a, aRound, moveA, b, bRound, moveB);

		// The boxes never touched: nor did any pixels. They did, and pixels are
		// what counts: they may yet not, or only later in the step.
		if (!boxHit || !pixels)
		{
			return boxHit;
		}

		return firstPixelHit(moveA - moveB, boxHit->time, [&](float time)
			{
				return pixelsOverlapAt(Box{ a.position + moveA * time, a.size }, solidA, Box{ b.position + moveB * time, b.size }, solidB);
			});
	}

	// One frame's move, as the engine plays it (Game::moveObjects): not in one
	// jump, but from touch to touch. Of `pairs`, the two things in each that
	// may touch, in order, sweep(pair, left) says when, through what is left
	// of the frame, they would first touch (none: they would not, or sit this
	// out); the earliest anywhere is found, advance(fraction) moves everything
	// that far through the frame, react(pair, edgeOfSecond) runs that pair's
	// rules, and the rest of the frame carries on from there with whatever
	// velocities the rules left behind. A ball that hits a brick a third of the
	// way through its move spends the other two thirds heading back. Each pair
	// reacts at most once a frame, which also bounds the loop; on a tie the
	// pair first in `pairs` goes first.
	template <typename Pair, typename Sweep, typename Advance, typename React>
	void playFrame(const std::vector<Pair>& pairs, Sweep sweep, Advance advance, React react)
	{
		std::vector<bool> handled(pairs.size(), false);
		float played = 0.0f;
		for (std::size_t reactions = 0; reactions <= pairs.size() && played < 1.0f; ++reactions)
		{
			const float left = 1.0f - played;

			std::optional<std::size_t> next;
			SweepHit nextHit;
			for (std::size_t i = 0; i < pairs.size(); ++i)
			{
				if (handled[i])
				{
					continue;
				}

				const std::optional<SweepHit> hit = sweep(pairs[i], left);
				if (hit && (!next || hit->time < nextHit.time))
				{
					next = i;
					nextHit = *hit;
				}
			}

			if (!next)
			{
				break;
			}

			const float step = nextHit.time * left;
			advance(step);
			played += step;
			handled[*next] = true;
			react(pairs[*next], nextHit.edgeOfSecond);
		}

		advance(1.0f - played);
	}

	// --- The window's edges.

	// Whether the thing has gone past that edge of a window `window` big,
	// which is when its rules about the edge run.
	bool pastWindowEdge(const Box& thing, const Vector2f& window, Edge edge) noexcept;

	// Whether all of the box is inside the window: where a hop or a jump may land.
	bool insideWindow(const Box& at, const Vector2f& window) noexcept;

	// --- What the verbs do. `edge` is always the thing's own edge that was
	// touched (of itself, at the window, or against another thing).

	// <bounce />: heading away from that edge, at the same speed.
	Vector2f bounced(const Vector2f& velocity, Edge edge) noexcept;

	// <deflect>: away from `other` at the same speed, at an angle set by where
	// it hit. How far the mover's middle is from the middle of the touched
	// side, as a share of half that side's length (-1 at one end, 1 at the
	// other, held there past the ends), times maxDegrees, is the angle away
	// from straight back out, towards the end it hit nearer to: off a Pong
	// paddle's top half the ball goes back up, off its bottom half down.
	Vector2f deflected(const Vector2f& velocity, const Box& mover, const Box& other, Edge edge, float maxDegrees) noexcept;

	// <stick />: pushed back inside the window on the touched edge's own axis,
	// losing only the velocity carrying it into the edge; the other way it
	// keeps going, so a thing pressed against the floor still slides.
	void stickInside(Vector2f& position, Vector2f& velocity, const Vector2f& size, const Vector2f& window, Edge edge) noexcept;

	// <wrap />: once the thing has gone right off the window through that
	// edge, and is still heading that way, in again from the opposite side,
	// one window and one of its own sizes along, so things spaced along a lane
	// stay spaced. Until it has gone completely it slides out as normal.
	void wrapAround(Vector2f& position, const Vector2f& size, const Vector2f& heading, const Vector2f& window, Edge edge) noexcept;

	// <land />: coming down onto the top of `other` (its own bottom edge
	// touched, not going up, not on a ladder) it stands there: put on the top,
	// its fall stopped, and the answer is yes. Any other touch does nothing,
	// so a platform can be walked past and jumped up through.
	bool landOn(Vector2f& position, Vector2f& velocity, const Vector2f& size, const Box& other, Edge edge, bool climbing) noexcept;

	// <leap>: the speed up that, under a pull of `pull` a frame, rises
	// `height`: v * v = 2 * pull * height.
	float leapSpeed(float pull, float height) noexcept;

	// A distance one of the four ways: a <hop>, or a <jump>'s whole way.
	Vector2f stepToward(Direction direction, float distance) noexcept;

	// The way of the four nearest to `way`: the axis it goes further along.
	Direction directionOf(const Vector2f& way) noexcept;

	// The way from one point to another, one pixel long; none at the point.
	std::optional<Vector2f> wayTo(const Vector2f& from, const Vector2f& to) noexcept;

	// A <heading> (degrees clockwise from straight up) as a way one pixel
	// long, and back.
	Vector2f ahead(float heading) noexcept;
	float headingOf(const Vector2f& way) noexcept;

	// <chase>: straight from `from` at `to` at `speed`, or, within stopWithin
	// of it, still. The way it set off, one pixel long, if it did.
	std::optional<Vector2f> chase(const Vector2f& from, const Vector2f& to, float speed, float stopWithin, Vector2f& velocity) noexcept;

	// Whether a climber is at a ladder: its middle over it, and its feet
	// between the ladder's top and bottom (half a pixel either way).
	bool atLadder(const Box& climber, const Box& ladder) noexcept;

	// One frame at a ladder (<climb>), `step` the way the keys held climb (up
	// below 0). Off it, it gets on only from the ground and when it can go
	// that way, lined up with the ladder's middle. On it, it goes the step,
	// its feet kept between the ladder's ends; at an end, going on, it gets
	// off. The answer is whether it was on the ladder this frame (it then has
	// no velocity of its own; `climbing` says whether it still is).
	bool climbLadder(Vector2f& position, const Vector2f& size, float step, bool& climbing, bool grounded, const Box& ladder) noexcept;

	// --- Shots (<fire>), at the projectile's own speed.

	struct Shot
	{
		Vector2f position;
		Vector2f velocity;
	};

	// Along an aim (one pixel long), from the shooter's middle, just clear of it.
	Shot shotAlong(const Box& shooter, const Vector2f& shotSize, const Vector2f& aim, float speed) noexcept;

	// Along a heading, from the shooter's middle, out by half its longer side.
	Shot shotAhead(const Box& shooter, const Vector2f& shotSize, float heading, float speed) noexcept;

	// From the middle of the side the shooter faces, just clear of it.
	Shot shotFacing(const Box& shooter, const Vector2f& shotSize, Direction facing, float speed) noexcept;

	// From the shooter's top, the shot's middle over the shooter's, so a thin
	// shot leaves the middle of the gun (the shot keeps its own velocity).
	Vector2f shotFromTop(const Box& shooter, const Vector2f& shotSize) noexcept;

	// --- Paths (<paths>, <follow>).

	// The velocity that takes a thing on along what is left of a leg at
	// `speed` a frame: all of it when that is nearer, none when the leg is done.
	std::optional<Vector2f> pathStep(const Vector2f& left, float speed) noexcept;
}
