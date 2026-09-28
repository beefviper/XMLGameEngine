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

		return { 0, 0, 0, 0 };
	}
}
