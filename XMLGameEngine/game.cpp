// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

namespace xge
{
	Game::Game(const std::string& game) :
		filename(game)
	{
		initXML();
		initEXPR();
		initSFML();
	}

	void Game::updateObjects(void)
	{
		for (auto& objectName : getCurrentState().show)
		{
			auto& object = getObject(objectName);

			auto objectWidth = object.sprite->getLocalBounds().width;
			auto objectHeight = object.sprite->getLocalBounds().height;

			if (object.collision) {
				// check collision with edges of screen
				if (bouncedOffTop(object) || bouncedOffBottom(object))
				{
					object.velocity.y *= -1;
				}

				if (bouncedOffLeft(object) || bouncedOffRight(object))
				{
					object.velocity.x *= -1;
				}

				if (stuckToTop(object) || stuckToBottom(object))
				{
					object.position.y = std::clamp(object.position.y, 0.0f, windowDesc.height - objectHeight);
					object.velocity.y = 0;
				}

				if (stuckToLeft(object) || stuckToRight(object))
				{
					object.position.x = std::clamp(object.position.x, 0.0f, windowDesc.width - objectWidth);
					object.velocity.x = 0;
				}

				if (object.position.x < 0 && object.collisionData.left == "inc('score2')")
				{
					object.position.x = windowDesc.width / 2 + object.sprite->getLocalBounds().width / 2;
					updateSFMLTextIncrementValue(getObject("score2"));
				}

				if (object.position.x > windowDesc.width - objectWidth && object.collisionData.right == "inc('score1')")
				{
					object.position.x = windowDesc.width / 2 + object.sprite->getLocalBounds().width / 2;
					updateSFMLTextIncrementValue(getObject("score1"));
				}

				// check collision with other objects
				if (object.collisionData.basic != "")
				{
					for (auto& otherObjectName : getCurrentState().show)
					{
						auto& otherObject = getObject(otherObjectName);
						if (circleRectangleCollision(object, otherObject))
						{
							object.velocity.x *= -1;
						}
					}
				}
			}

			object.position.x += object.velocity.x;
			object.position.y += object.velocity.y;

			object.sprite->setPosition(object.position.x, object.position.y);
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

		for (auto& object : objects)
		{
			std::cout << object << '\n';
		}

		for (auto& state : states)
		{
			std::cout << state << '\n';
		}
	}

	WindowDesc& Game::getWindowDesc(void)
	{
		return windowDesc;
	}

	Object& Game::getObject(const std::string& name)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		return *result;
	}

	float& Game::getVariable(const std::string& name)
	{
		auto result = std::find_if(std::begin(variables), std::end(variables), [&](std::pair<const std::string, float>& var) { return var.first == name; });
		return result->second;
	}

	State Game::getCurrentState(void)
	{
		return currentState.top();
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

	void Game::setObjectParam(const std::string& name, const std::string& param, const float& value)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		if (param == "velocity")
		{
			//result->velocity.y = value;
			result->velocity.y = value;
		}
	}

	bool Game::bouncedOffTop(Object& object)
	{
		return object.position.y < 0 && object.collisionData.top == "bounce";
	}

	bool Game::bouncedOffBottom(Object& object)
	{
		auto objectHeight = object.sprite->getLocalBounds().height;
		return object.position.y > windowDesc.height - objectHeight && object.collisionData.bottom == "bounce";
	}

	bool Game::bouncedOffLeft(Object& object)
	{
		return object.position.x < 0 && object.collisionData.left == "bounce";
	}

	bool Game::bouncedOffRight(Object& object)
	{
		auto objectWidth = object.sprite->getLocalBounds().width;
		return object.position.x > windowDesc.width - objectWidth && object.collisionData.right == "bounce";
	}

	bool Game::stuckToTop(Object& object)
	{
		return object.position.y < 0 && object.collisionData.top == "static";
	}

	bool Game::stuckToBottom(Object& object)
	{
		auto objectHeight = object.sprite->getLocalBounds().height;
		return object.position.y > windowDesc.height - objectHeight && object.collisionData.bottom == "static";
	}

	bool Game::stuckToLeft(Object& object)
	{
		return object.position.x < 0 && object.collisionData.left == "static";
	}

	bool Game::stuckToRight(Object& object)
	{
		auto objectWidth = object.sprite->getLocalBounds().width;
		return object.position.x > windowDesc.width - objectWidth && object.collisionData.right == "static";
	}

	bool Game::circleRectangleCollision(Object& object, Object& otherObject)
	{
		auto overlap{ 0.0f };
		auto isCircular = object.src.find("shape.circle") != std::string::npos;

		if (object.name != otherObject.name && otherObject.collision && isCircular)
		{
			auto midpoint = object.sprite->getPosition() +
				sf::Vector2f(object.sprite->getLocalBounds().width / 2, object.sprite->getLocalBounds().height / 2);

			auto otherObjectLeft = otherObject.sprite->getPosition().x;
			auto otherObjectRight = otherObjectLeft + otherObject.sprite->getLocalBounds().width;
			auto otherObjectTop = otherObject.sprite->getPosition().y;
			auto otherObjectBottom = otherObjectTop + otherObject.sprite->getLocalBounds().height;

			sf::Vector2f nearestPoint;
			nearestPoint.x = std::clamp(midpoint.x, otherObjectLeft, otherObjectRight);
			nearestPoint.y = std::clamp(midpoint.y, otherObjectTop, otherObjectBottom);

			auto rayToNearest = nearestPoint - midpoint;
			auto magOfray = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);
			overlap = object.sprite->getLocalBounds().width / 2 - magOfray;
			if (std::isnan(overlap)) overlap = 0;
		}
		return (overlap > 0);
	}
}
