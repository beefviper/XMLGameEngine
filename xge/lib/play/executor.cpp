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

		const bool horizontal = axis == Axis::MouseX || axis == Axis::Pad1X || axis == Axis::Pad2X;
		const WindowDesc& window = game.getWindowDesc();
		Object* turn = game.takesTurns(trigger->object) ? game.onTurn(trigger->object) : nullptr;
		if (game.takesTurns(trigger->object) && (!turn || !game.answers(*turn))) { return; }
		Object& object = turn ? *turn : game.getObject(trigger->object);
		if (!game.answersPicker(object)) { return; }
		// from=: only while it shows that look.
		if (!trigger->from.empty() && object.lookName() != trigger->from) { return; }
		for (const auto& action : object.kind->action[trigger->action])
		{
			const auto* along = std::get_if<CmdTrack>(&shapeOf(action));
			if (!along) { continue; }
			if (along->path.empty())
			{
				const float span = horizontal ? window.width : window.height;
				if (game.isHand(object)) { game.trackHand(object, horizontal, fraction * span); }
				else if (object.onCells) { game.trackCells(object, horizontal, fraction * span, along->onto); }
				else { track(object, horizontal, fraction, span); }
			}
			else if (const auto path = game.getPaths().find(along->path); path != game.getPaths().end())
			{
				trackPath(object, path->second, fraction);
			}
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

		// <track /> runs on a pointing control (executeAxis), <hold> and
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

		// A looping sound right on a key goes on again, as a move does.
		if (const auto* play = std::get_if<CmdPlay>(&command))
		{
			if (!game.soundLoops(play->sound))
			{
				return false;
			}
			game.requestSound(play->sound);
			return true;
		}

		const auto* trigger = std::get_if<CmdTriggerAction>(&command);
		if (!trigger)
		{
			return false;
		}

		Object& object = game.getObject(trigger->object);
		for (const auto& written : object.kind->action[trigger->action])
		{
			Command madeNow;
			const Command& actionCommand = now(written, madeNow, &object, nullptr);
			if (const auto* move = std::get_if<CmdMove>(&actionCommand))
			{
				holdStep(object, game.driveFor(object), move->direction, move->step);
			}
			else if (const auto* climb = std::get_if<CmdClimb>(&actionCommand))
			{
				Object::Climb& asked = game.parts().give(game.parts().climbs, object.id);
				asked.activeClimb[climb->direction == Direction::Up ? 0 : 1] = climb->step;
				asked.climbClass = climb->ladderClass;
			}
			else if (const auto* thrust = std::get_if<CmdThrustAcross>(&actionCommand))
			{
				applyActionThrust(object, thrust->direction, thrust->amount, thrust->burn);
			}
			else if (const auto* turn = std::get_if<CmdTurn>(&actionCommand))
			{
				applyActionTurn(object, turn->direction, turn->rate);
			}
			else if (const auto* ahead = std::get_if<CmdThrust>(&actionCommand))
			{
				applyActionThrustAhead(object, trigger->action, ahead->amount, ahead->burn);
			}
			else if (const auto* swing = std::get_if<CmdSwing>(&actionCommand))
			{
				Object::Swing& stroke = game.parts().give(game.parts().swings, object.id);
				stroke.seconds = swing->seconds;
				stroke.by = swing->by;
				stroke.held = true;
			}
			else if (const auto* play = std::get_if<CmdPlay>(&actionCommand))
			{
				if (game.soundLoops(play->sound)) { game.requestSound(play->sound); }
			}
			// The way a maze walker is asked to go, while the key asking is
			// still down: Pac-Man held right through READY! sets off right.
			else if (const auto* go = std::get_if<CmdGo>(&actionCommand); go && game.mazeOf(object) && !object.onCells)
			{
				startGo(*game.mazeOf(object), *go);
			}
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

	// Records that `direction` is now being pushed by `amount` a frame (0 =
	// just released), and the variable it burns; playOneFrame
	// applies every direction being held once a frame. Unlike a move's velocity
	// there is nothing to recombine: each direction adds its own amount.
	void CommandExecutor::applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn)
	{
		const auto index = static_cast<std::size_t>(direction);
		Drive& drive = game.driveFor(object);
		drive.thrust[index] = amount;
		drive.thrustBurn[index] = amount == 0.0f ? std::string{} : burn;
	}

	// Records that a turn is now held (rate 0 = just released);
	// playOneFrame turns the object once a frame while it is.
	void CommandExecutor::applyActionTurn(Object& object, Direction direction, float rate)
	{
		game.driveFor(object).turn[static_cast<std::size_t>(direction)] = rate;
	}

	// Records that thrust along the heading is now held (amount 0 = just
	// released) by that action, and the variable it burns. Each action's
	// thrust is its own, so two held at once add up.
	void CommandExecutor::applyActionThrustAhead(Object& object, const std::string& action, float amount, const std::string& burn)
	{
		if (amount == 0.0f)
		{
			if (Drive* drive = game.parts().drives.find(object.id)) { drive->thrustAhead.erase(action); }
			return;
		}
		game.driveFor(object).thrustAhead[action] = { amount, burn };
	}

}
