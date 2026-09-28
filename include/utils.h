// utils.h
// XML Game Engine
// author: beefviper
// date: Mar 7, 2021

#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <string>

namespace xge
{
	// TODO: delete utils, move functions to places they're needed
	// move sfmlColor to game_sfml
	sf::Color sfmlColor(std::string color) noexcept;
	// move sfmlKeyToString to window_sfml - its only caller now that Engine
	// itself no longer touches any SFML type directly (see window.h/window_sfml.h)
	std::string sfmlKeyToString(sf::Keyboard::Key key);
}
