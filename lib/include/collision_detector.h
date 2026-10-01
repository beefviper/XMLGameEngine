// collision_detector.h
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#pragma once

#include "command.h"
#include "object.h"

#include <optional>

namespace xge
{
	// Pure geometry: has anything happened, and where? No side effects, no
	// knowledge of Commands - CommandExecutor is what decides what to *do* about
	// a touched edge. Splitting this out of Game is what lets every check share
	// one response path.
	//
	// The object-against-object functions all answer with which edge of their
	// *second* argument was touched (e.g. overlap(a, b) answers "which edge of
	// b did a hit?"). Game::applyObjectCollision turns that into a self-relative
	// edge for each side of the pair.
	class CollisionDetector
	{
	public:
		// When, within one step of motion, two objects first touch.
		struct SweepHit
		{
			// How far through the step it happens: 0 is the start (they were
			// already touching), 1 is the end of the move.
			float time{};

			// The edge of the second object that was hit.
			Edge edgeOfSecond{ Edge::Top };
		};

		// Has `object` crossed the given screen edge?
		static bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge);

		// A plain axis-aligned bounding-box overlap test - exact enough on its
		// own for two rectangles. Returns which edge of `b` was hit by `a`.
		static std::optional<Edge> rectangleRectangle(const Object& a, const Object& b);

		// A closer geometric test for when at least one participant is a
		// circle, where a bounding-box overlap alone isn't precise enough
		// (e.g. the ball brushing past a corner). Returns which edge of `rect`
		// was hit by `circle`, however far into it the circle's centre is.
		static std::optional<Edge> circleRectangle(const Object& circle, const Object& rect);

		// Are the two overlapping right now? Picks the right test above for the
		// shapes involved (a circle is measured against the other object's
		// bounding box, whichever way round they are given), and returns which
		// edge of `b` was hit by `a`. When either object's collisions are of
		// type "pixel" this is also the second test: the boxes have to overlap
		// first, and then some pixel of each has to lie on a pixel of the other
		// (pixelsOverlap). The edge is then still the boxes' edge - a pixel
		// touch knows where pixels met, not which way a surface faces.
		static std::optional<Edge> overlap(const Object& a, const Object& b);

		// Whether either object's collisions are of type pixel, so that a pair
		// is only a hit where pixels actually meet.
		static bool usesPixels(const Object& a, const Object& b) noexcept;

		// Do any two pixels, one drawn by each object, fall on the same spot on
		// the screen? An object of type pixel is its picture (a sprite of lines,
		// see Bitmap), with empty pixels where nothing was drawn; any other
		// object, in a pair with one that is, counts as solid all over its
		// shape (a rectangle, or a circle as a circle). Each pixel of the
		// screen is asked about at its centre.
		static bool pixelsOverlap(const Object& a, const Object& b);

		// Moves `a` by moveA and `b` by moveB over one step and finds the first
		// moment they touch, if they do at all. Unlike checking where the two
		// end up, this cannot miss a small or fast object passing straight
		// through a thin one between two frames. Two objects that already
		// overlap are a hit at time 0; two that merely touch and are moving
		// apart are not a hit. Each object is swept on its own - nothing here
		// knows about lockstep. A pair of type pixel is swept as boxes first and
		// then walked along the same path, half a pixel at a time, from the
		// moment the boxes touch until pixels do (the hit's time is where they
		// first do, found to a fraction of that), and the edge reported is the
		// one the motion came in through.
		static std::optional<SweepHit> sweep(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB);
	};
}
