// svg.h
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#pragma once

#include "bitmap.h"

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

	// Draws an SVG file into a Bitmap, with no window or graphics library
	// involved, so every backend shows an SVG sprite the way it shows any
	// other picture the engine drew itself, and a collision of type "pixel"
	// tests the pixels that are shown. This is the one place the engine uses
	// its SVG library (lunasvg); nothing outside svg.cpp includes it.
	//
	// `region` is the part of the drawing to take (all of it if it is whole),
	// and `scale` is how many real pixels one unit of the drawing is, so a
	// 32 unit square at a scale of 2 is a 64 pixel bitmap (it is the region
	// size times the scale, rounded up to whole pixels). The pixels keep their
	// own colors and transparency, antialiased, with the alpha not
	// premultiplied; whatever the drawing leaves empty is transparent.
	//
	// `hide` lists the ids of elements to leave out, for a sheet of sprites
	// that carries a backdrop or a grid of its own; an id nothing has is
	// simply ignored.
	//
	// Throws std::runtime_error, saying which file, if it cannot be read or is
	// not an SVG, and std::invalid_argument for a scale that is not above 0 or
	// a region with no width or height (or a negative one, or one with no part
	// inside the drawing).
	Bitmap rasterizeSvg(const std::string& path, const SvgRegion& region, float scale, const std::vector<std::string>& hide = {});
}
