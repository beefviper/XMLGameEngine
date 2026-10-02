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
	// negative (std::invalid_argument). minWidth and minHeight make it at
	// least that big (the lines stay where they are, the rest is transparent).
	Bitmap rasterizeLines(const std::vector<LineSegment>& lines, int minWidth = 0, int minHeight = 0);

	// How many headings a turning object's drawing is kept at: 72 is every five
	// degrees. See rasterizeTurned.
	inline constexpr int headingSteps = 72;

	// The same drawing turned, about the middle of the box its line ends lie
	// in, to `steps` evenly spaced headings, clockwise: bitmap 0 is the
	// drawing as written (its "up"), bitmap steps / 4 has it turned a quarter
	// of the way round, and so on. All of them are the same square size, big
	// enough for the drawing at any heading, with the middle of the drawing at
	// the middle of the square, so an object that turns stays where it is and
	// keeps its size. Returns nothing for no lines, and throws
	// std::invalid_argument for a negative coordinate as rasterizeLines does.
	std::vector<Bitmap> rasterizeTurned(const std::vector<LineSegment>& lines, int steps = headingSteps);
}
