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
		// edge of `b` was hit by `a`.
		static std::optional<Edge> overlap(const Object& a, const Object& b);

		// Moves `a` by moveA and `b` by moveB over one step and finds the first
		// moment they touch, if they do at all. Unlike checking where the two
		// end up, this cannot miss a small or fast object passing straight
		// through a thin one between two frames. Two objects that already
		// overlap are a hit at time 0; two that merely touch and are moving
		// apart are not a hit. Each object is swept on its own - nothing here
		// knows about groups.
		static std::optional<SweepHit> sweep(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB);
	};
}
