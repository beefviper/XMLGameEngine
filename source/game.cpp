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
		xml.init(filename, windowDesc, variables, states, rawObjects);
		expr.init(windowDesc, variables, states, rawObjects, objects);
		sfml.init(objects);
	}

	void Game::updateObjects(void)
	{
		for (auto& object : getCurrentObjects())
		{
			if (isShown(object))
			{
				if (object.collisionData.enabled && (object.velocity.x != 0 || object.velocity.y != 0))
				{
					// check collisions with edges of screen
					checkEdge(object, "top");
					checkEdge(object, "bottom");
					checkEdge(object, "left");
					checkEdge(object, "right");

					// check collision with other objects
					if (object.collisionData.basic.size() != 0)
					{
						for (auto& otherObject : getCurrentObjects())
						{
							bool tookHit = false;
							const auto isCircular = object.src.find("shape.circle") != std::string::npos;
							if (isCircular && !tookHit)
							{
								tookHit = circleRectangleCollision(object, otherObject);
							}
						}
					}
				}

				object.position.x += object.velocity.x;
				object.position.y += object.velocity.y;

				object.sprite->setPosition(object.position.x, object.position.y);
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

	bool Game::isShown(Object& object)
	{
		bool result = false;

		for (auto shown : currentState.top().show)
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

	float& Game::getVariable(const std::string& name)
	{
		auto result = std::find_if(std::begin(variables), std::end(variables), [&](std::pair<const std::string, float>& var) { return var.first == name; });
		return result->second;
	}

	State Game::getCurrentState(void)
	{
		return currentState.top();
	}

	std::vector<Object>& Game::getCurrentObjects(void)
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

	void Game::setObjectParam(const std::string& name, const std::string& param, const float& value)
	{
		auto result = std::find_if(std::begin(objects), std::end(objects), [&](Object& obj) { return obj.name == name; });
		if (param == "velocity")
		{
			result->velocity.y = value;
		}
	}

	void Game::checkEdge(Object& object, std::string side)
	{
		auto objectWidth = object.sprite->getLocalBounds().width;
		auto objectHeight = object.sprite->getLocalBounds().height;

		std::vector<std::string> curSide{};
		float leftBound = 0;
		float rightBound = windowDesc.width - objectWidth;
		float topBound = 0;
		float bottomBound = windowDesc.height - objectHeight;

		if (side == "left") { curSide = object.collisionData.left; }
		else if (side == "right") { curSide = object.collisionData.right; }
		else if (side == "top") { curSide = object.collisionData.top; }
		else if (side == "bottom") { curSide = object.collisionData.bottom; }

		if ((object.position.x < leftBound && side == "left")
			|| (object.position.x > rightBound && side == "right")
			|| (object.position.y < topBound && side == "top")
			|| (object.position.y > bottomBound && side == "bottom"))
		{

			auto colIter = curSide.begin();
			while (colIter != curSide.end())
			{
				if (*colIter == "inc")
				{
					colIter++;
					sfml.updateTextIncrementValue(getObject(*colIter));
					colIter++;
				}
				else if (*colIter == "collide")
				{
					colIter++;
					if (*colIter == "reset")
					{
						object.position = object.positionOriginal;
					}
					else if (*colIter == "bounce")
					{
						if (side == "left" || side == "right") { object.velocity.x *= -1; }
						else if (side == "top" || side == "bottom") { object.velocity.y *= -1; }
					}
					else if (*colIter == "stick")
					{
						if (side == "left" || side == "right")
						{
							object.position.x = std::clamp(object.position.x, 0.0f, windowDesc.width - objectWidth);
						}
						else if (side == "top" || side == "bottom")
						{
							object.position.y = std::clamp(object.position.y, 0.0f, windowDesc.height - objectHeight);
						}
						object.velocity.x = 0;
					}
					else if (*colIter == "die")
					{
						object.collisionData.enabled = false;
						object.isVisible = false;
					}
					colIter++;
				}
			}
		}
	}

	bool Game::circleRectangleCollision(Object& object, Object& otherObject)
	{
		auto overlap{ 0.0f };
		const auto isCircular = object.src.find("shape.circle") != std::string::npos;
		std::string edgeTouched = "none";

		if (object.name != otherObject.name && otherObject.collisionData.enabled && isCircular)
		{
			const auto midpoint = object.sprite->getPosition() +
				sf::Vector2f(object.sprite->getLocalBounds().width / 2, object.sprite->getLocalBounds().height / 2);

			auto otherObjectLeft = otherObject.sprite->getPosition().x;
			auto otherObjectRight = otherObjectLeft + otherObject.sprite->getLocalBounds().width;
			auto otherObjectTop = otherObject.sprite->getPosition().y;
			auto otherObjectBottom = otherObjectTop + otherObject.sprite->getLocalBounds().height;

			sf::Vector2f nearestPoint;
			nearestPoint.x = std::clamp(midpoint.x, otherObjectLeft, otherObjectRight);
			nearestPoint.y = std::clamp(midpoint.y, otherObjectTop, otherObjectBottom);

			const auto rayToNearest = nearestPoint - midpoint;
			const auto magOfray = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);
			overlap = object.sprite->getLocalBounds().width / 2 - magOfray;
			if (std::isnan(overlap)) overlap = 0;

			if (overlap > 0)
			{
				if (midpoint.y > otherObjectTop - midpoint.y
					&& midpoint.y < otherObjectBottom + midpoint.y
					&& nearestPoint.x == otherObjectLeft)
				{
					edgeTouched = "left";
				}
				else if (midpoint.y > otherObjectTop - midpoint.y
					&& midpoint.y < otherObjectBottom + midpoint.y
					&& nearestPoint.x == otherObjectRight)
				{
					edgeTouched = "right";
				}
				else if (midpoint.x > otherObjectLeft - midpoint.x
					&& midpoint.x < otherObjectRight + midpoint.x
					&& nearestPoint.y == otherObjectTop)
				{
					edgeTouched = "top";
				}
				else if (midpoint.x > otherObjectLeft - midpoint.x
					&& midpoint.x < otherObjectRight + midpoint.x
					&& nearestPoint.y == otherObjectBottom)
				{
					edgeTouched = "bottom";
				}
			}
			else
			{
				edgeTouched = "none";
			}
		}

		if (edgeTouched != "none")
		{
			if (edgeTouched == "left") { object.velocity.x = std::abs(object.velocity.x) * -1; }
			else if (edgeTouched == "right") { object.velocity.x = std::abs(object.velocity.x); }
			else if (edgeTouched == "top") { object.velocity.y = std::abs(object.velocity.y) * -1; }
			else if (edgeTouched == "bottom") { object.velocity.y = std::abs(object.velocity.y); }
		}

		if (edgeTouched != "none")
		{
			if (otherObject.collisionData.basic.size() != 0)
			{
				if (otherObject.collisionData.basic.at(0) == "collide")
				{
					if (otherObject.collisionData.basic.at(1) == "die")
					{
						otherObject.isVisible = false;
						otherObject.collisionData.enabled = false;
					}
				}
			}
		}
		return edgeTouched != "none";
	}
}
