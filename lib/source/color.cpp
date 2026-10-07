// color.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "color.h"

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

		constexpr std::array<NamedColor, 17> namedColors{ {
			{ "color.black",   { 0, 0, 0, 255 } },
			{ "color.white",   { 255, 255, 255, 255 } },
			{ "color.red",     { 255, 0, 0, 255 } },
			{ "color.green",   { 0, 255, 0, 255 } },
			{ "color.blue",    { 0, 0, 255, 255 } },
			{ "color.yellow",  { 255, 255, 0, 255 } },
			{ "color.magenta", { 255, 0, 255, 255 } },
			{ "color.cyan",    { 0, 255, 255, 255 } },

			// Muted colors for backgrounds and scenery, beside the bright ones above.
			{ "color.grey",        { 128, 128, 128, 255 } },
			{ "color.darkgrey",    { 64, 64, 64, 255 } },
			{ "color.lightgrey",   { 192, 192, 192, 255 } },
			{ "color.brown",       { 139, 69, 19, 255 } },
			{ "color.orange",      { 255, 165, 0, 255 } },
			{ "color.purple",      { 128, 0, 128, 255 } },
			{ "color.darkblue",    { 0, 0, 139, 255 } },
			{ "color.darkgreen",   { 0, 100, 0, 255 } },
			{ "color.forestgreen", { 34, 139, 34, 255 } },
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
