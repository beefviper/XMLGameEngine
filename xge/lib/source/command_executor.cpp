// command_executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "command_executor.h"

#include "game.h"
#include "geometry.h"
#include "maze.h"
#include "verbs.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace xge
{
	void CommandExecutor::executeScreenEdgeCollision(const Command& command, Object& object, Edge edge)
	{
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
			[&](const auto&) { /* CmdPushState/CmdPopState/CmdFire/CmdTriggerAction never
			                      appear in a collisionData list, and ride() and
			                      deflect() are about another object, which a screen
			                      edge is not; ignore defensively. */ }
		}, command);
	}

	void CommandExecutor::executeObjectCollision(const Command& command, Object& object, const Object& other, Edge edge)
	{
		std::visit(overload{
			[&](const CmdBounce&) { xge::bounce(object, edge); },
			[&](const CmdDeflect& d) { xge::deflect(object, other, edge, d.maxAngle); },
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
			[&](const CmdAim& a) { aim(object, a.target); },
			[&](const CmdHunt& h) { startHunt(object, h); },
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
			[&](const auto&) { /* stick/wrap are about a screen edge, and the rest
			                      only make sense on a state's input or an object's
			                      own action; ignore. */ }
		}, command);
	}

	void CommandExecutor::executeInput(const Command& command, bool keyPressed)
	{
		std::visit(overload{
			[&](const CmdPushState& s) { if (keyPressed) { game.pushState(s.name); } },
			[&](const CmdPopState& s) { if (keyPressed) { game.popState(s.name); } },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, keyPressed); },
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
			[&](const CmdBecome& b) { if (keyPressed) { become(nullptr, b); } },
			[&](const CmdReveal& r) { if (keyPressed) { game.reveal(r.target, r.count); } },
			[&](const CmdFollow& f) { if (keyPressed) { follow(nullptr, f); } },
			[&](const CmdEnterScene& e) { if (keyPressed) { game.enter(e); } },
			[&](const CmdResetScene& r) { if (keyPressed) { game.reset(r); } },
			[&](const auto&) { /* bounce/stick/die/move/inc/fire never appear
			                      directly on a state's <input>; only reachable
			                      via CmdTriggerAction into an object's own
			                      action list. */ }
		}, command);
	}

	void CommandExecutor::executeAxis(const Command& command, Axis axis, float fraction)
	{
		const auto* trigger = std::get_if<CmdTriggerAction>(&command);
		if (!trigger)
		{
			return;
		}

		const bool horizontal = axis == Axis::MouseX || axis == Axis::Pad1X || axis == Axis::Pad2X;
		const WindowDesc& window = game.getWindowDesc();
		Object& object = game.getObject(trigger->object);
		for (const auto& action : object.action[trigger->action])
		{
			if (std::holds_alternative<CmdTrack>(action))
			{
				track(object, horizontal, fraction, horizontal ? window.width : window.height);
			}
		}
	}

	void CommandExecutor::executeTimer(const Command& command, Object* owner)
	{
		std::visit(overload{
			[&](const CmdPushState& s) { game.pushState(s.name); },
			[&](const CmdPopState& s) { game.popState(s.name); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, true); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdCopy& c) { game.copyVariable(c.from, c.into); },
			[&](const CmdRank& c) { game.rank(c.entry, c.table); },
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
			[&](const CmdFire& f) { if (owner) { spawnProjectile(*owner, f.projectileName); } },
			[&](const CmdReverse&) { if (owner) { reverse(*owner); } },
			[&](const CmdDie&) { if (owner) { xge::die(*owner); } },
			[&](const CmdStop&) { if (owner) { xge::stop(*owner); } },
			[&](const CmdDrop&) { if (owner) { xge::drop(*owner); } },
			[&](const CmdMove& m) { if (owner) { moveByStep(*owner, m.direction, m.step); } },
			[&](const CmdRelease& r) { if (owner) { release(*owner, r.target, r.count); } },
			[&](const CmdChase& c) { if (owner) { chase(*owner, c); } },
			[&](const CmdAim& a) { if (owner) { aim(*owner, a.target); } },
			[&](const CmdHunt& h) { if (owner) { startHunt(*owner, h); } },
			[&](const CmdWander& w) { if (owner) { startWander(*owner, w); } },
			[&](const auto&) { /* the rest are about a key held, an edge or another
			                      object touched, none of which a timer has */ }
		}, command);
	}

	void CommandExecutor::become(Object* self, const CmdBecome& command)
	{
		if (!command.target.empty())
		{
			game.become(command.target, command.sprite, command.from);
		}
		else if (self)
		{
			showLookNamed(*self, command.sprite);
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

	void CommandExecutor::executeCondition(const Command& command)
	{
		executeInput(command, true);
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

	bool CommandExecutor::executeHeldInput(const Command& command)
	{
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
		for (const auto& actionCommand : object.action[trigger->action])
		{
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

		for (auto& candidate : game.getCurrentObjects())
		{
			if (released >= count) { break; }
			if (candidate.isVisible || &candidate == &from || !isNamed(candidate, target) || game.isAway(candidate)) { continue; }

			// Back in play is a new start: a <random> in it is drawn again.
			game.drawStartAgain(candidate);
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
	void CommandExecutor::startJump(Object& object, const CmdJump& jump)
	{
		if (object.isAirborne())
		{
			return;
		}

		// A jump that would land off the screen is not made at all (up from the
		// top row stays where it is), rather than going part of the way.
		const WindowDesc& window = game.getWindowDesc();
		const Box landing{ object.position + stepToward(jump.direction, jump.distance), sizeOf(object) };
		if (!insideWindow(landing, { window.width, window.height }))
		{
			return;
		}

		const int frames = game.framesFor(jump.seconds);
		object.jumpStep = stepToward(jump.direction, jump.distance / static_cast<float>(frames));

		object.jumpFramesLeft = frames;
		object.facing = jump.direction;
		object.hasAim = false;
	}

	void CommandExecutor::queueHop(Object& object, Direction direction, float distance)
	{
		object.hopPending = stepToward(direction, distance);
	}

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed)
	{
		Object& object = game.getObject(objectName);

		for (const auto& command : object.action[actionName])
		{
			std::visit(overload{
				[&](const CmdMove& m)
				{
					holdMove(object, m.direction, m.step, keyPressed);
				},
				// A hop belongs to the press alone: letting go of the key does
				// nothing, and it is not among what executeHeldInput resumes.
				[&](const CmdHop& h) { if (keyPressed) { queueHop(object, h.direction, h.distance); object.facing = h.direction; object.hasAim = false; } },
				// So does a jump, and one in the air waits for the landing.
				[&](const CmdJump& j) { if (keyPressed) { startJump(object, j); } },
				// A <go> is the press alone too: the walker keeps going that way.
				[&](const CmdGo& g) { if (keyPressed) { startGo(object, g); } },
				[&](const CmdLeap& l) { if (keyPressed) { leap(object, l.height); } },
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
				[&](const CmdFire& f) { if (keyPressed) { spawnProjectile(object, f.projectileName); } },
				[&](const CmdDrop&) { if (keyPressed) { xge::drop(object); } },
				// A sound belongs to the press, like a hop: letting go is silent.
				// One that loops is held like a move: on while the key is down.
				[&](const CmdPlay& p) { playHeld(p.sound, keyPressed); },
				[&](const CmdSilence& s) { if (keyPressed) { game.silenceSound(s.sound); } },
				[&](const auto&) { /* an object's own <action> list only ever produces
				                      move/hop/accelerate/turn/thrust/fire/drop/play/silence commands today; ignore anything else. */ }
			}, command);
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
			object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
				- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
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
	// way it goes. With none in play it carries on as it was.
	void CommandExecutor::chase(Object& object, const CmdChase& command)
	{
		const Object* target = game.nearestInPlay(object, command.target);
		if (!target) { return; }

		if (const auto way = xge::chase(middleOf(object), middleOf(*target), command.speed, command.stopWithin, object.velocity))
		{
			object.facing = directionOf(*way);
		}
	}

	// Turns to the nearest of `target` in play: the next shots go straight at
	// where its middle is now, and the object faces that way (to the nearest
	// of the four, or exactly, if it has a heading).
	void CommandExecutor::aim(Object& object, const std::string& target)
	{
		const Object* other = game.nearestInPlay(object, target);
		if (!other) { return; }

		const auto way = wayTo(middleOf(object), middleOf(*other));
		if (!way) { return; }

		object.aim = *way;
		object.hasAim = true;
		object.facing = directionOf(object.aim);
		if (object.hasHeading)
		{
			object.heading = headingOf(object.aim);
			object.showHeading();
		}
	}

	// Up off the ground at the speed that, under the object's own pull, rises
	// `height` pixels: v * v = 2 * pull * height. Only from the ground and not
	// on a ladder; nothing without a pull down (the loader refuses that).
	void CommandExecutor::leap(Object& object, float height)
	{
		if (!object.grounded || object.climbing || object.acceleration.y <= 0.0f)
		{
			return;
		}

		object.velocity.y = -leapSpeed(object.acceleration.y, height);
		object.grounded = false;
		object.leaping = true;
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
	void CommandExecutor::spawnProjectile(Object& shooter, const std::string& projectileName)
	{
		Object* projectile = nullptr;
		for (auto& candidate : game.getCurrentObjects())
		{
			if (isNamed(candidate, projectileName) && !candidate.collisionData.enabled && !game.isAway(candidate))
			{
				projectile = &candidate;
				break;
			}
		}
		if (!projectile) { return; }

		// Fired is a new start: a <random> in the projectile's is drawn again.
		game.drawStartAgain(*projectile);

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
	}
}
