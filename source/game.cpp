// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

#include "command_executor.h"

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
		for (auto& object : getCurrentObjects())
		{
			if (isShown(object)) // TODO: at some point you might wanna collide with invisible objects
			{
				if (object.collisionData.enabled && (object.velocity.x != 0 || object.velocity.y != 0))
				{
					// check collisions with edges of screen
					checkEdge(object, Edge::Top);
					checkEdge(object, Edge::Bottom);
					checkEdge(object, Edge::Left);
					checkEdge(object, Edge::Right);

					// check collision with other objects (only circular movers can
					// currently initiate an object-object check - see ShapeKind)
					if (!object.collisionData.basic.empty() && object.shapeKind == ShapeKind::Circle)
					{
						for (auto& otherObject : getCurrentObjects())
						{
							circleRectangleCollision(object, otherObject);
						}
					}
				}
				// TODO: currently only checks objects with edges, and circular objects with walls and other objects
				//   will also need to check objects with other objects, and some method for detecting possible collisions

				// TODO: currently only have position and velocity, will probably need acceleration too
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

	bool Game::circleRectangleCollision(Object& object, Object& otherObject)
	{
		if (object.name == otherObject.name
			|| !otherObject.collisionData.enabled
			|| object.shapeKind != ShapeKind::Circle)
		{
			return false;
		}

		const auto edge = CollisionDetector::circleRectangle(object, otherObject);
		if (!edge)
		{
			return false;
		}

		// Both participants get a say: the circular mover runs its own 'basic'
		// commands (e.g. the ball bouncing), and whatever it hit runs its own
		// (e.g. a brick or bullet dying). This replaces the old hardcoded
		// "always bounce the mover, then pattern-match for a 'die' command"
		// logic with the same generic dispatch checkEdge uses.
		CommandExecutor executor(*this);
		for (const auto& command : object.collisionData.basic)
		{
			executor.executeObjectCollision(command, object, *edge, true);
		}
		for (const auto& command : otherObject.collisionData.basic)
		{
			executor.executeObjectCollision(command, otherObject, *edge, false);
		}

		return true;
	}
}
