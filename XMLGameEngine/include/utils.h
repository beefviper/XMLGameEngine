// utils.h
// XML Game Engine
// author: beefviper
// date: Mar 7, 2021

#pragma once

// disable warnings generated by SFML
#pragma warning(push)
#pragma warning(disable : 26812)

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>

#pragma warning(pop)

#include <string>

namespace xge
{
	sf::Color sfmlColor(std::string color);
	std::string sfmlKeyToString(sf::Keyboard::Key key);
}
