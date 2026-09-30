// command_executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "command_executor.h"

#include "game.h"

namespace xge
{
	void CommandExecutor::executeScreenEdgeCollision(const Command& command, Object& object, Edge edge)
	{
		std::visit(overload{
			[&](const CmdBounce&) { bounceScreenEdge(object, edge); },
			[&](const CmdStick&) { stick(object, edge); },
			[&](const CmdReset&) { object.position = object.positionOriginal; },
			[&](const CmdDie&) { die(object); },
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target); },
			[&](const auto&) { /* CmdPushState/CmdPopState/CmdFire/CmdTriggerAction never
			                      appear in a collisionData list; ignore defensively. */ }
		}, command);
	}

	void CommandExecutor::executeObjectCollision(const Command& command, Object& object, Edge edge)
	{
		std::visit(overload{
			[&](const CmdBounce&) { bounceOffEdge(object, edge); },
			[&](const CmdDie&) { die(object); },
			[&](const auto&) { /* stick/reset/move/inc/etc. aren't used for
			                      object-object 'basic' collisions today; ignore. */ }
		}, command);
	}

	void CommandExecutor::executeInput(const Command& command, bool keyPressed)
	{
		std::visit(overload{
			[&](const CmdPushState& s) { if (keyPressed) { game.pushState(s.name); } },
			[&](const CmdPopState&) { if (keyPressed) { game.popState(); } },
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, keyPressed); },
			[&](const CmdResetObject& r) { if (keyPressed) { game.resetObject(r.target); } },
			// Bare reset() means something different here than it does inside a
			// collision (executeScreenEdgeCollision resets just the colliding
			// object's own position): with no "colliding object" to be implicit
			// about, it's the full-game reset - see Game::resetAll.
			[&](const CmdReset&) { if (keyPressed) { game.resetAll(); } },
			[&](const auto&) { /* bounce/stick/die/move/inc/fire never appear
			                      directly on a state's <input>; only reachable
			                      via CmdTriggerAction into an object's own
			                      action list. */ }
		}, command);
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
	}

	void CommandExecutor::bounceScreenEdge(Object& object, Edge edge)
	{
		// A grouped object (e.g. the invader block in spaceinvaders) bounces as a
		// whole group sideways instead of just moving this one object.
		// circleRectangleCollision never had group behaviour, so that path
		// (executeObjectCollision) always goes straight to bounceOffEdge.
		if (object.collisionData.group && (edge == Edge::Left || edge == Edge::Right))
		{
			game.updateGroupOfObjects(object);
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

		if (object.collisionData.group > 0)
		{
			for (auto& obj : game.getCurrentObjects())
			{
				if (obj.collisionData.group == object.collisionData.group)
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

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed)
	{
		Object& object = game.getObject(objectName);

		for (const auto& command : object.action[actionName])
		{
			std::visit(overload{
				[&](const CmdMove& m) { applyActionVelocity(object, m.direction, keyPressed ? m.step : 0.0f); },
				[&](const CmdFire& f) { if (keyPressed) { spawnProjectile(object, f.projectileName); } },
				[&](const auto&) { /* an object's own <action> list only ever produces
				                      move/fire commands today; ignore anything else. */ }
			}, command);
		}
	}

	// Records which way `direction` is now pushing (0 = just released) and
	// recombines every direction currently held into velocity.x/velocity.y,
	// instead of this one key event simply overwriting the whole axis - see
	// Object::activeMoveStep for why that used to lose a still-held opposite
	// key. Left/Right and Up/Down are independent axes, so both can be held
	// at once for an 8-way diagonal off a 4-way D-pad.
	void CommandExecutor::applyActionVelocity(Object& object, Direction direction, float step)
	{
		object.activeMoveStep[static_cast<std::size_t>(direction)] = step;

		object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
			- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
		object.velocity.y = object.activeMoveStep[static_cast<std::size_t>(Direction::Down)]
			- object.activeMoveStep[static_cast<std::size_t>(Direction::Up)];
	}

	void CommandExecutor::spawnProjectile(Object& shooter, const std::string& projectileName)
	{
		Object& projectile = game.getObject(projectileName);

		if (!projectile.collisionData.enabled)
		{
			projectile.position.x = shooter.position.x + shooter.size.x / 2;
			projectile.position.y = shooter.position.y;
			projectile.velocity.y = projectile.variable["speed"];
			projectile.isVisible = true;
			projectile.collisionData.enabled = true;
		}
	}
}
