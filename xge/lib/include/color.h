// color.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace xge
{
	// Backend-agnostic RGBA color. Every backend converts one of these to its
	// own native color type at draw time (a one-line conversion - see
	// window_sfml.cpp / window_raylib.cpp / window_sdl2.cpp). The name ->
	// RGBA mapping itself isn't backend-specific (every backend agrees on
	// what "color.red" means), so it lives here once instead of being
	// duplicated three times - this used to be SFML-only, as utils.h's
	// sfmlColor().
	struct Color
	{
		std::uint8_t r{};
		std::uint8_t g{};
		std::uint8_t b{};
		std::uint8_t a{ 255 };
	};

	// Matches a sprite's color argument (e.g. "color.black"). Unrecognized
	// names come back fully transparent, same as the old sfmlColor()'s
	// sf::Color::Transparent default.
	Color colorFromName(const std::string& name) noexcept;

	// Whether a name is one of the colors above, and all of their names, for
	// the loader to refuse any other with a list of what there is.
	bool isColorName(const std::string& name) noexcept;
	std::vector<std::string> colorNames();
}
