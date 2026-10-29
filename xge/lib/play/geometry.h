// geometry.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#pragma once

#include "game/types.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
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

	// Two circles, each filling its box (its width the diameter), closer than
	// their two radii. The edge is the side of the second the first is
	// mostly toward, by the line between their middles.
	std::optional<Edge> circlesOverlap(const Box& a, const Box& b) noexcept;

	// The right test for the shapes: two circles as circles, a circle against
	// the other one's box, whichever way round, or two boxes.
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

	// Everything about a touch, worked out once, where it happens (see
	// contactOf), for every rule and verb that answers it to read.
	struct Contact
	{
		// The edge of the second shape that was touched; the first's is the
		// opposite one.
		Edge edgeOfSecond{ Edge::Top };

		// How far through the frame it happened: 0 its start, 1 its end.
		float time{};

		// Where on the screen the two meet.
		Vector2f point;

		// The way out of the second towards the first, one pixel long: across
		// a flat side, straight out of it; at a circle or a corner, along the
		// line through the point they meet.
		Vector2f normal;

		// Where along the touched side of each the point is, from 0 at its
		// left or top end to 1 at its right or bottom end.
		float alongSecond{};
		float alongFirst{};

		// How far into each other they are along the normal; 0 when they
		// have only just met.
		float depth{};
	};

	// The touch between two shapes as they are now, which have just been
	// found touching on that edge of the second, `time` through the frame.
	Contact contactOf(const Box& a, bool aRound, const Box& b, bool bRound, Edge edgeOfSecond, float time) noexcept;

	// One side's view of a touch: its own edge, the way out of what it
	// touched towards itself, and where along the other's side it touched.
	struct Touch
	{
		Edge edge{ Edge::Top };
		Vector2f point;
		Vector2f normal;
		float along{};
		float depth{};
		float time{};
		// Felt through another thing (a <knock> through a floor): that
		// thing's class; null for a touch itself.
		const std::string* through{ nullptr };
	};

	// The touch as the first or the second of the pair sees it.
	Touch touchOfFirst(const Contact& contact) noexcept;
	Touch touchOfSecond(const Contact& contact) noexcept;

	// The way straight out of a side of a box: up out of its top.
	Vector2f outOf(Edge edge) noexcept;

	// Moves `a` by moveA and `b` by moveB over one step and finds the first
	// moment they touch, if they do at all, so a small or fast thing cannot
	// pass straight through a thin one between two frames. Two that already
	// overlap are a hit at time 0; two that merely touch and are moving apart
	// are not a hit.
	std::optional<SweepHit> sweepShapes(const Box& a, bool aRound, const Vector2f& moveA, const Box& b, bool bRound, const Vector2f& moveB) noexcept;

	// A first look before a sweep: the box a box covers over a whole step
	// (from where it starts to where it ends, with half a pixel to spare all
	// round), and whether two such meet. Two that do not can never touch in
	// that step; most pairs in a busy frame are far apart and stop here. A
	// box covered over part of the step is inside the one for all of it.
	Box sweptBox(const Box& box, const Vector2f& move) noexcept;
	inline bool boxesMeet(const Box& a, const Box& b) noexcept
	{
		return a.left() <= b.right() && b.left() <= a.right() && a.top() <= b.bottom() && b.top() <= a.bottom();
	}

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

	// What a sweep says of a pair through what is left of a frame: when it
	// first touches (`hit`), or that it does not; `far` when it cannot touch
	// at all until a reaction changes one of the two (they are too far apart).
	struct PairSweep
	{
		std::optional<SweepHit> hit;
		bool far{ false };
	};

	// One frame's move, as the engine plays it (playOneFrame): not in one
	// jump, but from touch to touch. Of `pairs`, the two things in each that
	// may touch, in order (each pair names its things by index, .i and .j,
	// of `things` in all), sweep(pair, left) says when, through what is left
	// of the frame, they would first touch (none: they would not, or sit
	// this out); the earliest anywhere is found, advanceTo(played) moves
	// everything to that far through the frame (0 its start, 1 its end),
	// react(pair, edgeOfSecond, played, changed) runs that pair's rules and
	// adds to `changed` every thing it moved, turned, resized, showed or
	// hid, and the rest of the frame carries on from there with whatever
	// velocities the rules left behind. A ball that hits a brick a third of
	// the way through its move spends the other two thirds heading back.
	// Each pair reacts at most once a frame, which also bounds the loop; on a
	// tie the pair first in `pairs` goes first. react is also told how far
	// through the frame the touch came (`played`, as advanceTo was).
	//
	// A pair found far apart is not swept again until a reaction changes one
	// of its two (what is left of the frame only gets shorter), so a busy
	// frame with many reactions sweeps only the pairs near each other again.
	template <typename Pair, typename Sweep, typename Advance, typename React>
	void playFrame(const std::vector<Pair>& pairs, std::size_t things, Sweep sweep, Advance advanceTo, React react)
	{
		// The pairs still to sweep, in order; the far ones are left out of it
		// until a reaction changes one of their two (pairsOf lists every
		// pair each thing is in: pairsOf[first[thing]] to pairsOf[first[thing + 1]]).
		std::vector<std::size_t> awake(pairs.size());
		for (std::size_t p = 0; p < pairs.size(); ++p) { awake[p] = p; }
		std::vector<char> asleep(pairs.size(), 0);

		std::vector<std::size_t> first(things + 1, 0);
		for (const Pair& pair : pairs)
		{
			++first[pair.i + 1];
			++first[pair.j + 1];
		}
		for (std::size_t thing = 0; thing < things; ++thing) { first[thing + 1] += first[thing]; }
		std::vector<std::size_t> pairsOf(first.back());
		{
			std::vector<std::size_t> filled(first.begin(), first.end() - 1);
			for (std::size_t p = 0; p < pairs.size(); ++p)
			{
				pairsOf[filled[pairs[p].i]++] = p;
				pairsOf[filled[pairs[p].j]++] = p;
			}
		}

		std::vector<std::size_t> changed;
		std::vector<std::size_t> woken;

		float played = 0.0f;
		for (std::size_t reactions = 0; reactions <= pairs.size() && played < 1.0f; ++reactions)
		{
			const float left = 1.0f - played;

			std::optional<std::size_t> next;
			SweepHit nextHit;
			std::size_t kept = 0;
			for (const std::size_t p : awake)
			{
				const PairSweep swept = sweep(pairs[p], left);
				if (swept.far)
				{
					asleep[p] = 1;
					continue;
				}

				awake[kept++] = p;
				if (swept.hit && (!next || swept.hit->time < nextHit.time))
				{
					next = p;
					nextHit = *swept.hit;
				}
			}
			awake.resize(kept);

			if (!next)
			{
				break;
			}

			const float step = nextHit.time * left;
			played += step;
			advanceTo(played);
			awake.erase(std::find(awake.begin(), awake.end(), *next));

			changed.clear();
			react(pairs[*next], nextHit.edgeOfSecond, played, changed);

			// What the reaction changed may now meet what it was far from.
			woken.clear();
			for (const std::size_t thing : changed)
			{
				for (std::size_t at = first[thing]; at < first[thing + 1]; ++at)
				{
					const std::size_t p = pairsOf[at];
					if (asleep[p])
					{
						asleep[p] = 0;
						woken.push_back(p);
					}
				}
			}
			if (!woken.empty())
			{
				// One thing's pairs are listed in order already.
				if (changed.size() > 1) { std::sort(woken.begin(), woken.end()); }
				const auto middle = static_cast<std::ptrdiff_t>(awake.size());
				awake.insert(awake.end(), woken.begin(), woken.end());
				std::inplace_merge(awake.begin(), awake.begin() + middle, awake.end());
			}
		}

		advanceTo(1.0f);
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

	// <knock>: two things meeting along `way` (one pixel long, from the
	// first towards the second), going at those velocities, with those
	// masses (0: one that does not move), share out what they carry along
	// that line as things that bounce `bounce` (0 to 1) do; across it each
	// keeps its own. None when they are not coming together.
	struct Knocked
	{
		Vector2f first;
		Vector2f second;
	};
	std::optional<Knocked> knockedApart(const Vector2f& first, float firstMass, const Vector2f& second, float secondMass, const Vector2f& way, float bounce) noexcept;

	// Whether `thing` rests against that side of `base`: up to a pixel
	// from it, and overlapping it along it (a walker standing on a floor
	// tile rests against its top).
	bool restsAgainst(const Box& thing, const Box& base, Edge side) noexcept;

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

	// Of the eight ways (across, up or down, or a true diagonal), one pixel
	// long, the one `apart` lies along to within `within` pixels: straight
	// across when it is that near level, up or down when that near above or
	// below, a diagonal when its two halves differ by no more. None when it
	// lies along none, or is no way at all.
	std::optional<Vector2f> linedUp(const Vector2f& apart, float within) noexcept;

	// A <heading> (degrees clockwise from straight up) as a way one pixel
	// long, and back.
	Vector2f ahead(float heading) noexcept;
	float headingOf(const Vector2f& way) noexcept;

	// <seek>: `velocity` turned toward `way` by at most `degrees`, at the
	// same speed. Still, it stays still.
	Vector2f turnedToward(const Vector2f& velocity, const Vector2f& way, float degrees) noexcept;

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
	// off, unless it holds at that end (`holdTop`, `holdBottom`: nothing to
	// stand on there), stopped. The step is its velocity,
	// straight up or down, so it moves, and touches what it passes, as
	// anything does. The answer is whether it was on the ladder this frame
	// (`climbing` says whether it still is).
	bool climbLadder(Vector2f& position, Vector2f& velocity, const Vector2f& size, float step, bool& climbing, bool grounded, const Box& ladder,
		bool holdTop, bool holdBottom) noexcept;

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
