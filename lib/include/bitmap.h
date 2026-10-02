// bitmap.h
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#pragma once

#include "color.h"

#include <cstdint>
#include <memory>
#include <string>
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

	// A picture written as rows of text, one character to a pixel: '.' is a
	// clear (transparent) pixel and '*' a solid one, drawn in `color`. Every
	// character becomes a `scale` by `scale` block of real pixels, so a handful
	// of characters makes a chunky sprite. The bitmap is as wide as the rows and
	// as tall as there are rows, times scale; rows are measured from the top
	// left, as a sprite of lines is. Throws std::invalid_argument, saying which
	// row, for no rows, a row with nothing in it, rows of different lengths, a
	// character other than '.' and '*', or a scale under 1.
	Bitmap rasterizeRows(const std::vector<std::string>& rows, int scale, const Color& color);

	// A turned drawing is made when it is wanted, at a whole number of degrees
	// from 0 up to (not including) 360, clockwise: 0 is the drawing as written
	// (its "up"), 90 has it turned a quarter of the way round, and so on. Any
	// number of degrees is taken round to the nearest whole one and wrapped
	// into that range. See Turnable.

	// The drawing of lines turned about the middle of the box its line ends lie
	// in. Every heading comes out the same square size, big enough for the
	// drawing at any heading, with the middle of the drawing at the middle of
	// the square, so an object that turns stays where it is and keeps its
	// size. Returns an empty bitmap for no lines, and throws
	// std::invalid_argument for a negative coordinate as rasterizeLines does.
	Bitmap rasterizeTurned(const std::vector<LineSegment>& lines, float degrees);

	// A finished picture turned the same way. Each pixel of the result takes
	// the colour of the one pixel of the original that lies under it (no
	// blending), so chunky pixel art stays chunky and what is drawn is exactly
	// what a pixel collision tests. The result is the one square size for
	// every heading, with the middle of the picture at the middle of the
	// square. Returns an empty bitmap for an empty picture.
	Bitmap turnBitmap(const Bitmap& picture, float degrees);

	// What an object that turns is drawn from, kept once and shared by every
	// object made from one definition: either a drawing of lines or a picture.
	// An object keeps just this and the one picture it shows now, and asks for
	// a new one only when its heading, rounded to a whole degree, changes.
	struct Turnable
	{
		std::vector<LineSegment> lines;
		std::shared_ptr<const Bitmap> picture;

		// The drawing at the heading: the lines turned if there are lines,
		// otherwise the picture turned.
		Bitmap at(float degrees) const
		{
			return lines.empty() ? turnBitmap(*picture, degrees) : rasterizeTurned(lines, degrees);
		}
	};
}
