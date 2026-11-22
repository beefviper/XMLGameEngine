// color.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "picture/color.h"

#include <algorithm>
#include <array>

namespace xge
{
	namespace
	{
		struct NamedColor
		{
			const char* name;
			Color color;
		};

		constexpr std::array<NamedColor, 19> namedColors{ {
			{ "black",   { 0, 0, 0, 255 } },
			{ "white",   { 255, 255, 255, 255 } },
			{ "red",     { 255, 0, 0, 255 } },
			{ "green",   { 0, 255, 0, 255 } },
			{ "blue",    { 0, 0, 255, 255 } },
			{ "yellow",  { 255, 255, 0, 255 } },
			{ "magenta", { 255, 0, 255, 255 } },
			{ "cyan",    { 0, 255, 255, 255 } },

			// Muted colors for backgrounds and scenery, beside the bright ones above.
			{ "grey",        { 128, 128, 128, 255 } },
			{ "darkgrey",    { 64, 64, 64, 255 } },
			{ "lightgrey",   { 192, 192, 192, 255 } },
			{ "brown",       { 139, 69, 19, 255 } },
			{ "orange",      { 255, 165, 0, 255 } },
			{ "purple",      { 128, 0, 128, 255 } },
			{ "darkblue",    { 0, 0, 139, 255 } },
			{ "darkgreen",   { 0, 100, 0, 255 } },
			{ "forestgreen", { 34, 139, 34, 255 } },
			{ "gold",        { 230, 180, 30, 255 } },
			{ "peach",       { 255, 214, 170, 255 } },
		} };
	}

	Color colorFromName(const std::string& name) noexcept
	{
		const auto found = std::find_if(namedColors.begin(), namedColors.end(), [&](const NamedColor& named) { return name == named.name; });
		return found == namedColors.end() ? Color{ 0, 0, 0, 0 } : found->color;
	}

	bool isColorName(const std::string& name) noexcept
	{
		return std::any_of(namedColors.begin(), namedColors.end(), [&](const NamedColor& named) { return name == named.name; });
	}

	std::vector<std::string> colorNames()
	{
		std::vector<std::string> names;
		for (const NamedColor& named : namedColors) { names.emplace_back(named.name); }
		return names;
	}
}
