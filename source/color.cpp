// color.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "color.h"

namespace xge
{
	Color colorFromName(const std::string& name) noexcept
	{
		if (name == "color.black")   { return { 0, 0, 0, 255 }; }
		if (name == "color.white")   { return { 255, 255, 255, 255 }; }
		if (name == "color.red")     { return { 255, 0, 0, 255 }; }
		if (name == "color.green")   { return { 0, 255, 0, 255 }; }
		if (name == "color.blue")    { return { 0, 0, 255, 255 }; }
		if (name == "color.yellow")  { return { 255, 255, 0, 255 }; }
		if (name == "color.magenta") { return { 255, 0, 255, 255 }; }
		if (name == "color.cyan")    { return { 0, 255, 255, 255 }; }

		// Muted colors for backgrounds and scenery, beside the bright ones above.
		if (name == "color.grey")        { return { 128, 128, 128, 255 }; }
		if (name == "color.darkgrey")    { return { 64, 64, 64, 255 }; }
		if (name == "color.lightgrey")   { return { 192, 192, 192, 255 }; }
		if (name == "color.brown")       { return { 139, 69, 19, 255 }; }
		if (name == "color.orange")      { return { 255, 165, 0, 255 }; }
		if (name == "color.purple")      { return { 128, 0, 128, 255 }; }
		if (name == "color.darkblue")    { return { 0, 0, 139, 255 }; }
		if (name == "color.darkgreen")   { return { 0, 100, 0, 255 }; }
		if (name == "color.forestgreen") { return { 34, 139, 34, 255 }; }

		return { 0, 0, 0, 0 };
	}
}
