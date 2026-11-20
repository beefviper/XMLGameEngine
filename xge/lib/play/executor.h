// executor.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "game/command.h"
#include "engine/keys.h"
#include "game/object.h"
#include "play/geometry.h"
#include "verbs/verb.h"

#include <optional>
#include <string>

namespace xge
{
	class Game;

	// Response: given a Command that a touch (or a state's <input>) says
	// applies, do the thing.
	class CommandExecutor
	{
	public:
		explicit CommandExecutor(Game& gameToRun) noexcept : game(gameToRun) {}

		// A command bound to a key in the current State's <input> list: push or
		// pop a state, or trigger a named action on another object.
		bool executeInput(const Command& command, bool keyPressed);

		// A key that was already down when a state began: applies only the
		// continuous part of its binding (an action's move.* and thrust.*,
		// a sound that loops) and returns true
		// if the command was one of those. One-shot commands (state changes,
		// fire) are left alone, since the key was never pressed in this state.
		bool executeHeldInput(const Command& command);

		// A command bound to a control that points (<input axis="mouse.x">),
		// run when it moves: a <trigger>'s action runs its <track />s,
		// `fraction` (0 to 1) of the way across the window, along the
		// axis's own direction.
		void executeAxis(const Command& command, Axis axis, float fraction);

		// Whether the last <trigger> run reached an action and every action
		// it reached was refused: what its <otherwise> answers.
		bool triggerRefused() const noexcept { return lastTriggerRefused; }

		// --- What the verbs use (verbs/, a file each): the game, and the work
		// that needs it.

		Game& game;

		// While a held key's commands run, the step's length in seconds: an
		// <inc> or <dec> adds its number times
		// it, so a held count goes by the time held. 1 otherwise.
		float heldShare = 1.0f;

		// A command, done as `doer` says by its verb (Verb::run): a rule
		// about another object gives self, other and the touch (self-relative:
		// see play/collision.h); one about the window's edge self and the
		// edge; an object's timer self; a state's timer or condition nothing;
		// a cell map's check what it found. False when the verb is refused
		// (a <spawn> with every copy out, an <aim> not lined up, a hop that
		// cannot be made), which ends the list it is in.
		bool run(const Command& command, const Doer& doer);


		// The command as it runs: itself, or one made as it runs (CmdLive)
		// made again into `made` (see Game::makeNow).
		const Command& now(const Command& command, Command& made, const Object* self, const Object* other);


		// Runs an action's commands in order. On the press, a verb that can
		// be refused (<fire> with nothing left to fire, a <hop>, <jump>
		// or hunt by hops that cannot be made, an <aim> not lined up)
		// and is refused ends the action there: what it would have done
		// after (a sound, a look) is not done.
		// With `from`, only those showing that look run it. `byRule`: run by
		// a rule, a timer or a condition rather than a key, so a <climb> in
		// it takes the object along one ladder and then lets go.
		// Sets lastTriggerRefused.
		void triggerObjectAction(Name objectName, Name actionName, bool keyPressed, bool byRule, Name from = {});
		// One object's action of that name, as triggerObjectAction runs it
		// for that object or for each member of a group. Whether it ran and
		// a verb in it was refused.
		bool runAction(Object& object, Name actionName, bool keyPressed, bool byRule);
		// A <trigger> with <here /> or <around />: the action of each piece of
		// its class in those cells about `self`, each piece once however the
		// actions it runs trigger more (aboutReached, aboutDepth).
		void triggerAbout(Object& self, const CmdTriggerAction& trigger, bool byRule);


		// A <through /> among the commands run: the refused cell step they
		// answer is made anyway (Game::Reaction).
		bool throughAsked{ false };

	private:
		bool lastTriggerRefused{ false };
		std::vector<std::string> aboutReached;
		int aboutDepth = 0;
	};
}
