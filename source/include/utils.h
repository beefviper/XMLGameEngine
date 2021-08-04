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
	sf::Color sfmlColor(std::string color);
	std::string sfmlKeyToString(sf::Keyboard::Key key);
}
