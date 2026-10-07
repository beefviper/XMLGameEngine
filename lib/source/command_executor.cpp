// command_executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "command_executor.h"

#include "game.h"

#include <algorithm>
#include <cmath>

namespace xge
{
	void CommandExecutor::executeScreenEdgeCollision(const Command& command, Object& object, Edge edge)
	{
		std::visit(overload{
			[&](const CmdBounce&) { bounceScreenEdge(object, edge); },
			[&](const CmdStick&) { stick(object, edge); },
			[&](const CmdReset&) { restart(object); },
			[&](const CmdDie&) { die(object); },
			[&](const CmdStop&) { stop(object); },
			[&](const CmdWrap&) { wrap(object, edge); },
			[&](const CmdRelease& r) { release(object, r.target, r.count); },
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdReverse&) { reverse(object); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdBecome& b) { become(&object, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(&object, f); },
			[&](const auto&) { /* CmdPushState/CmdPopState/CmdFire/CmdTriggerAction never
			                      appear in a collisionData list, and carry() and
			                      deflect() are about another object, which a screen
			                      edge is not; ignore defensively. */ }
		}, command);
	}

	void CommandExecutor::executeObjectCollision(const Command& command, Object& object, const Object& other, Edge edge)
	{
		std::visit(overload{
			[&](const CmdBounce&) { bounceOffEdge(object, edge); },
			[&](const CmdDeflect& d) { deflect(object, other, edge, d.maxAngle); },
			[&](const CmdDie&) { die(object); },
			[&](const CmdStop&) { stop(object); },
			[&](const CmdReset&) { restart(object); },
			[&](const CmdRelease& r) { release(object, r.target, r.count); },
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.decrementText(d.target, d.amount); },
			[&](const CmdCarry&) { carry(object, other); },
			[&](const CmdLand&) { land(object, other, edge); },
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdReverse&) { reverse(object); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdBecome& b) { become(&object, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(&object, f); },
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
			[&](const CmdPlay& p) { if (keyPressed) { game.requestSound(p.sound); } },
			// What a <condition> (or a key) can also do to the game's objects:
			// change a variable, a look, or bring something back into play.
			[&](const CmdIncrement& i) { if (keyPressed) { game.incrementText(i.target, i.amount); } },
			[&](const CmdDecrement& d) { if (keyPressed) { game.decrementText(d.target, d.amount); } },
			[&](const CmdBecome& b) { if (keyPressed) { become(nullptr, b); } },
			[&](const CmdReveal& r) { if (keyPressed) { game.reveal(r.target, r.count); } },
			[&](const CmdFollow& f) { if (keyPressed) { follow(nullptr, f); } },
			[&](const auto&) { /* bounce/stick/die/move/inc/fire never appear
			                      directly on a state's <input>; only reachable
			                      via CmdTriggerAction into an object's own
			                      action list. */ }
		}, command);
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
			[&](const CmdPlay& p) { game.requestSound(p.sound); },
			[&](const CmdBecome& b) { become(owner, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdFollow& f) { follow(owner, f); },
			// On an object, a bare reset puts that object back; in a state, it is
			// the whole game, as on a key.
			[&](const CmdReset&) { if (owner) { restart(*owner); } else { game.resetAll(); } },
			[&](const CmdFire& f) { if (owner) { spawnProjectile(*owner, f.projectileName); } },
			[&](const CmdReverse&) { if (owner) { reverse(*owner); } },
			[&](const CmdDie&) { if (owner) { die(*owner); } },
			[&](const CmdStop&) { if (owner) { stop(*owner); } },
			[&](const CmdMove& m) { if (owner) { moveByStep(*owner, m.direction, m.step); } },
			[&](const CmdRelease& r) { if (owner) { release(*owner, r.target, r.count); } },
			[&](const auto&) { /* the rest are about a key held, an edge or another
			                      object touched, none of which a timer has */ }
		}, command);
	}

	void CommandExecutor::become(Object* self, const CmdBecome& command)
	{
		if (!command.target.empty())
		{
			game.become(command.target, command.sprite);
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

	bool CommandExecutor::executeHeldInput(const Command& command)
	{
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
				applyActionVelocity(object, move->direction, move->step);
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
				applyActionThrustAhead(object, ahead->amount, ahead->burn);
			}
		}

		return true;
	}

	// Out of play: it stops being drawn, moved and collided with. Everything
	// else about it is left as it was, so whatever brings it back (fire()
	// re-launching a bullet, reset()) starts from a known place.
	void CommandExecutor::die(Object& object)
	{
		object.collisionData.enabled = false;
		object.isVisible = false;
		object.followPath.clear();
	}

	// Comes to rest: no velocity, and nothing left to change it - neither the
	// object's own pull nor a thruster still held. (A reset gives them back.)
	// Being in motion is what makes collisions run, so an object that has
	// stopped is no longer asked about what it is resting on.
	void CommandExecutor::stop(Object& object)
	{
		object.velocity = {};
		object.acceleration = {};
		object.activeMoveStep = {};
		object.activeClimb = {};
		object.climbing = false;
		object.leaping = false;
		object.activeThrust = {};
		object.activeThrustBurn = {};
		object.activeTurn = {};
		object.activeThrustAhead = 0.0f;
		object.activeThrustAheadBurn.clear();
		object.followPath.clear();
	}

	// Back where it started, facing the way it started and going the way it
	// started, with any <random> in that start drawn again (a new serve).
	// While a held key is moving it, that key still decides its velocity.
	void CommandExecutor::restart(Object& object)
	{
		game.drawStartAgain(object);
		object.position = object.positionOriginal;
		object.climbing = false;
		object.leaping = false;
		object.grounded = false;
		const bool keyHeld = std::any_of(object.activeMoveStep.begin(), object.activeMoveStep.end(), [](float step) { return step != 0.0f; });
		if (!keyHeld)
		{
			object.velocity = object.velocityOriginal;
		}
		object.heading = object.headingOriginal;
		object.showHeading();
	}

	namespace
	{
		bool isNamed(const Object& object, const std::string& name)
		{
			return object.name == name || object.baseName == name || (!object.groupName.empty() && object.groupName == name);
		}

		// What an object measures: what the window measured, or, before there
		// is one, what its sprite implies.
		Vector2f sizeOf(const Object& object)
		{
			if (object.size.x != 0.0f || object.size.y != 0.0f) { return object.size; }
			return measureShapeSize(object.spriteParams, object.shapeKind);
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
			if (candidate.isVisible || &candidate == &from || !isNamed(candidate, target)) { continue; }

			candidate.position = middle - sizeOf(candidate) * 0.5f;
			candidate.velocity = candidate.velocityOriginal;
			candidate.carry = {};
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

		bounceOffEdge(object, edge);
	}

	void CommandExecutor::bounceOffEdge(Object& object, Edge edge)
	{
		switch (edge)
		{
		case Edge::Left:   object.velocity.x = std::abs(object.velocity.x); break;
		case Edge::Right:  object.velocity.x = -std::abs(object.velocity.x); break;
		case Edge::Top:    object.velocity.y = std::abs(object.velocity.y); break;
		case Edge::Bottom: object.velocity.y = -std::abs(object.velocity.y); break;
		}
	}

	// A bounce off `other` that leaves at an angle set by where it hit. How far
	// the object's middle is from the middle of the touched side, as a share of
	// half that side's length (-1 at one end, 1 at the other, held there past
	// the ends), times maxAngle, is the angle away from straight back out,
	// towards the end it hit nearer to: off a Pong paddle's top half the ball
	// goes back up, off its bottom half down. The speed is what it was.
	void CommandExecutor::deflect(Object& object, const Object& other, Edge edge, float maxAngle)
	{
		const float speed = std::hypot(object.velocity.x, object.velocity.y);

		const Vector2f middle = object.position + sizeOf(object) * 0.5f;
		const Vector2f otherSize = sizeOf(other);
		const Vector2f otherMiddle = other.position + otherSize * 0.5f;

		const bool side = (edge == Edge::Left || edge == Edge::Right);
		const float offset = side ? middle.y - otherMiddle.y : middle.x - otherMiddle.x;
		const float half = (side ? otherSize.y : otherSize.x) * 0.5f;
		const float along = (half > 0.0f) ? std::clamp(offset / half, -1.0f, 1.0f) : 0.0f;

		const float radians = maxAngle * along * 3.14159265358979323846f / 180.0f;
		const float away = speed * std::cos(radians);
		const float across = speed * std::sin(radians);

		switch (edge)
		{
		case Edge::Left:   object.velocity = { away, across }; break;
		case Edge::Right:  object.velocity = { -away, across }; break;
		case Edge::Top:    object.velocity = { across, away }; break;
		case Edge::Bottom: object.velocity = { across, -away }; break;
		}
	}

	void CommandExecutor::stick(Object& object, Edge edge)
	{
		const auto& windowDesc = game.getWindowDesc();

		// Only the touched edge's own axis is affected: the object is pushed
		// back inside on that axis and loses only the velocity that was
		// carrying it into the wall. The other axis is left alone, so an
		// object pressed against the bottom wall still slides left or right
		// (and one moving away from the wall is not held back either).
		switch (edge)
		{
		case Edge::Left:
			object.position.x = std::max(object.position.x, 0.0f);
			object.velocity.x = std::max(object.velocity.x, 0.0f);
			break;
		case Edge::Right:
			object.position.x = std::min(object.position.x, windowDesc.width - object.size.x);
			object.velocity.x = std::min(object.velocity.x, 0.0f);
			break;
		case Edge::Top:
			object.position.y = std::max(object.position.y, 0.0f);
			object.velocity.y = std::max(object.velocity.y, 0.0f);
			break;
		case Edge::Bottom:
			object.position.y = std::min(object.position.y, windowDesc.height - object.size.y);
			object.velocity.y = std::min(object.velocity.y, 0.0f);
			break;
		}
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

	// A jump asked for by a hop.*() action. It is only queued here: Game::
	// moveObjects makes it at the start of the frame's move, where it can be
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
		Vector2f landing = object.position;
		switch (jump.direction)
		{
		case Direction::Up:    landing.y -= jump.distance; break;
		case Direction::Down:  landing.y += jump.distance; break;
		case Direction::Left:  landing.x -= jump.distance; break;
		case Direction::Right: landing.x += jump.distance; break;
		}
		const WindowDesc& window = game.getWindowDesc();
		const Vector2f size = sizeOf(object);
		if (landing.x < 0 || landing.y < 0 || landing.x + size.x > window.width || landing.y + size.y > window.height)
		{
			return;
		}

		const int frames = game.framesFor(jump.seconds);
		const float step = jump.distance / static_cast<float>(frames);

		switch (jump.direction)
		{
		case Direction::Up:    object.jumpStep = { 0.0f, -step }; break;
		case Direction::Down:  object.jumpStep = { 0.0f, step }; break;
		case Direction::Left:  object.jumpStep = { -step, 0.0f }; break;
		case Direction::Right: object.jumpStep = { step, 0.0f }; break;
		}

		object.jumpFramesLeft = frames;
		object.facing = jump.direction;
	}

	void CommandExecutor::queueHop(Object& object, Direction direction, float distance)
	{
		switch (direction)
		{
		case Direction::Up:    object.hopPending = { 0.0f, -distance }; break;
		case Direction::Down:  object.hopPending = { 0.0f, distance }; break;
		case Direction::Left:  object.hopPending = { -distance, 0.0f }; break;
		case Direction::Right: object.hopPending = { distance, 0.0f }; break;
		}
	}

	// Once an object has gone right off the screen through `edge` - and is
	// still heading that way - it comes back in from the opposite side, one
	// screen and one object's width along, so anything spaced along a lane
	// keeps its spacing. Until it has gone completely, nothing happens: the
	// rule is asked every frame the object is touching the edge, and an
	// object that is still partly in view slides out as normal.
	void CommandExecutor::wrap(Object& object, Edge edge)
	{
		const auto& windowDesc = game.getWindowDesc();
		const Vector2f heading = object.velocity + object.carry;

		switch (edge)
		{
		case Edge::Right:
			if (heading.x > 0.0f && object.position.x >= windowDesc.width)
			{
				object.position.x -= windowDesc.width + object.size.x;
			}
			break;
		case Edge::Left:
			if (heading.x < 0.0f && object.position.x + object.size.x <= 0.0f)
			{
				object.position.x += windowDesc.width + object.size.x;
			}
			break;
		case Edge::Bottom:
			if (heading.y > 0.0f && object.position.y >= windowDesc.height)
			{
				object.position.y -= windowDesc.height + object.size.y;
			}
			break;
		case Edge::Top:
			if (heading.y < 0.0f && object.position.y + object.size.y <= 0.0f)
			{
				object.position.y += windowDesc.height + object.size.y;
			}
			break;
		}
	}

	// Rides along with what it is touching: for this frame the object moves
	// with the other one's velocity as well as its own. Not kept - it is
	// worked out again every frame from whatever is still being touched, so
	// the moment the object is no longer on the other one, it is at rest.
	void CommandExecutor::carry(Object& object, const Object& other)
	{
		object.carry = other.velocity;
	}

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed)
	{
		Object& object = game.getObject(objectName);

		for (const auto& command : object.action[actionName])
		{
			std::visit(overload{
				[&](const CmdMove& m)
				{
					applyActionVelocity(object, m.direction, keyPressed ? m.step : 0.0f);
					if (keyPressed && m.step != 0.0f) { object.facing = m.direction; }
				},
				// A hop belongs to the press alone: letting go of the key does
				// nothing, and it is not among what executeHeldInput resumes.
				[&](const CmdHop& h) { if (keyPressed) { queueHop(object, h.direction, h.distance); object.facing = h.direction; } },
				// So does a jump, and one in the air waits for the landing.
				[&](const CmdJump& j) { if (keyPressed) { startJump(object, j); } },
				[&](const CmdLeap& l) { if (keyPressed) { leap(object, l.height); } },
				// A climb is held like a move; Game::applyClimbing makes it.
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
				[&](const CmdThrust& t) { applyActionThrustAhead(object, keyPressed ? t.amount : 0.0f, t.burn); },
				[&](const CmdFire& f) { if (keyPressed) { spawnProjectile(object, f.projectileName); } },
				// A sound belongs to the press, like a hop: letting go is silent.
				[&](const CmdPlay& p) { if (keyPressed) { game.requestSound(p.sound); } },
				[&](const auto&) { /* an object's own <action> list only ever produces
				                      move/hop/accelerate/turn/thrust/fire/play commands today; ignore anything else. */ }
			}, command);
		}
	}

	// Records which way `direction` is now pushing (0 = just released) and
	// recombines every direction currently held on that axis into its part of
	// the velocity, instead of this one key event simply overwriting it - see
	// Object::activeMoveStep for why that used to lose a still-held opposite
	// key. Left/Right and Up/Down are independent axes, so both can be held
	// at once for an 8-way diagonal off a 4-way D-pad, and a key on one axis
	// leaves the other alone (walking does not stop a fall). In a <leap> the
	// way across is kept until the landing, which takes up the keys held then.
	void CommandExecutor::applyActionVelocity(Object& object, Direction direction, float step)
	{
		object.activeMoveStep[static_cast<std::size_t>(direction)] = step;

		if (direction == Direction::Left || direction == Direction::Right)
		{
			if (!object.leaping)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
		else
		{
			object.velocity.y = object.activeMoveStep[static_cast<std::size_t>(Direction::Down)]
				- object.activeMoveStep[static_cast<std::size_t>(Direction::Up)];
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
		if (edge != Edge::Bottom || object.climbing || object.velocity.y < 0.0f)
		{
			return;
		}

		object.position.y = other.position.y - sizeOf(object).y;
		object.velocity.y = 0.0f;
		object.grounded = true;

		if (object.leaping)
		{
			object.leaping = false;
			object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
				- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
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

		object.velocity.y = -std::sqrt(2.0f * object.acceleration.y * height);
		object.grounded = false;
		object.leaping = true;
	}

	// Records that `direction` is now being pushed by `amount` a frame (0 =
	// just released), and the variable it burns; Game::applyAcceleration
	// applies every direction being held once a frame. Unlike a move's velocity
	// there is nothing to recombine: each direction adds its own amount.
	void CommandExecutor::applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn)
	{
		const auto index = static_cast<std::size_t>(direction);
		object.activeThrust[index] = amount;
		object.activeThrustBurn[index] = amount == 0.0f ? std::string{} : burn;
	}

	// Records that a turn is now held (rate 0 = just released); Game::
	// applyAcceleration turns the object once a frame while it is.
	void CommandExecutor::applyActionTurn(Object& object, Direction direction, float rate)
	{
		object.activeTurn[static_cast<std::size_t>(direction)] = rate;
	}

	// Records that thrust along the heading is now held (amount 0 = just
	// released), and the variable it burns.
	void CommandExecutor::applyActionThrustAhead(Object& object, float amount, const std::string& burn)
	{
		object.activeThrustAhead = amount;
		object.activeThrustAheadBurn = amount == 0.0f ? std::string{} : burn;
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
			if (isNamed(candidate, projectileName) && !candidate.collisionData.enabled)
			{
				projectile = &candidate;
				break;
			}
		}
		if (!projectile) { return; }

		if (shooter.hasHeading)
		{
			const float radians = shooter.heading * 3.14159265358979323846f / 180.0f;
			const Vector2f ahead{ std::sin(radians), -std::cos(radians) };
			const Vector2f size = sizeOf(shooter);
			const Vector2f centre = shooter.position + size * 0.5f;
			const float speed = std::hypot(projectile->velocityOriginal.x, projectile->velocityOriginal.y);

			projectile->position = centre + ahead * (std::max(size.x, size.y) / 2.0f) - sizeOf(*projectile) * 0.5f;
			projectile->velocity = ahead * speed;
		}
		else if (shooter.hasFacing)
		{
			// From the middle of the side it faces, just clear of it, moving that
			// way at the projectile's own speed.
			const Vector2f size = sizeOf(shooter);
			const Vector2f shot = sizeOf(*projectile);
			const float speed = std::hypot(projectile->velocityOriginal.x, projectile->velocityOriginal.y);
			const float middleX = shooter.position.x + size.x / 2 - shot.x / 2;
			const float middleY = shooter.position.y + size.y / 2 - shot.y / 2;

			switch (shooter.facing)
			{
			case Direction::Up:
				projectile->position = { middleX, shooter.position.y - shot.y };
				projectile->velocity = { 0.0f, -speed };
				break;
			case Direction::Down:
				projectile->position = { middleX, shooter.position.y + size.y };
				projectile->velocity = { 0.0f, speed };
				break;
			case Direction::Left:
				projectile->position = { shooter.position.x - shot.x, middleY };
				projectile->velocity = { -speed, 0.0f };
				break;
			case Direction::Right:
				projectile->position = { shooter.position.x + size.x, middleY };
				projectile->velocity = { speed, 0.0f };
				break;
			}
		}
		else
		{
			// From the shooter's top-centre: the projectile's own middle over
			// the shooter's, so a thin shot leaves the middle of the gun.
			projectile->position.x = shooter.position.x + sizeOf(shooter).x / 2 - sizeOf(*projectile).x / 2;
			projectile->position.y = shooter.position.y;
			projectile->velocity = projectile->velocityOriginal;
		}

		projectile->isVisible = true;
		projectile->collisionData.enabled = true;
	}
}
