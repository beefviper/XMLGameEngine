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

		// object.collisionData.basic (circle-vs-rectangle), driven by
		// CollisionDetector::circleRectangle. `isPrimaryMover` is true for the
		// circular object that detected the hit, false for the object it hit -
		// it only affects `die`, which additionally halts and parks the primary
		// mover off-screen, matching the original circleRectangleCollision.
		void executeObjectCollision(const Command& command, Object& object, Edge edge, bool isPrimaryMover);

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

