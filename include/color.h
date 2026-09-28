// color.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <cstdint>
#include <string>

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
}
