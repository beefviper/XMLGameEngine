// command_executor.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "command.h"
#include "object.h"

#include <string>

namespace xge
{
	class Game;

	// Response: given a Command that CollisionDetector (or a state's <input>)
	// says applies, do the thing. This is the one place collision/action
	// responses are implemented now, instead of three separate hand-rolled
	// interpreters.
	class CommandExecutor
	{
	public:
		explicit CommandExecutor(Game& game) noexcept : game(game) {}

		// object.collisionData.{top,bottom,left,right}, driven by
		// CollisionDetector::touchesScreenEdge.
		void executeScreenEdgeCollision(const Command& command, Object& object, Edge edge);

		// object.collisionData.basic, driven by Game::checkObjectCollision.
		// `edge` must already be self-relative to `object` (the edge of
		// *this* object that was touched) - see the comment on
		// CollisionDetector for why that's not simply the detector's raw
		// result. Only affects `die` differently depending on shape: a
		// circular object (the ball, a bullet) additionally halts and parks
		// off-screen, matching the original circleRectangleCollision;
		// anything else just stops colliding.
		void executeObjectCollision(const Command& command, Object& object, Edge edge);

		// A command bound to a key in the current State's <input> list: push or
		// pop a state, or trigger a named action on another object.
		void executeInput(const Command& command, bool keyPressed);

	private:
		Game& game;

		void bounceScreenEdge(Object& object, Edge edge);
		void bounceOffEdge(Object& object, Edge edge);
		void stick(Object& object, Edge edge);
		void moveByStep(Object& object, Direction direction, float step);

		void triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed);
		void applyActionVelocity(Object& object, Direction direction, float step);
		void spawnProjectile(Object& shooter, const std::string& projectileName);
	};
}

