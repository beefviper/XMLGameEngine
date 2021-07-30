// game_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 21, 2020

#include "game_sfml.h"

namespace xge
{
	void game_sfml::init(std::vector<Object>& preObjects, std::vector<Object>& objects)
	{
		for (auto& object : preObjects)
		{
			auto gridXmax = 1;
			auto gridYmax = 1;

			auto gridXpadding = 0;
			auto gridYpadding = 0;

			auto objWidth = 0;
			auto objHeight = 0;

			if (object.spriteParams.size() > 5)
			{
				if (object.spriteParams.at(3) == "grid" && object.spriteParams.at(0) == "circle")
				{
					gridXmax = std::stoi(object.spriteParams.at(4));
					gridYmax = std::stoi(object.spriteParams.at(5));

					gridXpadding = std::stoi(object.spriteParams.at(6));
					gridYpadding = std::stoi(object.spriteParams.at(7));

					objWidth = std::stoi(object.spriteParams.at(1));
					objHeight = std::stoi(object.spriteParams.at(1));
				}
				else if (object.spriteParams.at(4) == "grid" && object.spriteParams.at(0) == "rectangle")
				{
					gridXmax = std::stoi(object.spriteParams.at(5));
					gridYmax = std::stoi(object.spriteParams.at(6));

					gridXpadding = std::stoi(object.spriteParams.at(7));
					gridYpadding = std::stoi(object.spriteParams.at(8));

					objWidth = std::stoi(object.spriteParams.at(1));
					objHeight = std::stoi(object.spriteParams.at(2));
				}
			}

			auto totalObjects = gridXmax * gridYmax;
			for (auto gridX = 0; gridX < gridXmax; gridX++)
			{
				for (auto gridY = 0; gridY < gridYmax; gridY++)
				{
					Object newObject{};
					newObject.action = object.action;
					newObject.collision = object.collision;
					newObject.collisionData = object.collisionData;
					newObject.collisionDataEx = object.collisionDataEx;
					newObject.isVisible = object.isVisible;
					newObject.name = object.name;
					newObject.position = object.position;
					newObject.position_original = object.position_original;
					newObject.sposition = object.sposition;
					newObject.spriteParams = object.spriteParams;
					newObject.src = object.src;
					newObject.svelocity = object.svelocity;
					newObject.velocity = object.velocity;
					newObject.velocity_original = object.velocity_original;
					newObject.renderTexture = std::make_unique<sf::RenderTexture>();
					newObject.sprite = std::make_unique<sf::Sprite>();

					newObject.position.x = object.position.x + ((objWidth + gridXpadding) * gridX);
					newObject.position.y = object.position.y + ((objHeight + gridYpadding) * gridY);
					newObject.position_original = newObject.position;

					objects.push_back(std::move(newObject));
				}
			}
		}

		for (auto& object : objects)
		{
			object.renderTexture = std::make_unique<sf::RenderTexture>();

			if (object.spriteParams.at(0) == "circle")
			{
				createCircle(object);
			}
			else if (object.spriteParams.at(0) == "rectangle")
			{

				createRectangle(object);
			}
			else if (object.spriteParams.at(0) == "text")
			{
				createText(object);
			}
			else if (object.spriteParams.at(0) == "image")
			{
				createImage(object);
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

		auto number = std::stoi(object.spriteParams.at(1));
		number++;
		object.spriteParams.at(1) = std::to_string(number);

		text.setFont(font);
		text.setString(object.spriteParams.at(1));
		text.setCharacterSize(std::stoi(object.spriteParams.at(2)));
		text.setFillColor(sfmlColor(object.spriteParams.at(3)));

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

	void game_sfml::createCircle(Object& object)
	{
		sf::CircleShape circle;

		const float radius = std::stof(object.spriteParams.at(1));

		circle.setRadius(radius);
		circle.setFillColor(sfmlColor(object.spriteParams.at(2)));

		const int width = static_cast<int>(std::ceil(circle.getLocalBounds().width));
		const int height = static_cast<int>(std::ceil(circle.getLocalBounds().height));

		object.renderTexture->create(width, height);
		object.renderTexture->draw(circle);
	}

	void game_sfml::createRectangle(Object& object)
	{
		sf::RectangleShape rectangle;

		const float recWidth = std::stof(object.spriteParams.at(1));
		const float recHeight = std::stof(object.spriteParams.at(2));

		rectangle.setSize(sf::Vector2f(recWidth, recHeight));
		rectangle.setFillColor(sfmlColor(object.spriteParams.at(3)));

		const int width = static_cast<int>(std::ceil(rectangle.getLocalBounds().width));
		const int height = static_cast<int>(std::ceil(rectangle.getLocalBounds().height));

		object.renderTexture->create(width, height);
		object.renderTexture->draw(rectangle);
	}

	void game_sfml::createText(Object& object)
	{
		sf::Font font;
		sf::Text text;

		std::string fontFile{ "games/assets/tuffy.ttf" };
		if (!font.loadFromFile(fontFile))
		{
			std::cout << "error: failed to load font: " << fontFile << std::endl;
		}

		text.setFont(font);
		text.setString(object.spriteParams.at(1));
		text.setCharacterSize(std::stoi((object.spriteParams.at(2))));
		text.setFillColor(sfmlColor(object.spriteParams.at(3)));
		text.setPosition(-text.getLocalBounds().left, -text.getLocalBounds().top);

		const int width = static_cast<int>(std::ceil(text.getLocalBounds().width));
		const int height = static_cast<int>(std::ceil(text.getLocalBounds().height));

		object.renderTexture->create(width, height);
		object.renderTexture->draw(text);
	}

	void game_sfml::createImage(Object& object)
	{
		sf::Texture texture;
		sf::Sprite sprite;

		auto& imageFile = object.spriteParams.at(1);

		if (!texture.loadFromFile(imageFile))
		{
			std::cout << "error: SFML Image: failed to load " << imageFile << '\n';
			exit(EXIT_FAILURE);
		}

		sprite.setTexture(texture);

		const int width = static_cast<int>(std::ceil(sprite.getLocalBounds().width));
		const int height = static_cast<int>(std::ceil(sprite.getLocalBounds().height));

		if (object.spriteParams.at(2) == "flip.horizontal")
		{
			sprite.setTextureRect(sf::IntRect(width, 0, -width, height));
		}
		else if (object.spriteParams.at(2) == "flip.vertical")
		{
			sprite.setTextureRect(sf::IntRect(0, height, width, -height));
		}

		object.renderTexture->create(width, height);
		object.renderTexture->draw(sprite);
	}
}
