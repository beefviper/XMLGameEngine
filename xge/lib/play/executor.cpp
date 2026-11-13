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
	const Command& CommandExecutor::now(const Command& command, Command& made)
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			made = game.makeNow(*live);
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
		for (const auto& action : object.action[trigger->action])
		{
			const auto* along = std::get_if<CmdTrack>(&shapeOf(action));
			if (!along) { continue; }
			if (along->path.empty())
			{
				const float span = horizontal ? window.width : window.height;
				if (object.hand.is) { game.trackHand(object, horizontal, fraction * span); }
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
		// go too). Anything else belongs to the press alone.
		const Verb* shape = verbOf(command);
		if (!doer.pressed && !(shape && shape->onRelease)) { return true; }

		// A command whose numbers change as the game runs is made again first.
		Command made;
		const Command& current = doer.pressed ? now(command, made) : shapeOf(command);

		// A condition in an action: its commands while it holds, up to one
		// refused.
		if (const auto* when = std::get_if<CmdWhen>(&current))
		{
			if (!when->body || !when->body->test || !game.testHolds(*when->body->test)) { return true; }
			for (const Command& inner : when->body->commands)
			{
				if (!run(inner, doer)) { break; }
			}
			return true;
		}

		// <track /> runs on a pointing control (executeAxis), <hold> and
		// <think> in a <computer>: they have no run of their own.
		const Verb* verb = verbOf(current);
		return !verb || !verb->run || verb->run(current, doer, *this);
	}

	void CommandExecutor::become(Object* self, const CmdBecome& command)
	{
		if (!command.target.empty())
		{
			game.become(command.target, command.sprite, command.from, command.style);
		}
		else if (self && (command.from.empty() || self->lookName() == command.from))
		{
			const std::string before = self->hand.is ? self->lookName() : std::string();
			if (!command.sprite.empty()) { showLookNamed(*self, command.sprite); }
			if (!command.style.empty()) { wearPaletteNamed(*self, command.style); }
			// A hand sits centred on its card by its look's size.
			if (self->hand.is && self->lookName() != before) { game.layOutCards(); }
		}
	}

	void CommandExecutor::follow(Object* self, const CmdFollow& command)
	{
		if (!command.target.empty())
		{
			game.follow(command.target, command.path, command.stagger, command.nearest, self);
		}
		else if (self)
		{
			game.follow(*self, command.path);
		}
	}

	void CommandExecutor::reverse(Object& object)
	{
		object.velocity = object.velocity * -1.0f;
	}

	// Taken from whatever had it: one thing is carried by one object at a time.
	void CommandExecutor::carry(Object& object, const Object& item)
	{
		if (item.name == object.carrying || item.name == object.dropped)
		{
			return;
		}

		for (auto& other : game.getCurrentObjects())
		{
			if (&other != &object && other.carrying == item.name)
			{
				other.carrying.clear();
			}
		}

		xge::carry(object, item);
	}

	void CommandExecutor::playHeld(const std::string& sound, bool keyPressed)
	{
		if (keyPressed)
		{
			game.requestSound(sound);
		}
		else if (game.soundLoops(sound))
		{
			game.silenceSound(sound);
		}
	}

	bool CommandExecutor::executeHeldInput(const Command& held)
	{
		Command made;
		const Command& command = now(held, made);

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
		for (const auto& written : object.action[trigger->action])
		{
			Command madeNow;
			const Command& actionCommand = now(written, madeNow);
			if (const auto* move = std::get_if<CmdMove>(&actionCommand))
			{
				holdStep(object, move->direction, move->step);
			}
			else if (const auto* climb = std::get_if<CmdClimb>(&actionCommand))
			{
				object.activeClimb[climb->direction == Direction::Up ? 0 : 1] = climb->step;
				object.climbClass = climb->ladderClass;
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
				object.swingFrames = std::max(1, game.framesFor(swing->seconds));
				object.swingBy = swing->by;
				object.swingHeld = true;
			}
			else if (const auto* play = std::get_if<CmdPlay>(&actionCommand))
			{
				if (game.soundLoops(play->sound)) { game.requestSound(play->sound); }
			}
		}

		return true;
	}

	// Back where it started, facing the way it started and going the way it
	// started, with any <random> in that start drawn again (a new serve).
	// While a held key is moving it, that key still decides its velocity.
	void CommandExecutor::restart(Object& object)
	{
		game.drawStartAgain(object);
		xge::restart(object);
	}

	void CommandExecutor::bounceScreenEdge(Object& object, Edge edge, float share)
	{
		// An object in lockstep (e.g. the invader block in spaceinvaders) bounces as a
		// whole block sideways instead of just moving this one object.
		// circleRectangleCollision never had lockstep behaviour, so that path
		// (executeObjectCollision) always goes straight to bounceOffEdge.
		if (object.collisionData.lockstep && (edge == Edge::Left || edge == Edge::Right))
		{
			game.updateLockstepObjects(object);
			return;
		}

		xge::bounceOffEdge(object, edge, share);
	}

	void CommandExecutor::moveByStep(Object& object, Direction direction, float step)
	{
		const auto apply = [&](Object& target)
		{
			switch (direction)
			{
			case Direction::Up:    target.position.y -= step; break;
			case Direction::Down:  target.position.y += step; break;
			case Direction::Left:  target.position.x -= step; break;
			case Direction::Right: target.position.x += step; break;
			}
		};

		if (object.collisionData.lockstep > 0)
		{
			for (auto& obj : game.getCurrentObjects())
			{
				if (obj.collisionData.lockstep == object.collisionData.lockstep)
				{
					apply(obj);
				}
			}
		}
		else
		{
			apply(object);
		}
	}

	bool CommandExecutor::startFlight(Object& object, Vector2f whole, float seconds, float arc)
	{
		if (object.isAirborne())
		{
			return false;
		}

		// A jump that would land off the screen is not made at all (up from the
		// top row stays where it is), rather than going part of the way.
		const WindowDesc& window = game.getWindowDesc();
		const Box landing{ object.position + whole, sizeOf(object) };
		if (!insideWindow(landing, { window.width, window.height }))
		{
			return false;
		}

		const int frames = game.framesFor(seconds);
		object.flightStep = whole * (1.0f / static_cast<float>(frames));
		object.flightFramesLeft = frames;
		object.flightFrames = frames;
		object.flightArc = arc;
		return true;
	}

	bool CommandExecutor::hop(Object& object, const CmdHop& command)
	{
		// On a cell map: whole cells (one, with no distance), refused (and
		// touching) when it cannot go.
		if (object.hand.is) { return game.hopHand(object, command.direction); }
		if (object.onCells && !command.onto.empty()) { return game.hopOnto(object, command.direction, command.onto); }
		if (object.onCells)
		{
			const auto cells = [](float pixels, float distance)
			{
				if (pixels == 0.0f && distance == 0.0f) { return 0; }
				const int count = std::max(1, static_cast<int>(std::lround(std::fabs(distance > 0.0f ? distance : pixels))));
				return pixels < 0.0f ? -count : count;
			};
			Vector2f way = command.step;
			if (way.x == 0.0f && way.y == 0.0f) { way = stepToward(command.direction, 1.0f); }
			const int across = way.x == 0.0f ? 0 : cells(way.x, command.distance);
			const int down = way.y == 0.0f ? 0 : cells(way.y, command.distance);
			if (!game.stepCells(object, across, down)) { return false; }
			object.facing = command.direction;
			return true;
		}

		// One in the air waits for the landing.
		if (object.isAirborne()) { return false; }

		if (command.seconds > 0.0f)
		{
			if (!startFlight(object, command.step, command.seconds, command.arc)) { return false; }
		}
		else
		{
			// playOneFrame refuses it again where the object is then; asked
			// here, so the action knows at once.
			const WindowDesc& window = game.getWindowDesc();
			if (!insideWindow({ object.position + command.step, sizeOf(object) }, { window.width, window.height })) { return false; }
			queueHop(object, command.step);
		}
		object.facing = command.direction;
		if (command.step.x != 0.0f) { object.goingLeft = command.step.x < 0.0f; }
		object.hasAim = false;
		return true;
	}

	void CommandExecutor::queueHop(Object& object, Vector2f step)
	{
		object.hopPending = step;
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
			if (objects[i].inGroup(objectName) && showing(objects[i]) && !objects[i].awayFromHome() && (byRule || game.answers(objects[i]))) { reached.push_back(i); }
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
		if (object.awayFromHome())
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
		for (const auto& written : object.action[actionName])
		{
			// Refused: what comes after it answers a press that did nothing.
			if (!run(written, doer)) { return true; }
		}
		return false;
	}

	void CommandExecutor::knock(Object& object, Object& other, const Touch& touch, float bounce)
	{
		if (other.mass > 0.0f)
		{
			xge::knock(object, other, touch, bounce);
			return;
		}

		// Through a thing that does not move: the far side of it is the side
		// the object's own touched edge faces, and what rests there is
		// knocked as though the object, as heavy as can be, had hit it.
		const Edge farSide = touch.edge;
		const Box base{ other.position, sizeOf(other) };
		const Vector2f way = touch.normal * -1.0f;
		for (Object& thing : game.getCurrentObjects())
		{
			if (&thing == &object || &thing == &other || !(thing.mass > 0.0f) || !thing.collisionsOn || thing.isAirborne() || !game.isShown(thing))
			{
				continue;
			}

			const Box at{ thing.position, sizeOf(thing) };
			if (!restsAgainst(at, base, farSide))
			{
				continue;
			}

			const std::optional<Knocked> after = knockedApart(object.velocity, 0.0f, thing.velocity, thing.mass, way, bounce);
			if (!after)
			{
				continue;
			}
			thing.velocity = after->second;

			Touch felt;
			felt.edge = opposite(farSide);
			felt.normal = way;
			felt.point = { std::clamp(touch.point.x, at.left(), at.right()), std::clamp(touch.point.y, at.top(), at.bottom()) };
			felt.along = 0.5f;
			felt.time = touch.time;
			felt.through = &other.objClass;
			game.reactTo(thing, object, felt);
		}
	}

	// Coming down onto the top of `other` (its own bottom edge touched, not
	// going up): it stands there. Put on the top, its fall stopped, on the
	// ground until the next frame's landing is worked out; a jump ends, and
	// the keys held now decide the way across. Any other touch does nothing,
	// so a platform can be jumped up through and walked past, and nothing
	// lands while it climbs.
	void CommandExecutor::land(Object& object, const Object& other, Edge edge)
	{
		if (!landOn(object.position, object.velocity, sizeOf(object), { other.position, sizeOf(other) }, edge, object.climbing))
		{
			return;
		}

		object.grounded = true;

		if (object.jumping)
		{
			object.jumping = false;
			// One with a <grip> below 1 slides on from the speed it landed at.
			if (object.grip >= 1.0f)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
	}

	// Up off the ground at the speed that, under the object's own pull, rises
	// `height` pixels: v * v = 2 * pull * height. Only from the ground and not
	// on a ladder; nothing without a pull down (the loader refuses that).
	bool CommandExecutor::jump(Object& object, float height)
	{
		if (!object.grounded || object.climbing || object.acceleration.y <= 0.0f)
		{
			return false;
		}

		object.velocity.y = -jumpSpeed(object.acceleration.y, height);
		object.grounded = false;
		object.jumping = true;
		return true;
	}

	// Records that `direction` is now being pushed by `amount` a frame (0 =
	// just released), and the variable it burns; playOneFrame
	// applies every direction being held once a frame. Unlike a move's velocity
	// there is nothing to recombine: each direction adds its own amount.
	void CommandExecutor::applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn)
	{
		const auto index = static_cast<std::size_t>(direction);
		object.activeThrust[index] = amount;
		object.activeThrustBurn[index] = amount == 0.0f ? std::string{} : burn;
	}

	// Records that a turn is now held (rate 0 = just released);
	// playOneFrame turns the object once a frame while it is.
	void CommandExecutor::applyActionTurn(Object& object, Direction direction, float rate)
	{
		object.activeTurn[static_cast<std::size_t>(direction)] = rate;
	}

	// Records that thrust along the heading is now held (amount 0 = just
	// released) by that action, and the variable it burns. Each action's
	// thrust is its own, so two held at once add up.
	void CommandExecutor::applyActionThrustAhead(Object& object, const std::string& action, float amount, const std::string& burn)
	{
		if (amount == 0.0f)
		{
			object.activeThrustAheadBy.erase(action);
			return;
		}
		object.activeThrustAheadBy[action] = { amount, burn };
	}

	// `command.count` new objects, copies of `spawn.target` (Game::spawn),
	// each put and sent where the command says. False only when not one more
	// of it may exist; past its <most> the rest are simply not made.
	bool CommandExecutor::spawn(const CmdSpawn& command, const Object* self)
	{
		bool made = false;
		for (int each = 0; each < command.count && spawnOne(command, self); ++each) { made = true; }
		return made;
	}

	// One of them. A half of the position left out is the spawner's middle
	// (with no spawner, where the file says it starts); on a cell map, the
	// spawner's cell. Velocity and acceleration left out are its own.
	bool CommandExecutor::spawnOne(const CmdSpawn& command, const Object* self)
	{
		Object* made = game.spawn(command.target, command.random);
		if (!made) { return false; }

		// On a cell map: in a cell, a shape's top left one (counting from 1).
		if (made->onCells)
		{
			// Onto another map: its first cell unless told; on its own, its own.
			const bool elsewhere = !command.into.empty() && command.into != made->mapName;
			// Left out: the spawner's own cell, when it is on that map.
			const bool besideSelf = self && self->onCells && self->mapName == (command.into.empty() ? made->mapName : command.into);
			const auto cell = [&](const std::optional<float>& given, int own, float selfCell)
			{
				if (given) { return static_cast<int>(std::lround(*given)); }
				if (besideSelf) { return static_cast<int>(std::lround(selfCell)); }
				return elsewhere ? 1 : own + 1;
			};
			if (command.free)
			{
				if (!game.putInFreeCell(*made, command.into, command.freeBelow)) { return false; }
			}
			else if (!game.putOnCells(*made, cell(command.column, made->mapColumn, self ? self->cellColumn : 0.0f), cell(command.row, made->mapRow, self ? self->cellRow : 0.0f), command.into)) { return false; }
			if (command.velocityX) { made->velocity.x = *command.velocityX; }
			if (command.velocityY) { made->velocity.y = *command.velocityY; }
			if (command.accelerationX) { made->acceleration.x = *command.accelerationX; }
			if (command.accelerationY) { made->acceleration.y = *command.accelerationY; }
			return true;
		}

		const Vector2f size = sizeOf(*made);
		const Vector2f middle = self ? self->position + sizeOf(*self) * 0.5f - size * 0.5f : made->position;
		made->position.x = !command.x ? middle.x : command.centerX ? *command.x - size.x * 0.5f : *command.x;
		made->position.y = !command.y ? middle.y : command.centerY ? *command.y - size.y * 0.5f : *command.y;
		if (command.velocityX) { made->velocity.x = *command.velocityX; }
		if (command.velocityY) { made->velocity.y = *command.velocityY; }
		if (command.accelerationX) { made->acceleration.x = *command.accelerationX; }
		if (command.accelerationY) { made->acceleration.y = *command.accelerationY; }
		return true;
	}

	void CommandExecutor::despawn(Object* self, const CmdDespawn& command)
	{
		if (!command.target.empty()) { game.despawn(command.target); }
		else if (self) { game.despawn(*self); }
	}
}
