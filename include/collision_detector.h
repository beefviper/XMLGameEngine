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
	// Game::circleRectangleCollision is what lets those two, previously very
	// different-looking, checks share one response path.
	class CollisionDetector
	{
	public:
		// Has `object` crossed the given screen edge?
		static bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge);

		// If `circle` overlaps `rect`, returns which edge of `rect` was hit.
		static std::optional<Edge> circleRectangle(const Object& circle, const Object& rect);
	};
}

