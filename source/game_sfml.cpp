// game_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_sfml.h"

namespace xge
{
	void game_sfml::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			object.renderTexture = std::make_unique<sf::RenderTexture>();

			if (object.src.starts_with("shape.circle"))
			{
				sf::CircleShape circle;

				const float radius = std::stof(object.spriteParams.at(0));

				circle.setRadius(radius);
				circle.setFillColor(sfmlColor(object.spriteParams.at(1)));

				const int width = static_cast<int>(std::ceil(circle.getLocalBounds().width));
				const int height = static_cast<int>(std::ceil(circle.getLocalBounds().height));

				object.renderTexture->create(width, height);
				object.renderTexture->draw(circle);
			}
			else if (object.src.starts_with("shape.rectangle"))
			{
				sf::RectangleShape rectangle;

				const float recWidth = std::stof(object.spriteParams.at(0));
				const float recHeight = std::stof(object.spriteParams.at(1));

				rectangle.setSize(sf::Vector2f(recWidth, recHeight));
				rectangle.setFillColor(sfmlColor(object.spriteParams.at(2)));

				const int width = static_cast<int>(std::ceil(rectangle.getLocalBounds().width));
				const int height = static_cast<int>(std::ceil(rectangle.getLocalBounds().height));

				object.renderTexture->create(width, height);
				object.renderTexture->draw(rectangle);
			}
			else if (object.src.starts_with("text"))
			{
				sf::Font font;
				sf::Text text;

				std::string fontFile{ "games/assets/tuffy.ttf" };
				if (!font.loadFromFile(fontFile))
				{
					std::cout << "error: failed to load font: " << fontFile << std::endl;
				}

				text.setFont(font);
				text.setString(object.spriteParams.at(0));
				text.setCharacterSize(std::stoi((object.spriteParams.at(1))));
				text.setFillColor(sfmlColor(object.spriteParams.at(2)));
				text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top);

				const int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
				const int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

				object.renderTexture->create(width, height);
				object.renderTexture->draw(text);
			}
			else if (object.src.starts_with("image"))
			{
				sf::Texture texture;
				sf::Sprite sprite;

				auto& imageFile = object.spriteParams.at(0);

				if (!texture.loadFromFile(imageFile))
				{
					std::cout << "error: SFML Image: failed to load " << imageFile << '\n';
					exit(EXIT_FAILURE);
				}

				sprite.setTexture(texture);

				const int width = static_cast<int>(std::ceil(sprite.getLocalBounds().width));
				const int height = static_cast<int>(std::ceil(sprite.getLocalBounds().height));

				if (object.spriteParams.at(1) == "flip.horizontal")
				{
					sprite.setTextureRect(sf::IntRect(width, 0, -width, height));
				}
				else if (object.spriteParams.at(1) == "flip.vertical")
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

	void game_sfml::updateTextIncrementValue(Object& object)
	{
		sf::Font font;
		sf::Text text;

		std::string fontFile{ "games/assets/tuffy.ttf" };
		if (!font.loadFromFile(fontFile))
		{
			std::cout << "error: failed to load font: " << fontFile << std::endl;
		}

		auto number = std::stoi(object.spriteParams.at(0));
		number++;
		object.spriteParams.at(0) = std::to_string(number);

		text.setFont(font);
		text.setString(object.spriteParams.at(0));
		text.setCharacterSize(std::stoi(object.spriteParams.at(1)));
		text.setFillColor(sfmlColor(object.spriteParams.at(2)));

		const int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
		const int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

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
