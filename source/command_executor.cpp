// command_executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

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
			[&](const CmdDie&)
			{
				object.collisionData.enabled = false;
				object.isVisible = false;
			},
			[&](const CmdMove& m) { moveByStep(object, m.direction, m.step); },
			[&](const CmdIncrement& i) { game.incrementText(i.target); },
			[&](const auto&) { /* CmdPushState/CmdPopState/CmdFire/CmdTriggerAction never
			                      appear in a collisionData list; ignore defensively. */ }
		}, command);
	}

	void CommandExecutor::executeObjectCollision(const Command& command, Object& object, Edge edge, bool isPrimaryMover)
	{
		std::visit(overload{
			[&](const CmdBounce&) { bounceOffEdge(object, edge); },
			[&](const CmdDie&)
			{
				object.collisionData.enabled = false;
				object.isVisible = false;

				// Matches the original circleRectangleCollision: only the circular
				// object that initiated the hit gets stopped dead and parked
				// off-screen on death; the object it hit just stops colliding.
				if (isPrimaryMover)
				{
					object.velocity = {};
					object.position = { -100.0f, -100.0f };
				}
			},
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
			[&](const auto&) { /* bounce/stick/reset/die/move/inc/fire never appear
			                      directly on a state's <input>; only reachable via
			                      CmdTriggerAction into an object's own action list. */ }
		}, command);
	}

	void CommandExecutor::bounceScreenEdge(Object& object, Edge edge)
	{
		// A grouped object (e.g. the invader block in spaceinvaders) bounces as a
		// whole group sideways instead of just moving this one object.
		// circleRectangleCollision never had group behaviour, so that path
		// (executeObjectCollision) always goes straight to bounceOffEdge.
		if (object.collisionData.group && (edge == Edge::Left || edge == Edge::Right))
		{
			game.updateGroupOfObjects(object, edge == Edge::Left ? "left" : "right");
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
		const auto objectWidth = object.sprite->getLocalBounds().size.x;
		const auto objectHeight = object.sprite->getLocalBounds().size.y;
		const auto& windowDesc = game.getWindowDesc();

		if (edge == Edge::Left || edge == Edge::Right)
		{
			object.position.x = std::clamp(object.position.x, 0.0f, windowDesc.width - objectWidth);
		}
		else if (edge == Edge::Top || edge == Edge::Bottom)
		{
			object.position.y = std::clamp(object.position.y, 0.0f, windowDesc.height - objectHeight);
		}

		// NOTE: preserved verbatim from the original checkEdge - this always
		// zeroes velocity.x, even on a top/bottom stick (likely meant to be
		// velocity.y there). No game currently relies on the top/bottom case,
		// so this is left alone rather than silently changed.
		object.velocity.x = 0;
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

	void CommandExecutor::applyActionVelocity(Object& object, Direction direction, float step)
	{
		switch (direction)
		{
		case Direction::Up:    object.velocity.y = -step; break;
		case Direction::Down:  object.velocity.y = step; break;
		case Direction::Left:  object.velocity.x = -step; break;
		case Direction::Right: object.velocity.x = step; break;
		}
	}

	void CommandExecutor::spawnProjectile(Object& shooter, const std::string& projectileName)
	{
		Object& projectile = game.getObject(projectileName);

		if (!projectile.collisionData.enabled)
		{
			projectile.position.x = shooter.position.x + shooter.sprite->getLocalBounds().size.x / 2;
			projectile.position.y = shooter.sprite->getGlobalBounds().position.y;
			projectile.velocity.y = projectile.variable["speed"];
			projectile.isVisible = true;
			projectile.collisionData.enabled = true;
		}
	}
}

