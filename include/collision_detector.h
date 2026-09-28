// collision_detector.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "command.h"
#include "object.h"

#include <optional>

namespace xge
{
	// Pure geometry: has anything happened, and where? No side effects, no
	// knowledge of Commands - CommandExecutor is what decides what to *do* about
	// a touched edge. Splitting this out of Game::checkEdge and
	// Game::checkObjectCollision is what lets those checks share one response
	// path.
	//
	// Both rectangleRectangle() and circleRectangle() return which edge of
	// their *second* argument was touched (e.g. circleRectangle(a, b) answers
	// "which edge of b did a hit?"). Game::checkObjectCollision is what turns
	// that into a self-relative edge for each side of the pair.
	class CollisionDetector
	{
	public:
		// Has `object` crossed the given screen edge?
		static bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge);

		// A plain axis-aligned bounding-box overlap test - exact enough on its
		// own for two rectangles. Returns which edge of `b` was hit by `a`.
		static std::optional<Edge> rectangleRectangle(const Object& a, const Object& b);

		// A closer geometric test for when at least one participant is a
		// circle, where a bounding-box overlap alone isn't precise enough
		// (e.g. the ball brushing past a corner). Returns which edge of `rect`
		// was hit by `circle`.
		static std::optional<Edge> circleRectangle(const Object& circle, const Object& rect);
	};
}

