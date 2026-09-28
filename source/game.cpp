// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"

#include <cstddef>

namespace xge
{
	Game::Game(const std::string& game) :
		filename(game)
	{
		xml.init(filename, windowDesc, variables, rawStates, rawObjects);
		expr.init(windowDesc, variables, rawStates, states, rawObjects, objects);
		sfml.init(objects);
	}

	void Game::updateObjects(void)
	{
		auto& currentObjects = getCurrentObjects();

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : currentObjects)
		{
			if (isShown(object) && object.collisionData.enabled && (object.velocity.x != 0 || object.velocity.y != 0))
			{
				checkEdge(object, Edge::Top);
				checkEdge(object, Edge::Bottom);
				checkEdge(object, Edge::Left);
				checkEdge(object, Edge::Right);
			}
		}

		// Object-object checks: every unordered pair is looked at once. A
		// simple bounding-box test is enough for two rectangles; when either
		// side is a circle (the ball, a bullet) checkObjectCollision looks
		// closer to make sure it's actually touching, not just its bounding
		// box overlapping.
		for (std::size_t i = 0; i < currentObjects.size(); ++i)
		{
			if (!isShown(currentObjects[i]))
			{
				continue;
			}

			for (std::size_t j = i + 1; j < currentObjects.size(); ++j)
			{
				if (!isShown(currentObjects[j]))
				{
					continue;
				}

				checkObjectCollision(currentObjects[i], currentObjects[j]);
			}
		}

		// TODO: currently only have position and velocity, will probably need acceleration too
		for (auto& object : currentObjects)
		{
			if (isShown(object)) // TODO: at some point you might wanna collide with invisible objects
			{
				object.position.x += object.velocity.x;
				object.position.y += object.velocity.y;

				object.sprite->setPosition(object.position);
			}
		}
	}

	void Game::printGame(void)
	{
		std::cout << windowDesc << '\n';

		for (auto& variable : variables)
		{
			std::cout << "variable: name=" << variable.first << ", value=" << variable.second << '\n';
		}
		std::cout << '\n';

		for (auto& rawObject : rawObjects)
		{
			std::cout << rawObject << '\n';
		}

		for (auto& object : objects)
		{
			std::cout << object << '\n';
		}

		for (auto& state : states)
		{
			std::cout << state << '\n';
		}
	}

	WindowDesc& Game::getWindowDesc(void) noexcept
	{
		return windowDesc;
	}

	bool Game::isShown(const Object& object) noexcept
	{
		bool result = false;

		for (auto& shown : currentState.top().show)
		{
			if (shown == object.name)
			{
				result = true;
			}
		}
		return result && object.isVisible;
	}

	Object& Game::getObject(const std::string& name)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		return *result;
	}

	float Game::getVariable(const std::string& name)
	{
		auto result = std::find_if(std::begin(variables), std::end(variables), [&](std::pair<const std::string, float>& var) { return var.first == name; });

		if (result == variables.end())
		{
			return 0;
		}
		else
		{
			return result->second;
		}
	}

	State Game::getCurrentState(void)
	{
		return currentState.top();
	}

	std::vector<Object>& Game::getCurrentObjects(void) noexcept
	{
		return objects;
	}

	void Game::setCurrentState(const int& index)
	{
		currentState.push(states.at(index));
	}

	void Game::setCurrentState(const std::string& name)
	{
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
	}

	void Game::pushState(std::string name) {
		auto result = std::find_if(std::begin(states), std::end(states), [&](State& state) { return state.name == name; });
		currentState.push(*result);
	}

	void Game::popState(void) noexcept
	{
		currentState.pop();
	}

	void Game::setObjectParam(const std::string& name, const std::string& param, const float& value)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		if (param == "velocity")
		{
			result->velocity.y = value;
		}
	}

	void Game::incrementText(const std::string& objectName)
	{
		sfml.updateTextIncrementValue(getObject(objectName));
	}

	void Game::updateGroupOfObjects(const Object& object, std::string side) noexcept
	{
		const int groupNum = object.collisionData.group;

		bool foundObject = false;

		for (auto& obj : objects)
		{
			if (obj.collisionData.group == groupNum)
			{
				// TODO: fix logic, no need to set a variable, and then test it in the next block
				// get rid of varialbe foundObject, combine if, if, and else
				if (obj.position.x == object.position.x && obj.position.y == object.position.y)
				{
					foundObject = true;
				}

				if (foundObject == false)
				{
					obj.velocity.x *= -1;
					if (side == "right") { obj.position.x += obj.velocity.x * 3; }
					if (side == "left") { obj.position.x += obj.velocity.x; }
				}
				else
				{
					obj.velocity.x *= -1;
					if (side == "right") { obj.position.x += obj.velocity.x; }
					if (side == "left") { obj.position.x += obj.velocity.x; }
				}
			}
		}
	}

	void Game::checkEdge(Object& object, Edge edge)
	{
		if (!CollisionDetector::touchesScreenEdge(object, windowDesc, edge))
		{
			return;
		}

		std::vector<Command>* commands = nullptr;
		switch (edge)
		{
		case Edge::Left:   commands = &object.collisionData.left; break;
		case Edge::Right:  commands = &object.collisionData.right; break;
		case Edge::Top:    commands = &object.collisionData.top; break;
		case Edge::Bottom: commands = &object.collisionData.bottom; break;
		}

		CommandExecutor executor(*this);
		for (const auto& command : *commands)
		{
			executor.executeScreenEdgeCollision(command, object, edge);
		}
	}

	void Game::checkObjectCollision(Object& a, Object& b)
	{
		if (!a.collisionData.enabled || !b.collisionData.enabled)
		{
			return;
		}

		// Objects rigidly moving together as a group (e.g. the invader block
		// in spaceinvaders) never collide with each other.
		if (a.collisionData.group != 0 && a.collisionData.group == b.collisionData.group)
		{
			return;
		}

		// At least one side has to have moved for there to be anything new to
		// detect - two stationary objects can't have just started touching.
		const bool aMoved = (a.velocity.x != 0 || a.velocity.y != 0);
		const bool bMoved = (b.velocity.x != 0 || b.velocity.y != 0);
		if (!aMoved && !bMoved)
		{
			return;
		}

		// A plain bounding-box test is exact for two rectangles; when either
		// side is a circle, look closer. Either way this comes back as
		// "the edge of b that was touched" (see the comment on CollisionDetector).
		std::optional<Edge> edgeOfB;
		if (a.shapeKind == ShapeKind::Circle)
		{
			edgeOfB = CollisionDetector::circleRectangle(a, b);
		}
		else if (b.shapeKind == ShapeKind::Circle)
		{
			const auto edgeOfA = CollisionDetector::circleRectangle(b, a);
			edgeOfB = edgeOfA ? std::optional<Edge>(opposite(*edgeOfA)) : std::nullopt;
		}
		else
		{
			edgeOfB = CollisionDetector::rectangleRectangle(a, b);
		}

		if (!edgeOfB)
		{
			return;
		}

		// edgeOfB is self-relative for b already; a's own self-relative
		// touched edge is the opposite side.
		CommandExecutor executor(*this);
		for (const auto& command : a.collisionData.basic)
		{
			executor.executeObjectCollision(command, a, opposite(*edgeOfB));
		}
		for (const auto& command : b.collisionData.basic)
		{
			executor.executeObjectCollision(command, b, *edgeOfB);
		}
	}
}
