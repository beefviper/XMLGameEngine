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

		auto checkCollision = [&object](std::string type) { return object.collisionData.find(type) != std::string::npos; };

		if (object.collision) {
			// check collision with edges of screen
			if ( (object.position.y < 0 && checkCollision("top: bounce") )
				|| (object.position.y > windowDesc.height - objectHeight && checkCollision("bottom: bounce") ) )
			{
				object.velocity.y *= -1;
			}
			if ( (object.position.y < 0 && checkCollision("top: static"))
				|| (object.position.y > windowDesc.height - objectHeight && checkCollision("bottom: static") ) )
			{
				if (object.position.y < 0) object.position.y = 0;
				if (object.position.y > windowDesc.height - objectHeight) object.position.y = windowDesc.height - objectHeight;
				object.velocity.y = 0;
			}

			// check collision with other objects
			if (checkCollision("default"))
			{
				for (auto& otherObjectName : getCurrentState().show)
				{
					auto& otherObject = getObject(otherObjectName);
					auto isCircular = getSObject(object.name).ssrc.find("shape.circle") != std::string::npos;

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
