// game_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game.h"
#include "game_sfml.h"

namespace xge
{
	void Game::initSFML(void)
	{
		for (auto& object : objects)
		{
			object.renderTexture = std::make_unique<sf::RenderTexture>();

			if (object.src.compare(0, 12, "shape.circle") == 0)
			{
				sf::CircleShape circle;

				float radius = object.params.at(0);

				circle.setRadius(radius);
				circle.setFillColor(sfmlColor(object.sparams.at(0)));

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
				rectangle.setFillColor(sfmlColor(object.sparams.at(0)));

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
				text.setFillColor(sfmlColor(object.sparams.at(1)));
				text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top);

				int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
				int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

				object.renderTexture->create(width, height);
				object.renderTexture->draw(text);
			}
			else if (object.src.compare(0, 5, "image") == 0)
			{
				sf::Texture texture;
				sf::Sprite sprite;

				auto& imageFile = object.sparams.at(0);

				if (!texture.loadFromFile(imageFile))
				{
					std::cout << "error: SFML Image: failed to load " << imageFile << '\n';
					exit(EXIT_FAILURE);
				}

				sprite.setTexture(texture);

				int width = static_cast<int>(std::ceil(sprite.getLocalBounds().width));
				int height = static_cast<int>(std::ceil(sprite.getLocalBounds().height));

				if (object.sparams.at(1) == "flip.horizontal")
				{
					sprite.setTextureRect(sf::IntRect(width, 0, -width, height));
				}
				else if (object.sparams.at(1) == "flip.vertical")
				{
					sprite.setTextureRect(sf::IntRect(0, height, width, -height));
				}

				object.renderTexture->create(width, height);
				object.renderTexture->draw(sprite);
			}

			object.renderTexture->display();

			object.sprite = std::make_unique<sf::Sprite>();
			object.sprite->setTexture(object.renderTexture->getTexture());
			object.sprite->setPosition(object.position);
		}
	}

	void Game::updateSFMLTextIncrementValue(Object& object)
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
		text.setFillColor(sfmlColor(object.sparams.at(1)));

		int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
		int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

		text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top);

		object.renderTexture = std::make_unique<sf::RenderTexture>();
		object.renderTexture->create(width, height);
		object.renderTexture->draw(text);
		object.renderTexture->display();

		object.sprite = std::make_unique<sf::Sprite>();
		object.sprite->setTexture(object.renderTexture->getTexture());
		object.sprite->setPosition(object.position);
	}
}
