// executor.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "game/command.h"
#include "engine/keys.h"
#include "game/object.h"
#include "play/geometry.h"

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
		explicit CommandExecutor(Game& gameToRun) noexcept : game(gameToRun) {}

		// object.collisionData.{top,bottom,left,right}, driven by
		// CollisionDetector::touchesScreenEdge. Each of these returns false
		// when the verb is refused (a <spawn> or <fire> with every copy out,
		// an <aim> not lined up, a hop that cannot be made), which ends the
		// rule, key or condition there, as it ends an action.
		bool executeScreenEdgeCollision(const Command& command, Object& object, Edge edge);

		// object.collisionData.others, driven by Game::reactTo. `other` is
		// the object being touched. `touch` must already be self-relative to
		// `object` (touch.edge the edge of *this* object that was touched,
		// touch.normal the way out of `other` towards it) - see the comment
		// on CollisionDetector for why that's not simply the detector's raw
		// result.
		bool executeObjectCollision(const Command& command, Object& object, Object& other, const Touch& touch);

		// The same with only the edge known: a touch straight across it.
		bool executeObjectCollision(const Command& command, Object& object, Object& other, Edge edge);

		// A command bound to a key in the current State's <input> list: push or
		// pop a state, or trigger a named action on another object.
		bool executeInput(const Command& command, bool keyPressed);

		// A State's <condition> action, once Game::checkConditions decides it's
		// met: same command set as an <input> (in practice just push/pop a
		// state), so this is just executeInput as an always-"pressed" input.
		bool executeCondition(const Command& command);

		// A <timer> that has gone off: on an object (owner) or in a state (no
		// owner). The state commands, inc, dec, play and trigger work in both;
		// on an object, fire, reverse, move, die, stop, release and a bare reset
		// act on that object (a bare reset in a state resets the game).
		// False when the command is a verb that could not happen (see
		// triggerObjectAction), and the timer's other commands are skipped.
		bool executeTimer(const Command& command, Object* owner);

		// A key that was already down when a state began: applies only the
		// continuous part of its binding (an action's move.* and accelerate.*,
		// a sound that loops) and returns true
		// if the command was one of those. One-shot commands (state changes,
		// fire) are left alone, since the key was never pressed in this state.
		bool executeHeldInput(const Command& command);

		// A command bound to a control that points (<input axis="mouse.x">),
		// run when it moves: a <trigger>'s action runs its <track />s,
		// `fraction` (0 to 1) of the way across the window, along the
		// axis's own direction.
		void executeAxis(const Command& command, Axis axis, float fraction);

	private:
		Game& game;

		// The command as it runs: itself, or one made as it runs (CmdLive)
		// made again into `made` (see Game::makeNow).
		const Command& now(const Command& command, Command& made);

		// <spawn>: see CmdSpawn; false, and nothing made, when no more of
		// it may exist (<most>).
		bool spawn(const CmdSpawn& command);

		// <despawn />: `self` (none in a state's list), or with object= the
		// objects it names.
		void despawn(Object* self, const CmdDespawn& command);

		// <carry />: see CmdCarry.
		void carry(Object& object, const Object& item);

		// A <play> on a key, or in the action a key triggers: the press asks
		// for the sound; letting go silences it if it loops.
		void playHeld(const std::string& sound, bool keyPressed);

		void bounceScreenEdge(Object& object, Edge edge);
		void moveByStep(Object& object, Direction direction, float step);
		void queueHop(Object& object, Vector2f step);
		bool startJump(Object& object, const CmdJump& jump);

		// A <hop>: at once (queued), or with <seconds> through the air, as a
		// jump goes; either way the object then faces that way. False when
		// it is in the air, or the hop would land outside the window.
		bool hop(Object& object, const CmdHop& hop);

		// A hunt by hops: see CmdHuntHop.
		bool huntByHop(Object& object, const CmdHuntHop& hunt);

		// Through the air by `whole` over `seconds`, touching nothing until it
		// lands: a <jump>, or a <hop> that takes time. Not when it is in the
		// air already, nor when it would land outside the window. With an
		// arc it is drawn that high above its way halfway there.
		bool startFlight(Object& object, Vector2f whole, float seconds, float arc);
		void land(Object& object, const Object& other, Edge edge);

		// <knock>: see CmdKnock. Into a thing with no mass, through it, to
		// everything with a mass resting against its far side.
		void knock(Object& object, Object& other, const Touch& touch, float bounce);
		bool leap(Object& object, float height);
		void chase(Object& object, const CmdChase& chase);
		bool aim(Object& object, const CmdAim& command);
		void reverse(Object& object);

		// <become>: the object running it (self, which a state's commands do
		// not have), or every object its object= names.
		void become(Object* self, const CmdBecome& command);

		// <follow>: with object=, every object of that name sets off along the
		// path (Game::follow); without, `self` does (nothing when there is none).
		void follow(Object* self, const CmdFollow& command);
		void applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn);
		void applyActionTurn(Object& object, Direction direction, float rate);
		void applyActionThrustAhead(Object& object, const std::string& action, float amount, const std::string& burn);
		void release(const Object& from, const std::string& target, int count);
		void restart(Object& object);

		// Runs an action's commands in order. On the press, a verb that can
		// be refused (<fire> with nothing left to fire, a <hop>, <jump>,
		// <leap> or hunt by hops that cannot be made) and is refused ends
		// the action there: what it would have done after (a sound, a look)
		// is not done.
		// With `from`, only those showing that look run it.
		void triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed, const std::string& from = {});
		// One object's action of that name, as triggerObjectAction runs it
		// for that object or for each member of a group.
		void runAction(Object& object, const std::string& actionName, bool keyPressed);
		// The same from a rule, a timer or a condition rather than a key: a
		// <climb> in it takes the object along one ladder and then lets go.
		void triggerByRule(const CmdTriggerAction& trigger);
		bool byRule{ false };

		// False when every one of the projectiles is in flight.
		bool spawnProjectile(Object& shooter, const std::string& projectileName);
	};
}
