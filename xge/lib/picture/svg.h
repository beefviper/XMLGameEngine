// svg.h
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#pragma once

#include "picture/bitmap.h"

#include <string>
#include <vector>

namespace xge
{
	// Which part of a drawing to take, in the drawing's own units (the ones
	// its width and height are written in). A region with a width and height
	// of 0 or less means all of it.
	struct SvgRegion
	{
		float x{};
		float y{};
		float width{};
		float height{};

		bool isWhole() const noexcept { return !(width > 0.0f && height > 0.0f); }
	};

	// An SVG drawn into a Bitmap by the engine (lunasvg, used only here), so
	// every window shows it alike and a pixel collision tests what is shown.
	// `scale` is pixels to a unit of the drawing (a 32 unit square at 2 is
	// 64 pixels, rounded up); the alpha is not premultiplied. `hide` names
	// elements to leave out, such as a sheet's backdrop. Throws
	// std::runtime_error for a file that is not a readable SVG, and
	// std::invalid_argument for a scale not above 0 or a region with nothing
	// of the drawing in it.
	Bitmap rasterizeSvg(const std::string& path, const SvgRegion& region, float scale, const std::vector<std::string>& hide = {});

	// An SVG file's width and height, in the units its drawing is written
	// in. Throws std::runtime_error as rasterizeSvg does.
	SvgRegion svgSize(const std::string& path);
}
