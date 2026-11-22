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
	// A colour in no library's type; each window converts it as it draws.
	struct Color
	{
		std::uint8_t r{};
		std::uint8_t g{};
		std::uint8_t b{};
		std::uint8_t a{ 255 };
	};

	// "black" and the rest; any other name is transparent.
	Color colorFromName(const std::string& name) noexcept;

	// Whether a name is one of the colors above, and all of their names, for
	// the loader to refuse any other with a list of what there is.
	bool isColorName(const std::string& name) noexcept;
	std::vector<std::string> colorNames();
}
