// game.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"

xge::Game::Game(std::string game) :
	filename(game)
{
	initXML();
	initEXPR();
	initSFML();
}

void xge::Game::updateObjects(void)
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
			
			if ( stuckToTop(object) || stuckToBottom(object))
			{
				object.position.y = xge::clamp(object.position.y, 0.0f, windowDesc.height - objectHeight);
				object.velocity.y = 0;
			}

			if (stuckToLeft(object) || stuckToRight(object))
			{
				object.position.x = xge::clamp(object.position.x, 0.0f, windowDesc.width - objectWidth);
				object.velocity.x = 0;
			}

			// check collision with other objects
			if (object.collisionData.basic != "")
			{
				for (auto& otherObjectName : getCurrentState().show)
				{
					auto& otherObject = getObject(otherObjectName);
					auto isCircular = object.src.find("shape.circle") != std::string::npos;

					if (object.name != otherObject.name && otherObject.collision && isCircular )
					{
						auto midpoint = object.sprite->getPosition() +
							sf::Vector2f(object.sprite->getLocalBounds().width / 2, object.sprite->getLocalBounds().height / 2);

						auto otherObjectLeft = otherObject.sprite->getPosition().x;
						auto otherObjectRight = otherObjectLeft + otherObject.sprite->getLocalBounds().width;
						auto otherObjectTop = otherObject.sprite->getPosition().y;
						auto otherObjectBottom = otherObjectTop + otherObject.sprite->getLocalBounds().height;

						sf::Vector2f nearestPoint;
						nearestPoint.x = std::max(otherObjectLeft, std::min(midpoint.x, otherObjectRight));
						nearestPoint.y = std::max(otherObjectTop, std::min(midpoint.y, otherObjectBottom));

						auto rayToNearest = nearestPoint - midpoint;
						auto magOfray = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);
						auto overlap = object.sprite->getLocalBounds().width / 2 - magOfray;
						if (std::isnan(overlap)) overlap = 0;

						if (overlap > 0)
						{
							object.velocity.x *= -1;
						}
					}
				}
			}
		}

		object.position.x += object.velocity.x;
		object.position.y += object.velocity.y;

		object.sprite->setPosition(object.position.x, object.position.y);
	}
}

void xge::Game::printGame(void)
{
	// debug -- print out window description
	std::cout << "window: ";
	std::cout << "title=" << windowDesc.name;
	std::cout << ", width=" << windowDesc.width;
	std::cout << ", height=" << windowDesc.height << std::endl;
	std::cout << std::endl;

	// debug -- print out variables 
	for (auto& variable : variables)
	{
		std::cout << "variable: name=" << variable.first << ", value=" << variable.second << std::endl;
	}
	std::cout << std::endl;

	// debug -- print out objects
	for (auto& object : objects)
	{
		std::cout << object << std::endl;
	}

	// debug -- print out states
	for (auto& state : states)
	{
		std::cout << "state: ";
		std::cout << "name=" << state.name << ", show=";
		for (auto& show : state.show)
		{
			std::cout << show << (show != state.show.back() ? ", " : "\n");
		}
		for (auto& input : state.input)
		{
			std::cout << "       action=" << input.first << ", button=" << input.second << std::endl;
		}
		std::cout << std::endl;
	}
}

xge::WindowDesc& xge::Game::getWindowDesc(void)
{
	return windowDesc;
}

std::vector<xge::Object>& xge::Game::getObjects(void)
{
	return objects;
}

xge::Object& xge::Game::getObject(std::string name)
{
	auto result = std::find_if(std::begin(objects), std::end(objects), [&](xge::Object& obj) { return obj.name == name; });
	return *result;
}

xge::SObject& xge::Game::getSObject(std::string name)
{
	auto result = std::find_if(std::begin(sObjects), std::end(sObjects), [&](xge::SObject& obj) { return obj.name == name; });
	return *result;
}

float& xge::Game::getVariable(std::string name)
{
	auto result = std::find_if(std::begin(variables), std::end(variables), [&](std::pair<const std::string, float>& var) { return var.first == name; });
	return result->second;
}

xge::State xge::Game::getCurrentState(void)
{
	return currentState.top();
}

void xge::Game::setCurrentState(int index)
{
	currentState.push(states.at(index));
}

void xge::Game::setCurrentState(std::string name)
{
	auto result = std::find_if(std::begin(states), std::end(states), [&](xge::State& state) { return state.name == name; });
	currentState.push(*result);
}

void xge::Game::setObjectParam(std::string name, std::string param, float value)
{
	auto result = std::find_if(std::begin(objects), std::end(objects), [&](xge::Object& obj) { return obj.name == name; });
	if (param == "velocity")
	{
		result->velocity.y = value;
	}
}

bool xge::Game::bouncedOffTop(xge::Object& object)
{
	return object.position.y < 0 && object.collisionData.top == "bounce";
}

bool xge::Game::bouncedOffBottom(xge::Object& object)
{
	auto objectHeight = object.sprite->getLocalBounds().height;
	return object.position.y > windowDesc.height - objectHeight && object.collisionData.bottom == "bounce";
}

bool xge::Game::bouncedOffLeft(xge::Object& object)
{
	return object.position.x < 0 && object.collisionData.left == "bounce";
}

bool xge::Game::bouncedOffRight(xge::Object& object)
{
	auto objectWidth = object.sprite->getLocalBounds().width;
	return object.position.x > windowDesc.height - objectWidth && object.collisionData.right == "bounce";
}

bool xge::Game::stuckToTop(xge::Object& object)
{
	return object.position.y < 0 && object.collisionData.top == "static";
}

bool xge::Game::stuckToBottom(xge::Object& object)
{
	auto objectHeight = object.sprite->getLocalBounds().height;
	return object.position.y > windowDesc.height - objectHeight && object.collisionData.bottom == "static";
}

bool xge::Game::stuckToLeft(xge::Object& object)
{
	return object.position.x < 0 && object.collisionData.left == "static";
}

bool xge::Game::stuckToRight(xge::Object& object)
{
	auto objectWidth = object.sprite->getLocalBounds().width;
	return object.position.x > windowDesc.width - objectWidth && object.collisionData.right == "static";
}
