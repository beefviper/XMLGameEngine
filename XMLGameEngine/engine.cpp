// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "engine.h"

xge::Engine::Engine(xge::Game& game) :
	game(game)
{
	xge::WindowDesc windowDesc = game.getWindowDesc();

	std::string name = windowDesc.name;

	int width = static_cast<int>(windowDesc.width);
	int height = static_cast<int>(windowDesc.height);
	sf::VideoMode videoMode(width, height);

	auto windowMode = (windowDesc.fullscreen == "true") ? sf::Style::Fullscreen : sf::Style::Default;

	window.create(videoMode, name, windowMode);
	window.setFramerateLimit(windowDesc.framerate);
}

void xge::Engine::loop(void)
{
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;

			case sf::Event::KeyPressed:
				handleKeyPressed(game, event);
				break;

			case sf::Event::KeyReleased:
				handleKeyReleased(game, event);
				break;

			default:
				break;
			}
		}

		game.updateObjects();

		window.clear(xge::sfmlColor(game.getWindowDesc().background));

		for (auto& name : game.getCurrentState().show)
		{
			window.draw(*game.getObject(name).sprite);
		}

		window.display();
	}
}

void xge::Engine::handleKeyPressed(xge::Game& game, sf::Event& event)
{
	if (!isKeyPressed[event.key.code])
	{
		isKeyPressed[event.key.code] = true;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(game, event, input, true);
		}
	}
}

void xge::Engine::handleKeyReleased(xge::Game& game, sf::Event& event)
{

	isKeyPressed[event.key.code] = false;

	for (auto& input : game.getCurrentState().input)
	{
		execute_action(game, event, input, false);
	}
}

void xge::Engine::execute_action(xge::Game& game, sf::Event& event, PairStringString& input, bool keyPressed)
{
	if (input.second == sfmlKeyToString(event.key.code))
	{
		std::string aObject = getObjectFromInput(input);
		std::string aCommand = getCommandFromInput(input);

		if (aObject == "state")
		{
			if (keyPressed)
			{
				game.setCurrentState(aCommand);
			}
		}
		else if (&game.getObject(aObject) != nullptr)
		{
			std::string aAction = game.getObject(aObject).action[aCommand];
			auto aStep = getValueFromAction(aAction);

			if (!keyPressed)
			{
				aStep = 0;
			}

			move(game.getObject(aObject), aCommand, aStep);
		}
	}
}

void xge::Engine::move(xge::Object& object, Engine::Direction direction, float step)
{
	switch (direction)
	{
	case Direction::Up:
		object.velocity.y = -step;
		break;
	case Direction::Down:
		object.velocity.y = step;
		break;
	case Direction::Left:
		object.velocity.x = -step;
		break;
	case Direction::Right:
		object.velocity.x = step;
		break;
	}
}

void xge::Engine::move(xge::Object& object, std::string direction, float step)
{
	move(object, mapDirection[direction], step);
}

std::string xge::Engine::getObjectFromInput(PairStringString input)
{
	auto sStart = 0;
	auto sEnd = input.first.find(".");

	return input.first.substr(sStart, sEnd);
}

std::string xge::Engine::getCommandFromInput(PairStringString input)
{
	auto sStart = input.first.rfind('.') + 1;
	auto sEnd = input.first.length();
	auto sLength = sEnd - sStart;

	return input.first.substr(sStart, sLength);
}

float xge::Engine::getValueFromAction(std::string action)
{
	auto sStart = action.find("(") + 1;
	auto sEnd = action.find(")");
	auto sLength = sEnd - sStart;
	auto sVar = action.substr(sStart, sLength);

	return game.getVariable(sVar);
}
