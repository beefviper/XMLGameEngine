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

		// object.collisionData.basic, driven by Game::applyObjectCollision.
		// `other` is the object being touched. `edge` must already be
		// self-relative to `object` (the edge of *this* object that was
		// touched) - see the comment on CollisionDetector for why that's not
		// simply the detector's raw result.
		void executeObjectCollision(const Command& command, Object& object, const Object& other, Edge edge);

		// A command bound to a key in the current State's <input> list: push or
		// pop a state, or trigger a named action on another object.
		void executeInput(const Command& command, bool keyPressed);

		// A State's <condition> action, once Game::checkConditions decides it's
		// met: same command set as an <input> (in practice just push/pop a
		// state), so this is just executeInput as an always-"pressed" input.
		void executeCondition(const Command& command);

		// A key that was already down when a state began: applies only the
		// continuous part of its binding (an action's move.* and accelerate.*) and returns true
		// if the command was one of those. One-shot commands (state changes,
		// fire) are left alone, since the key was never pressed in this state.
		bool executeHeldInput(const Command& command);

	private:
		Game& game;

		void die(Object& object);
		void bounceScreenEdge(Object& object, Edge edge);
		void bounceOffEdge(Object& object, Edge edge);
		void deflect(Object& object, const Object& other, Edge edge, float maxAngle);
		void stick(Object& object, Edge edge);
		void moveByStep(Object& object, Direction direction, float step);
		void wrap(Object& object, Edge edge);
		void carry(Object& object, const Object& other);
		void queueHop(Object& object, Direction direction, float distance);
		void stop(Object& object);
		void applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn);
		void applyActionTurn(Object& object, Direction direction, float rate);
		void applyActionThrustAhead(Object& object, float amount, const std::string& burn);
		void release(const Object& from, const std::string& target, int count);
		void restart(Object& object);

		void triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed);
		void applyActionVelocity(Object& object, Direction direction, float step);
		void spawnProjectile(Object& shooter, const std::string& projectileName);
	};
}
