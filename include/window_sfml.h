// window_sfml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window.h"

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

namespace xge
{
	// The only Window backend today: wraps a real sf::RenderWindow. Nothing
	// outside this file, window_sfml.cpp, and WindowFactory::create (which
	// is the one place that constructs one) ever names an SFML type - see
	// window.h.
	class SFMLWindow : public Window
	{
	public:
		explicit SFMLWindow(const WindowDesc& windowDesc);

		bool isOpen() const override;
		void close() override;
		std::vector<std::pair<std::string, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(const Object& object) override;
		void display() override;

	private:
		sf::RenderWindow window;
	};
}
