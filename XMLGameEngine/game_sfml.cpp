// game_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game.h"
#include <SFML/System.hpp>

void xge::Game::initSFML(void)
{
	for (auto& object : objects)
	{
		object.renderTexture = std::make_shared<sf::RenderTexture>();

		if (object.src.compare(0, 12, "shape.circle") == 0)
		{
			sf::CircleShape circle;

			float radius = object.params.at(0);

			circle.setRadius(radius);
			circle.setFillColor(sf::Color::Green);

			int width = static_cast<int>(std::ceil(circle.getLocalBounds().width));
			int height = static_cast<int>(std::ceil(circle.getLocalBounds().height));

			object.renderTexture->create(width, height);
			object.renderTexture->draw(circle);
		}
		else if (object.src.compare(0, 15, "shape.rectangle") == 0)
		{
			sf::RectangleShape rectangle;

			float recWidth = object.params.at(0);
			float recHeight = object.params.at(1);

			rectangle.setSize(sf::Vector2f(recWidth, recHeight));
			rectangle.setFillColor(sf::Color::Magenta);

			int width = static_cast<int>(std::ceil(rectangle.getLocalBounds().width));
			int height = static_cast<int>(std::ceil(rectangle.getLocalBounds().height));

			object.renderTexture->create(width, height);
			object.renderTexture->draw(rectangle);
		}
		else if (object.src.compare(0, 4, "text") == 0)
		{
			sf::Font font;
			sf::Text text;

			std::string fontFile{ "assets/arial.ttf" };
			if (!font.loadFromFile(fontFile))
			{
				std::cout << "error: failed to load font: " << fontFile << std::endl;
			}

			text.setFont(font);
			text.setString(object.sparams.at(0));
			text.setCharacterSize(static_cast<int>(object.params.at(0)));
			text.setFillColor(sf::Color::Green);
			text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top );

			int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
			int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

			object.renderTexture->create(width, height);
			object.renderTexture->draw(text);
		}

		object.renderTexture->display();

		object.sprite = std::make_shared<sf::Sprite>();
		object.sprite->setTexture(object.renderTexture->getTexture());
		object.sprite->setPosition(object.position);
	}
}

void xge::Game::updateSFMLTexture(xge::Object& object)
{
	sf::Font font;
	sf::Text text;

	std::string fontFile{ "assets/arial.ttf" };
	if (!font.loadFromFile(fontFile))
	{
		std::cout << "error: failed to load font: " << fontFile << std::endl;
	}

	auto number = std::stoi(object.sparams.at(0));
	number++;
	object.sparams[0] = std::to_string(number);

	text.setFont(font);
	text.setString(object.sparams.at(0));
	text.setCharacterSize(static_cast<int>(object.params.at(0)));
	text.setFillColor(sf::Color::Green);

	int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
	int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

	text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top);

	object.renderTexture = std::make_shared<sf::RenderTexture>();

	object.renderTexture->create(width, height);
	object.renderTexture->draw(text);
	object.renderTexture->display();

	object.sprite = std::make_shared<sf::Sprite>();
	object.sprite->setTexture(object.renderTexture->getTexture());
	object.sprite->setPosition(object.position);
}
