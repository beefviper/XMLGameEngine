// executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "play/executor.h"

#include "game/game.h"
#include "play/geometry.h"
#include "play/maze.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace xge
{
	const Command& CommandExecutor::now(const Command& command, Command& made, const Object* self, const Object* other)
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			made = game.makeNow(*live, self, other);
			return made;
		}
		return command;
	}

	bool CommandExecutor::executeScreenEdgeCollision(const Command& command, Object& object, Edge edge)
	{
		Doer doer;
		doer.self = &object;
		doer.screenEdge = edge;
		return run(command, doer);
	}

	bool CommandExecutor::executeObjectCollision(const Command& command, Object& object, Object& other, Edge edge)
	{
		Touch touch;
		touch.edge = edge;
		touch.normal = outOf(opposite(edge));
		touch.along = 0.5f;
		return executeObjectCollision(command, object, other, touch);
	}

	bool CommandExecutor::executeObjectCollision(const Command& command, Object& object, Object& other, const Touch& touch)
	{
		Doer doer;
		doer.self = &object;
		doer.other = &other;
		doer.touch = &touch;
		return run(command, doer);
	}

	bool CommandExecutor::executeInput(const Command& command, bool keyPressed)
	{
		Doer doer;
		doer.pressed = keyPressed;
		doer.byRule = false;
		return run(command, doer);
	}

	bool CommandExecutor::executeCondition(const Command& command)
	{
		return run(command, Doer{});
	}

	bool CommandExecutor::executeCheck(const Command& command, const Object::CellMatch& match)
	{
		Doer doer;
		doer.match = &match;
		return run(command, doer);
	}

	void CommandExecutor::executeAxis(const Command& command, Axis axis, float fraction)
	{
		const auto* trigger = std::get_if<CmdTriggerAction>(&shapeOf(command));
		if (!trigger)
		{
			return;
		}

		const bool across = axis == Axis::MouseX || axis == Axis::Pad1X || axis == Axis::Pad2X;
		Object* turn = game.takesTurns(trigger->object) ? game.onTurn(trigger->object) : nullptr;
		if (game.takesTurns(trigger->object) && (!turn || !game.answers(*turn))) { return; }
		Object& object = turn ? *turn : game.getObject(trigger->object);
		if (!game.answersPicker(object)) { return; }
		// from=: only while it shows that look.
		if (!trigger->from.empty() && object.lookName() != trigger->from) { return; }
		Doer doer;
		doer.self = &object;
		doer.executor = this;
		for (const auto& action : object.kind->action[trigger->action])
		{
			const Command& shape = shapeOf(action);
			if (const Verb* verb = verbOf(shape); verb && verb->point) { verb->point(shape, doer, across, fraction); }
		}
	}

	bool CommandExecutor::executeTimer(const Command& command, Object* owner)
	{
		Doer doer;
		doer.self = owner;
		return run(command, doer);
	}

	bool CommandExecutor::run(const Command& command, const Doer& doer)
	{
		// A let-go key: only a held verb still acts (a move, a climb, a
		// thrust, a turn, a sound that loops stops; an action it runs is let
		// go too, and so is what a condition in it holds, while the condition
		// does). Anything else belongs to the press alone.
		const Verb* shape = verbOf(command);
		const bool condition = std::holds_alternative<CmdWhen>(shapeOf(command));
		if (!doer.pressed && !condition && !(shape && shape->onRelease)) { return true; }

		// A command whose numbers change as the game runs is made again first.
		Command made;
		const Command& current = doer.pressed ? now(command, made, doer.self, doer.other) : shapeOf(command);

		// A condition in an action: its commands while it holds, up to one
		// refused.
		if (const auto* when = std::get_if<CmdWhen>(&current))
		{
			if (!when->body || !when->body->test || !game.testHolds(*when->body->test, doer.self, doer.other)) { return true; }
			for (const Command& inner : when->body->commands)
			{
				if (!run(inner, doer)) { break; }
			}
			return true;
		}

		// <track /> answers a pointing control (Verb::point), <hold> and
		// <think> in a <computer>: they have no run of their own.
		const Verb* verb = verbOf(current);
		if (!verb || !verb->run) { return true; }
		Doer here = doer;
		here.executor = this;
		return verb->run(current, here);
	}

	bool CommandExecutor::executeHeldInput(const Command& held)
	{
		Command made;
		const Command& command = now(held, made, nullptr, nullptr);
		Doer doer;
		doer.executor = this;

		// Right on a key (a looping sound goes on again, as a move does), or
		// each verb of the action the key triggers.
		const auto* trigger = std::get_if<CmdTriggerAction>(&command);
		if (!trigger)
		{
			const Verb* verb = verbOf(command);
			return verb && verb->hold && verb->hold(command, doer);
		}

		Object& object = game.getObject(trigger->object);
		doer.self = &object;
		doer.action = &trigger->action;
		for (const auto& written : object.kind->action[trigger->action])
		{
			Command madeNow;
			const Command& actionCommand = now(written, madeNow, &object, nullptr);
			if (const Verb* verb = verbOf(actionCommand); verb && verb->hold) { verb->hold(actionCommand, doer); }
		}
		return true;
	}

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed, bool byRule, const std::string& from)
	{
		const auto showing = [&](const Object& object) { return from.empty() || object.lookName() == from; };
		lastTriggerRefused = false;
		Object& first = game.getObject(objectName);
		if (first.name == objectName || !first.inGroup(objectName))
		{
			if (showing(first) && (byRule || game.answersPicker(first))) { lastTriggerRefused = runAction(first, actionName, keyPressed, byRule); }
			return;
		}

		// A group's name: every member, each its own press (by place, since an
		// action may spawn and so grow the list). Those it reaches are found
		// first: a piece one of them sends onto its map (the next shape of
		// Tetris) does not answer the same press.
		std::vector<Object>& objects = game.getCurrentObjects();
		std::vector<std::size_t> reached;
		// Players taking turns: a key reaches the one on its turn, and only
		// as the one who plays it presses (the computer, or a person).
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i].inGroup(objectName) && showing(objects[i]) && !game.awayFromHome(objects[i]) && (byRule || game.answers(objects[i]))) { reached.push_back(i); }
		}
		bool refused = !reached.empty();
		for (const std::size_t i : reached) { refused = runAction(objects[i], actionName, keyPressed, byRule) && refused; }
		lastTriggerRefused = refused;
	}

	void CommandExecutor::triggerAbout(Object& self, const CmdTriggerAction& trigger, bool byRule)
	{
		// The outermost one starts afresh; those it leads to share its list
		// of pieces reached, so each runs once and a flood ends.
		if (aboutDepth == 0) { aboutReached.clear(); }
		++aboutDepth;
		lastTriggerRefused = false;
		for (Object* piece : game.piecesAbout(self, trigger.objClass, trigger.place))
		{
			if (!trigger.from.empty() && piece->lookName() != trigger.from) { continue; }
			if (std::find(aboutReached.begin(), aboutReached.end(), piece->name) != aboutReached.end()) { continue; }
			aboutReached.push_back(piece->name);
			runAction(*piece, trigger.action, true, byRule);
		}
		--aboutDepth;
	}

	bool CommandExecutor::runAction(Object& object, const std::string& actionName, bool keyPressed, bool byRule)
	{
		// An object out of play does not act on a key: a ship that is not
		// there does not fire, and a cannon not yet brought in does nothing.
		// Letting go still goes through, so nothing stays held.
		if (keyPressed && !object.isVisible)
		{
			return false;
		}
		// A piece sent away from its own map (a next-piece box) is only shown.
		if (game.awayFromHome(object))
		{
			return false;
		}
		// One in a scene that is not current is not there: a key reaches
		// the worker of the room being played, not every room's.
		if (game.isAway(object))
		{
			return false;
		}

		Doer doer;
		doer.self = &object;
		doer.pressed = keyPressed;
		doer.byRule = byRule;
		doer.action = &actionName;
		for (const auto& written : object.kind->action[actionName])
		{
			// Refused: what comes after it answers a press that did nothing.
			if (!run(written, doer)) { return true; }
		}
		return false;
	}
}
