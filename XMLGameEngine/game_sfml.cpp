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
		sf::CircleShape circle;
		sf::RectangleShape rectangle;
		sf::Font font;
		sf::Text text;

		std::string fontFile{ "assets/arial.ttf" };
		if (!font.loadFromFile(fontFile))
		{
			std::cout << "error: failed to load font: " << fontFile << std::endl;
		}

		object.renderTexture = std::make_shared<sf::RenderTexture>();

		if (object.src.compare(0, 12, "shape.circle") == 0)
		{
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
			text.setFont(font);
			text.setString(object.sparams.at(0));
			text.setCharacterSize(static_cast<int>(object.params.at(0)));
			text.setFillColor(sf::Color::Green);
			text.setPosition(-text.getGlobalBounds().left, -text.getGlobalBounds().top );

			int width = static_cast<int>(std::ceil(text.getGlobalBounds().width));
			int height = static_cast<int>(std::ceil(text.getGlobalBounds().height));

			object.renderTexture->create(width, height);
			object.renderTexture->draw(text);
		}

		object.renderTexture->display();

		object.sprite = std::make_shared<sf::Sprite>();
		object.sprite->setTexture(object.renderTexture->getTexture());
		object.sprite->setPosition(object.position);
	}
}
