// window_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window_sfml.h"

#include "utils.h"

#include <optional>

namespace xge
{
	SFMLWindow::SFMLWindow(const WindowDesc& windowDesc)
	{
		const auto width = static_cast<unsigned int>(windowDesc.width);
		const auto height = static_cast<unsigned int>(windowDesc.height);
		const sf::VideoMode videoMode({ width, height });

		const auto windowState = (windowDesc.fullscreen == "true") ? sf::State::Fullscreen : sf::State::Windowed;

		window.create(videoMode, windowDesc.name, sf::Style::Default, windowState);
		window.setFramerateLimit(windowDesc.framerate);
	}

	bool SFMLWindow::isOpen() const
	{
		return window.isOpen();
	}

	void SFMLWindow::close()
	{
		window.close();
	}

	std::vector<std::pair<std::string, bool>> SFMLWindow::pollEvents()
	{
		std::vector<std::pair<std::string, bool>> events;

		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
			{
				events.emplace_back(sfmlKeyToString(keyPressed->code), true);
			}
			else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
			{
				events.emplace_back(sfmlKeyToString(keyReleased->code), false);
			}
		}

		return events;
	}

	void SFMLWindow::clear(const std::string& colorName)
	{
		window.clear(sfmlColor(colorName));
	}

	void SFMLWindow::draw(const Object& object)
	{
		window.draw(*object.sprite);
	}

	void SFMLWindow::display()
	{
		window.display();
	}
}
