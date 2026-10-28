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
		Command made;
		bool done = true;
		std::visit(overload{
			[&](const CmdBounce&) { bounceScreenEdge(object, edge); },
			[&](const CmdStick&) { xge::stick(object, game.getWindowDesc(), edge); },
			[&](const CmdReset&) { restart(object); },
			[&](const CmdDie&) { xge::die(object); },
			[&](const CmdStop&) { xge::stop(object); },
			[&](const CmdWrap&) { xge::wrap(object, game.getWindowDesc(), edge); },
			[&](const CmdDrop&) { xge::drop(object); },
			[&](const CmdRelease& r) { release(object, r.target, r.count); },
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdCopy& c) { game.copyVariable(c.from, c.into); },
			[&](const CmdRank& c) { game.rank(c.entry, c.table); },
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdSilence& s) { game.silenceSound(s.sound); },
			[&](const CmdReverse&) { reverse(object); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdBecome& b) { become(&object, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(&object, f); },
			[&](const CmdEnterScene& e) { game.enter(e); },
			[&](const CmdResetScene& r) { game.reset(r); },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, true, a.from); },
			[&](const CmdSet& c) { game.setNumber(c.target, c.value); },
			[&](const CmdSpawn& c) { done = spawn(c); },
			[&](const CmdDespawn& d) { despawn(&object, d); },
			[&](const CmdFire& f) { done = spawnProjectile(object, f.projectileName); },
			[&](const auto&) { /* the loader lets only a rule's commands
			                      (load/expr ruleFor) into a collisionData list;
			                      the rest are about another object or a key. */ }
		}, now(command, made));
		return done;
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
		const Edge edge = touch.edge;
		Command made;
		bool done = true;
		std::visit(overload{
			[&](const CmdBounce&) { xge::bounce(object, edge); },
			[&](const CmdDeflect& d) { xge::deflect(object, other, edge, d.maxAngle); },
			[&](const CmdKnock& k) { knock(object, other, touch, k.bounce); },
			[&](const CmdHole& h) { cutHole(object, other, h.radius, h.rough); },
			[&](const CmdDie&) { xge::die(object); },
			[&](const CmdStop&) { xge::stop(object); },
			[&](const CmdReset&) { restart(object); },
			[&](const CmdRelease& r) { release(object, r.target, r.count); },
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdCopy& c) { game.copyVariable(c.from, c.into); },
			[&](const CmdRank& c) { game.rank(c.entry, c.table); },
			[&](const CmdRide&) { xge::ride(object, other); },
			[&](const CmdCarry&) { carry(object, other); },
			[&](const CmdDrop&) { xge::drop(object); },
			[&](const CmdLand&) { land(object, other, edge); },
			[&](const CmdChase& c) { chase(object, c); },
			[&](const CmdAim& a) { done = aim(object, a); },
			[&](const CmdHunt& h) { startHunt(object, h); },
			[&](const CmdHuntHop& h) { done = huntByHop(object, h); },
			[&](const CmdWander& w) { startWander(object, w); },
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdSilence& s) { game.silenceSound(s.sound); },
			[&](const CmdReverse&) { reverse(object); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdBecome& b) { become(&object, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(&object, f); },
			[&](const CmdEnterScene& e) { game.enter(e); },
			[&](const CmdResetScene& r) { game.reset(r); },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, true, a.from); },
			[&](const CmdSet& c) { game.setNumber(c.target, c.value); },
			[&](const CmdSpawn& c) { done = spawn(c); },
			[&](const CmdDespawn& d) { despawn(&object, d); },
			[&](const CmdFire& f) { done = spawnProjectile(object, f.projectileName); },
			[&](const auto&) { /* stick/wrap are about a screen edge, and the rest
			                      only make sense on a state's input or an object's
			                      own action; ignore. */ }
		}, now(command, made));
		return done;
	}

	bool CommandExecutor::executeInput(const Command& command, bool keyPressed)
	{
		Command made;
		bool done = true;
		std::visit(overload{
			[&](const CmdPushState& s) { if (keyPressed) { game.pushState(s.name); } },
			[&](const CmdPopState& s) { if (keyPressed) { game.popState(s.name); } },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, keyPressed, a.from); },
			[&](const CmdResetObject& r) { if (keyPressed) { game.resetObject(r.target); } },
			// Bare reset() means something different here than it does inside a
			// collision (executeScreenEdgeCollision resets just the colliding
			// object's own position): with no "colliding object" to be implicit
			// about, it's the full-game reset - see Game::resetAll.
			[&](const CmdReset&) { if (keyPressed) { game.resetAll(); } },
			// A sound that loops plays for as long as its key is held.
			[&](const CmdPlay& p) { playHeld(p.sound, keyPressed); },
			[&](const CmdSilence& s) { if (keyPressed) { game.silenceSound(s.sound); } },
			// What a <condition> (or a key) can also do to the game's objects:
			// change a variable, a look, or bring something back into play.
			[&](const CmdIncrement& i) { if (keyPressed) { game.incrementText(i.target, i.amount); } },
			[&](const CmdDecrement& d) { if (keyPressed) { game.decrementText(d.target, d.amount); } },
			[&](const CmdCopy& c) { if (keyPressed) { game.copyVariable(c.from, c.into); } },
			[&](const CmdRank& c) { if (keyPressed) { game.rank(c.entry, c.table); } },
			[&](const CmdTurns& t) { if (keyPressed) { game.startTurns(t.players); } },
			[&](const CmdPass&) { if (keyPressed) { game.passTurn(false); } },
			[&](const CmdOut&) { if (keyPressed) { game.passTurn(true); } },
			[&](const CmdDemo&) { if (keyPressed) { game.requestDemo(); } },
			[&](const CmdBecome& b) { if (keyPressed) { become(nullptr, b); } },
			[&](const CmdReveal& r) { if (keyPressed) { game.reveal(r.target, r.count); } },
			[&](const CmdFollow& f) { if (keyPressed) { follow(nullptr, f); } },
			[&](const CmdEnterScene& e) { if (keyPressed) { game.enter(e); } },
			[&](const CmdResetScene& r) { if (keyPressed) { game.reset(r); } },
			[&](const CmdSet& c) { if (keyPressed) { game.setNumber(c.target, c.value); } },
			[&](const CmdSpawn& c) { if (keyPressed) { done = spawn(c); } },
			[&](const CmdDespawn& d) { if (keyPressed) { despawn(nullptr, d); } },
			[&](const auto&) { /* bounce/stick/die/move/inc/fire never appear
			                      directly on a state's <input>; only reachable
			                      via CmdTriggerAction into an object's own
			                      action list. */ }
		}, keyPressed ? now(command, made) : shapeOf(command));
		return done;
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
		Object& object = game.getObject(trigger->object);
		for (const auto& action : object.action[trigger->action])
		{
			const auto* along = std::get_if<CmdTrack>(&shapeOf(action));
			if (!along) { continue; }
			if (along->path.empty())
			{
				track(object, horizontal, fraction, horizontal ? window.width : window.height);
			}
			else if (const auto path = game.getPaths().find(along->path); path != game.getPaths().end())
			{
				trackPath(object, path->second, fraction);
			}
		}
	}

	bool CommandExecutor::executeTimer(const Command& command, Object* owner)
	{
		Command made;
		bool done = true;
		std::visit(overload{
			[&](const CmdPushState& s) { game.pushState(s.name); },
			[&](const CmdPopState& s) { game.popState(s.name); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, true, a.from); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdCopy& c) { game.copyVariable(c.from, c.into); },
			[&](const CmdRank& c) { game.rank(c.entry, c.table); },
			[&](const CmdTurns& t) { if (!owner) { game.startTurns(t.players); } },
			[&](const CmdPass&) { if (!owner) { game.passTurn(false); } },
			[&](const CmdOut&) { if (!owner) { game.passTurn(true); } },
			[&](const CmdDemo&) { if (!owner) { game.requestDemo(); } },
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdSilence& s) { game.silenceSound(s.sound); },
			[&](const CmdBecome& b) { become(owner, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(owner, f); },
			[&](const CmdEnterScene& e) { game.enter(e); },
			[&](const CmdResetScene& r) { game.reset(r); },
			// On an object, a bare reset puts that object back; in a state, it is
			// the whole game, as on a key.
			[&](const CmdReset&) { if (owner) { restart(*owner); } else { game.resetAll(); } },
			[&](const CmdFire& f) { if (owner) { done = spawnProjectile(*owner, f.projectileName); } },
			[&](const CmdReverse&) { if (owner) { reverse(*owner); } },
			[&](const CmdDie&) { if (owner) { xge::die(*owner); } },
			[&](const CmdStop&) { if (owner) { xge::stop(*owner); } },
			[&](const CmdDrop&) { if (owner) { xge::drop(*owner); } },
			[&](const CmdMove& m) { if (owner) { moveByStep(*owner, m.direction, m.step); } },
			[&](const CmdRelease& r) { if (owner) { release(*owner, r.target, r.count); } },
			[&](const CmdChase& c) { if (owner) { chase(*owner, c); } },
			[&](const CmdAim& a) { if (owner) { done = aim(*owner, a); } },
			[&](const CmdHunt& h) { if (owner) { startHunt(*owner, h); } },
			[&](const CmdHuntHop& h) { if (owner) { done = huntByHop(*owner, h); } },
			[&](const CmdWander& w) { if (owner) { startWander(*owner, w); } },
			[&](const CmdSet& c) { game.setNumber(c.target, c.value); },
			[&](const CmdSpawn& c) { done = spawn(c); },
			[&](const CmdDespawn& d) { despawn(owner, d); },
			[&](const auto&) { /* the rest are about a key held, an edge or another
			                      object touched, none of which a timer has */ }
		}, now(command, made));
		return done;
	}

	void CommandExecutor::become(Object* self, const CmdBecome& command)
	{
		if (!command.target.empty())
		{
			game.become(command.target, command.sprite, command.from, command.style);
		}
		else if (self && (command.from.empty() || self->lookName() == command.from))
		{
			if (!command.sprite.empty()) { showLookNamed(*self, command.sprite); }
			if (!command.style.empty()) { wearPaletteNamed(*self, command.style); }
		}
	}

	void CommandExecutor::follow(Object* self, const CmdFollow& command)
	{
		if (!command.target.empty())
		{
			game.follow(command.target, command.path, command.stagger);
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

	bool CommandExecutor::executeCondition(const Command& command)
	{
		return executeInput(command, true);
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
			else if (const auto* thrust = std::get_if<CmdAccelerate>(&actionCommand))
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

	namespace
	{
		bool isNamed(const Object& object, const std::string& name)
		{
			return object.name == name || object.inGroup(name);
		}

	}

	// Puts `count` of the objects called `target` that are out of play back in,
	// each centred on `from` and at its own starting velocity. A pool of them is
	// a <group>: the first ones found that are not in play are the ones used, and
	// when there are fewer than asked for the rest are simply not released.
	void CommandExecutor::release(const Object& from, const std::string& target, int count)
	{
		const Vector2f middle = from.position + sizeOf(from) * 0.5f;
		int released = 0;

		// Of an object that does not exist at the start, new copies.
		if (game.isDescription(target))
		{
			for (; released < count; ++released)
			{
				Object* made = game.spawn(target);
				if (!made) { break; }
				made->position = middle - sizeOf(*made) * 0.5f;
				made->velocity = made->velocityOriginal;
			}
			return;
		}

		for (auto& candidate : game.getCurrentObjects())
		{
			if (released >= count) { break; }
			if (candidate.isVisible || !candidate.exists || &candidate == &from || !isNamed(candidate, target) || game.isAway(candidate)) { continue; }

			// Back in play is a new start: a <random> in it is drawn again,
			// and its timers and animation start over.
			game.drawStartAgain(candidate);
			candidate.startOver();
			candidate.position = middle - sizeOf(candidate) * 0.5f;
			candidate.velocity = candidate.velocityOriginal;
			candidate.riding = {};
			candidate.isVisible = true;
			candidate.collisionData.enabled = true;
			++released;
		}
	}

	void CommandExecutor::bounceScreenEdge(Object& object, Edge edge)
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

		xge::bounce(object, edge);
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

	// A jump asked for by a hop.*() action. It is only queued here:
	// playOneFrame makes it at the start of the frame's move, where it can be
	// refused if it would leave the window. Asking twice in one frame keeps
	// the later one, so a hop is always a single step in one direction.
	bool CommandExecutor::startJump(Object& object, const CmdJump& jump)
	{
		if (!startFlight(object, stepToward(jump.direction, jump.distance), jump.seconds, jump.arc))
		{
			return false;
		}

		object.facing = jump.direction;
		if (jump.direction == Direction::Left || jump.direction == Direction::Right) { object.goingLeft = jump.direction == Direction::Left; }
		object.hasAim = false;
		return true;
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
		object.jumpStep = whole * (1.0f / static_cast<float>(frames));
		object.jumpFramesLeft = frames;
		object.jumpFrames = frames;
		object.jumpArc = arc;
		return true;
	}

	bool CommandExecutor::hop(Object& object, const CmdHop& command)
	{
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

	bool CommandExecutor::huntByHop(Object& object, const CmdHuntHop& hunt)
	{
		if (object.isAirborne() || !game.isShown(object)) { return false; }

		const Object* quarry = game.nearestInPlay(object, hunt.target);
		if (!quarry) { return false; }

		// The four hops it could make, in the order a tie goes: up before
		// down, then left before right.
		const std::array<std::pair<Direction, Vector2f>, 4> hops = hunt.diagonal
			? std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { -hunt.across, -hunt.down } }, { Direction::Up, { hunt.across, -hunt.down } },
				{ Direction::Down, { -hunt.across, hunt.down } }, { Direction::Down, { hunt.across, hunt.down } } } }
			: std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { 0.0f, -hunt.down } }, { Direction::Left, { -hunt.across, 0.0f } },
				{ Direction::Down, { 0.0f, hunt.down } }, { Direction::Right, { hunt.across, 0.0f } } } };

		const WindowDesc& window = game.getWindowDesc();
		const Vector2f there = quarry->position + sizeOf(*quarry) * 0.5f;
		const Vector2f middle = object.position + sizeOf(object) * 0.5f;
		std::optional<std::pair<Direction, Vector2f>> best;
		float bestDistance = 0.0f;
		for (const auto& [way, step] : hops)
		{
			if (!insideWindow({ object.position + step, sizeOf(object) }, { window.width, window.height })) { continue; }
			const Vector2f off = middle + step - there;
			const float distance = off.x * off.x + off.y * off.y;
			if (!best || distance < bestDistance)
			{
				best = std::make_pair(way, step);
				bestDistance = distance;
			}
		}
		if (!best) { return false; }

		return hop(object, CmdHop{ best->first, 0.0f, best->second, hunt.seconds, hunt.arc });
	}

	void CommandExecutor::queueHop(Object& object, Vector2f step)
	{
		object.hopPending = step;
	}

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed, const std::string& from)
	{
		const auto showing = [&](const Object& object) { return from.empty() || object.lookName() == from; };
		Object& first = game.getObject(objectName);
		if (first.name == objectName || !first.inGroup(objectName))
		{
			if (showing(first)) { runAction(first, actionName, keyPressed); }
			return;
		}

		// A group's name: every member, each its own press (by place, since an
		// action may spawn and so grow the list).
		std::vector<Object>& objects = game.getCurrentObjects();
		const std::size_t count = objects.size();
		for (std::size_t i = 0; i < count; ++i)
		{
			if (objects[i].inGroup(objectName) && showing(objects[i])) { runAction(objects[i], actionName, keyPressed); }
		}
	}

	void CommandExecutor::runAction(Object& object, const std::string& actionName, bool keyPressed)
	{

		// An object out of play does not act on a key: a ship that is not
		// there does not fire, and a cannon not yet brought in does nothing.
		// Letting go still goes through, so nothing stays held.
		if (keyPressed && !object.isVisible)
		{
			return;
		}

		for (const auto& written : object.action[actionName])
		{
			Command made;
			bool done = true;
			std::visit(overload{
				[&](const CmdMove& m)
				{
					holdMove(object, m.direction, m.step, keyPressed);
				},
				// A hop belongs to the press alone: letting go of the key does
				// nothing, and it is not among what executeHeldInput resumes.
				[&](const CmdHop& h) { if (keyPressed) { done = hop(object, h); } },
				[&](const CmdHuntHop& h) { if (keyPressed) { done = huntByHop(object, h); } },
				// So does a jump, and one in the air waits for the landing.
				[&](const CmdJump& j) { if (keyPressed) { done = startJump(object, j); } },
				// A <go> is the press alone too: the walker keeps going that way.
				[&](const CmdGo& g) { if (keyPressed) { startGo(object, g); } },
				[&](const CmdLeap& l) { if (keyPressed) { done = leap(object, l.height); } },
				// A climb is held like a move; playOneFrame makes it.
				[&](const CmdClimb& c)
				{
					object.activeClimb[c.direction == Direction::Up ? 0 : 1] = keyPressed ? c.step : 0.0f;
					object.climbClass = c.ladderClass;
				},
				// And a bare reset (the object back where it started), a change of
				// look and a reveal.
				[&](const CmdReset&) { if (keyPressed) { restart(object); } },
				[&](const CmdBecome& b) { if (keyPressed) { become(&object, b); } },
				[&](const CmdReveal& r) { if (keyPressed) { game.reveal(r.target, r.count); } },
				[&](const CmdFollow& f) { if (keyPressed) { follow(&object, f); } },
				// Thrust is held like a move: on while the key is down.
				[&](const CmdAccelerate& a) { applyActionThrust(object, a.direction, keyPressed ? a.amount : 0.0f, a.burn); },
				// So are a turn and a thrust along the heading.
				[&](const CmdTurn& t) { applyActionTurn(object, t.direction, keyPressed ? t.rate : 0.0f); },
				[&](const CmdThrust& t) { applyActionThrustAhead(object, actionName, keyPressed ? t.amount : 0.0f, t.burn); },
				[&](const CmdFire& f) { if (keyPressed) { done = spawnProjectile(object, f.projectileName); } },
				[&](const CmdDrop&) { if (keyPressed) { xge::drop(object); } },
				// A sound belongs to the press, like a hop: letting go is silent.
				// One that loops is held like a move: on while the key is down.
				[&](const CmdPlay& p) { playHeld(p.sound, keyPressed); },
				[&](const CmdSilence& s) { if (keyPressed) { game.silenceSound(s.sound); } },
				// What a rule or a timer does through an action (one death for
				// several rules): change a variable, put things back, move on.
				[&](const CmdIncrement& i) { if (keyPressed) { game.incrementText(i.target, i.amount); } },
				[&](const CmdDecrement& d) { if (keyPressed) { game.decrementText(d.target, d.amount); } },
				[&](const CmdCopy& c) { if (keyPressed) { game.copyVariable(c.from, c.into); } },
				[&](const CmdResetObject& r) { if (keyPressed) { game.resetObject(r.target); } },
				[&](const CmdResetScene& r) { if (keyPressed) { game.reset(r); } },
				[&](const CmdEnterScene& e) { if (keyPressed) { game.enter(e); } },
				[&](const CmdDie&) { if (keyPressed) { xge::die(object); } },
				[&](const CmdSet& c) { if (keyPressed) { game.setNumber(c.target, c.value); } },
				[&](const CmdSpawn& c) { if (keyPressed) { done = spawn(c); } },
				[&](const CmdDespawn& d) { if (keyPressed) { despawn(&object, d); } },
				[&](const CmdRank& c) { if (keyPressed) { game.rank(c.entry, c.table); } },
				[&](const CmdRelease& r) { if (keyPressed) { release(object, r.target, r.count); } },
				[&](const CmdReverse&) { if (keyPressed) { reverse(object); } },
				[&](const CmdStop&) { if (keyPressed) { xge::stop(object); } },
				// Steering, as from a timer: the press sets it going.
				[&](const CmdChase& c) { if (keyPressed) { chase(object, c); } },
				[&](const CmdAim& a) { if (keyPressed) { aim(object, a); } },
				[&](const CmdHunt& h) { if (keyPressed) { startHunt(object, h); } },
				[&](const CmdWander& w) { if (keyPressed) { startWander(object, w); } },
				// Another action, of this object or another, run as part of this
				// one; the loader turns away an action that runs itself again.
				[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, keyPressed, a.from); },
				[&](const auto&) { /* ruleFor in load/expr lists what an action holds */ }
			}, keyPressed ? now(written, made) : shapeOf(written));

			// Refused: what comes after it answers a press that did nothing.
			if (!done) { return; }
		}
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
			if (&thing == &object || &thing == &other || !(thing.mass > 0.0f) || !thing.collisionData.enabled || thing.isAirborne() || !game.isShown(thing))
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
	// ground until the next frame's landing is worked out; a leap ends, and
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

		if (object.leaping)
		{
			object.leaping = false;
			// One with a <grip> below 1 slides on from the speed it landed at.
			if (object.grip >= 1.0f)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
	}

	namespace
	{
		Vector2f middleOf(const Object& object)
		{
			return object.position + sizeOf(object) * 0.5f;
		}
	}

	// Heads for the nearest of `chase.target` in play: straight at its middle
	// at `chase.speed`, or, within `chase.stopWithin` of it, stops. It faces the
	// way it goes, and a heading turns to it. With none in play it carries on
	// as it was.
	void CommandExecutor::chase(Object& object, const CmdChase& command)
	{
		const Object* target = game.nearestInPlay(object, command.target);
		if (!target) { return; }

		if (const auto way = xge::chase(middleOf(object), middleOf(*target), command.speed, command.stopWithin, object.velocity))
		{
			object.facing = directionOf(*way);
			if (object.hasHeading)
			{
				object.heading = headingOf(*way);
				object.showHeading();
			}
		}
	}

	// Turns to the nearest of `target` in play: the next shots go straight at
	// where its middle is now, and the object faces that way (to the nearest
	// of the four, or exactly, if it has a heading). Lined, only along one of
	// the eight ways, and refused (false) when none is near enough.
	bool CommandExecutor::aim(Object& object, const CmdAim& command)
	{
		const Object* other = game.nearestInPlay(object, command.target);
		if (!other) { return !command.lined; }

		auto way = wayTo(middleOf(object), middleOf(*other));
		if (!way) { return !command.lined; }
		if (command.lined)
		{
			way = linedUp(middleOf(*other) - middleOf(object), command.within);
			if (!way) { return false; }
		}

		object.aim = *way;
		object.hasAim = true;
		object.facing = directionOf(object.aim);
		if (object.hasHeading)
		{
			object.heading = headingOf(object.aim);
			object.showHeading();
		}
		return true;
	}

	// Up off the ground at the speed that, under the object's own pull, rises
	// `height` pixels: v * v = 2 * pull * height. Only from the ground and not
	// on a ladder; nothing without a pull down (the loader refuses that).
	bool CommandExecutor::leap(Object& object, float height)
	{
		if (!object.grounded || object.climbing || object.acceleration.y <= 0.0f)
		{
			return false;
		}

		object.velocity.y = -leapSpeed(object.acceleration.y, height);
		object.grounded = false;
		object.leaping = true;
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

	// Launches a projectile that is not already in flight. The name can be a
	// pool of them (a <group>): the first one not in flight is used, and with
	// all of them in flight nothing happens. From a shooter with a heading the
	// shot leaves its nose the way it faces, at the projectile's own speed;
	// from any other it leaves the top centre, at the projectile's own
	// velocity.
	bool CommandExecutor::spawnProjectile(Object& shooter, const std::string& projectileName)
	{
		Object* projectile = nullptr;
		if (game.isDescription(projectileName))
		{
			// A new one, as many as its <most> allows.
			projectile = game.spawn(projectileName);
		}
		else
		{
			for (auto& candidate : game.getCurrentObjects())
			{
				if (isNamed(candidate, projectileName) && candidate.exists && !candidate.collisionData.enabled && !game.isAway(candidate))
				{
					projectile = &candidate;
					break;
				}
			}
			if (projectile)
			{
				// Fired is a new start: a <random> in the projectile's is drawn
				// again, and its timers and animation start over.
				game.drawStartAgain(*projectile);
				projectile->startOver();
			}
		}
		if (!projectile) { return false; }

		const Box from{ shooter.position, sizeOf(shooter) };
		const Vector2f shotSize = sizeOf(*projectile);
		const float speed = std::hypot(projectile->velocityOriginal.x, projectile->velocityOriginal.y);
		std::optional<Shot> shot;
		if (shooter.hasAim)
		{
			// Straight along the aim, from the middle of the shooter, just clear
			// of it, at the projectile's own speed.
			shot = shotAlong(from, shotSize, shooter.aim, speed);
		}
		else if (shooter.hasHeading)
		{
			shot = shotAhead(from, shotSize, shooter.heading, speed);
		}
		else if (shooter.hasFacing)
		{
			// From the middle of the side it faces, just clear of it, moving that
			// way at the projectile's own speed.
			shot = shotFacing(from, shotSize, shooter.facing, speed);
		}
		else
		{
			// From the shooter's top-centre: the projectile's own middle over
			// the shooter's, so a thin shot leaves the middle of the gun.
			shot = Shot{ shotFromTop(from, shotSize), projectile->velocityOriginal };
		}

		projectile->position = shot->position;
		projectile->velocity = shot->velocity;
		projectile->isVisible = true;
		projectile->collisionData.enabled = true;
		return true;
	}

	// A new object, a copy of `spawn.target` (Game::spawn), put and sent
	// where the command says; what it leaves out is as the file says it
	// starts. False when no more of it may exist.
	bool CommandExecutor::spawn(const CmdSpawn& command)
	{
		Object* made = game.spawn(command.target);
		if (!made) { return false; }

		const Vector2f size = sizeOf(*made);
		if (command.x) { made->position.x = command.centerX ? *command.x - size.x * 0.5f : *command.x; }
		if (command.y) { made->position.y = command.centerY ? *command.y - size.y * 0.5f : *command.y; }
		if (command.velocityX) { made->velocity.x = *command.velocityX; }
		if (command.velocityY) { made->velocity.y = *command.velocityY; }
		return true;
	}

	void CommandExecutor::despawn(Object* self, const CmdDespawn& command)
	{
		if (!command.target.empty()) { game.despawn(command.target); }
		else if (self) { game.despawn(*self); }
	}
}
