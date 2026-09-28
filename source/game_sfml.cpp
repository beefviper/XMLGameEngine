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

			// TODO: should be able to combine with setGridXY() in game_expr, caculate final x and y pos there
			const GridData gridData = setGridData(object);
			object.position.x = object.positionOriginal.x + ((gridData.obj.x + gridData.padding.x) * object.position.x);
			object.position.y = object.positionOriginal.y + ((gridData.obj.y + gridData.padding.y) * object.position.y);

			object.positionOriginal = object.position;
			// won't need above lines, once fixed

			object.renderTexture->display();

			object.sprite = std::make_unique<sf::Sprite>(object.renderTexture->getTexture());
			object.sprite->setPosition(object.position);
		}
	}

	void game_sfml::updateTextIncrementValue(Object& object)
	{
		const float newValue = std::stof(object.spriteParams.at(1)) + 1;
		setDisplayedNumber(object, newValue);
	}

	void game_sfml::setDisplayedNumber(Object& object, float value)
	{
		sf::Font font;

		// TODO: fix font handling, make variable to cache font
		std::string fontFile{ "assets/tuffy.ttf" };
		if (!font.openFromFile(fontFile))
		{
			std::cout << "error: failed to load font: " << fontFile << std::endl;
		}

		object.spriteParams.at(1) = formatDisplayNumber(value);

		sf::Text text(font);
		text.setString(object.spriteParams.at(1));
		text.setCharacterSize(std::stoi(object.spriteParams.at(2)));
		text.setFillColor(sfmlColor(object.spriteParams.at(3)));

		const int width = static_cast<int>(std::ceil(text.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(text.getLocalBounds().size.y));

		text.setPosition({ -text.getLocalBounds().position.x, -text.getLocalBounds().position.y });

		object.renderTexture = std::make_unique<sf::RenderTexture>();
		if (!object.renderTexture->resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		object.renderTexture->draw(text);
		object.renderTexture->display();

		object.sprite = std::make_unique<sf::Sprite>(object.renderTexture->getTexture());
		object.sprite->setPosition(object.position);
	}

	void game_sfml::createCircle(Object& object)
	{
		sf::CircleShape circle;

		const float radius = std::stof(object.spriteParams.at(1));

		circle.setRadius(radius);
		circle.setFillColor(sfmlColor(object.spriteParams.at(3)));

		const int width = static_cast<int>(std::ceil(circle.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(circle.getLocalBounds().size.y));

		if (!object.renderTexture->resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		object.renderTexture->draw(circle);
	}

	void game_sfml::createRectangle(Object& object)
	{
		sf::RectangleShape rectangle;

		const float recWidth = std::stof(object.spriteParams.at(1));
		const float recHeight = std::stof(object.spriteParams.at(2));

		rectangle.setSize(sf::Vector2f(recWidth, recHeight));
		rectangle.setFillColor(sfmlColor(object.spriteParams.at(3)));

		const int width = static_cast<int>(std::ceil(rectangle.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(rectangle.getLocalBounds().size.y));

		if (!object.renderTexture->resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		object.renderTexture->draw(rectangle);
	}

	void game_sfml::createText(Object& object)
	{
		sf::Font font;

		// TODO: fix font handling, make variable to cache font
		std::string fontFile{ "assets/tuffy.ttf" };
		if (!font.openFromFile(fontFile))
		{
			std::cout << "error: failed to load font: " << fontFile << std::endl;
		}

		sf::Text text(font);
		text.setString(object.spriteParams.at(1));
		text.setCharacterSize(std::stoi((object.spriteParams.at(2))));
		text.setFillColor(sfmlColor(object.spriteParams.at(3)));
		text.setPosition({ -text.getLocalBounds().position.x, -text.getLocalBounds().position.y });

		const int width = static_cast<int>(std::ceil(text.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(text.getLocalBounds().size.y));

		if (!object.renderTexture->resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		object.renderTexture->draw(text);
	}

	void game_sfml::createImage(Object& object)
	{
		sf::Texture texture;

		auto& imageFile = object.spriteParams.at(1);

		if (!texture.loadFromFile(imageFile))
		{
			std::cout << "error: SFML Image: failed to load " << imageFile << '\n';
			exit(EXIT_FAILURE);
		}

		sf::Sprite sprite(texture);

		const int width = static_cast<int>(std::ceil(sprite.getLocalBounds().size.x));
		const int height = static_cast<int>(std::ceil(sprite.getLocalBounds().size.y));

		if (object.spriteParams.at(2) == "flip.horizontal")
		{
			sprite.setTextureRect(sf::IntRect({ width, 0 }, { -width, height }));
		}
		else if (object.spriteParams.at(2) == "flip.vertical")
		{
			sprite.setTextureRect(sf::IntRect({ 0, height }, { width, -height }));
		}

		if (!object.renderTexture->resize({ static_cast<unsigned int>(width), static_cast<unsigned int>(height) }))
		{
			std::cout << "error: failed to resize render texture" << std::endl;
		}
		object.renderTexture->draw(sprite);
	}

	// TODO: remove once combined with setGridXY() in game_expr
	GridData game_sfml::setGridData(Object& object)
	{
		GridData gridData;
		auto& spriteParams = object.spriteParams;

		if (spriteParams.size() > 5 && spriteParams.at(4) == "grid")
		{
			gridData.padding.x = std::stoi(spriteParams.at(7));
			gridData.padding.y = std::stoi(spriteParams.at(8));

			gridData.obj.x = object.renderTexture->getSize().x;
			gridData.obj.y = object.renderTexture->getSize().y;
		}

		return gridData;
	}
}
