// bitmap.h
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#pragma once

#include "color.h"

#include <cstdint>
#include <vector>

namespace xge
{
	// A picture the engine draws itself, pixel by pixel, with no window or
	// graphics library involved: what a sprite made of <line>s is turned into
	// when the game loads. The backends upload `rgba` as a texture, and a
	// collision of type "pixel" looks at the same pixels, so what is drawn is
	// exactly what is tested. A pixel nothing was drawn on is fully transparent.
	struct Bitmap
	{
		int width{};
		int height{};

		// width * height pixels, row by row from the top left, four bytes each:
		// red, green, blue, alpha.
		std::vector<std::uint8_t> rgba;

		// Whether something was drawn on this pixel. Outside the bitmap is empty.
		bool solidAt(int x, int y) const noexcept
		{
			if (x < 0 || y < 0 || x >= width || y >= height) { return false; }
			return rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4 + 3] != 0;
		}
	};

	// One straight line to draw, in pixels from the top left of the sprite.
	struct LineSegment
	{
		float x1{};
		float y1{};
		float x2{};
		float y2{};
		Color color;

		// How many pixels wide, at least 1. A line is drawn by stamping a
		// square of this size along it, so from 2 up no two lines can cross
		// without sharing a pixel (a one-pixel line can slip between the
		// pixels of another at a slant, which matters to a pixel collision).
		int thickness{ 1 };
	};

	// Draws the lines, in the order given (a later one covers an earlier one
	// where they meet), onto a transparent bitmap just large enough to hold
	// them: it reaches the right-most and bottom-most point plus the
	// thickness, and begins at 0, 0, so a sprite's own top left is where its
	// lines' coordinates are measured from. Endpoints are rounded to whole
	// pixels. Returns an empty bitmap for no lines. Coordinates must not be
	// negative (std::invalid_argument).
	Bitmap rasterizeLines(const std::vector<LineSegment>& lines);
}
