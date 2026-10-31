// bitmap.h
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#pragma once

#include "picture/color.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <map>
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

		// Which of the drawn pixels a collision of type pixel touches with,
		// one to a pixel, row by row (1: it touches): only the characters a
		// <solid char> names, of a bitmap's rows. Empty: every drawn pixel.
		std::vector<std::uint8_t> touching;

		// Whether something was drawn on this pixel. Outside the bitmap is empty.
		bool solidAt(int x, int y) const noexcept
		{
			if (x < 0 || y < 0 || x >= width || y >= height) { return false; }
			return rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4 + 3] != 0;
		}

		// Whether a pixel collision touches with this pixel: drawn on, and
		// among those `touching` names.
		bool touchesAt(int x, int y) const noexcept
		{
			if (!solidAt(x, y)) { return false; }
			return touching.empty() || touching[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] != 0;
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

	// The same, with more colors than one: a character `inks` has a color for
	// (a bitmap's <color char="w">) is a solid pixel of that color, so a
	// ghost's eyes are white on its red body. '.' is still clear and '*' still
	// `color`; any character neither of those nor in `inks` is the error.
	Bitmap rasterizeRows(const std::vector<std::string>& rows, int scale, const Color& color, const std::map<char, Color>& inks);

	// The same, touching (Bitmap::touching) only with the pixels of the
	// characters in `solid`; empty, with all of them. A character of `solid`
	// that is neither '*' nor in `inks` is the error.
	Bitmap rasterizeRows(const std::vector<std::string>& rows, int scale, const Color& color, const std::map<char, Color>& inks, const std::string& solid);

	// The picture mirrored: left for right (horizontal), top for bottom
	// (vertical), or both. The same size, each pixel exactly as it was, only
	// moved, so a flipped picture is as sharp as the original and a pixel
	// collision tests the flipped pixels.
	Bitmap flipBitmap(const Bitmap& picture, bool horizontal, bool vertical);

	// A turned drawing is made when it is wanted, at a whole number of degrees
	// from 0 up to (not including) 360, clockwise: 0 is the drawing as written
	// (its "up"), 90 has it turned a quarter of the way round, and so on. Any
	// number of degrees is taken round to the nearest whole one and wrapped
	// into that range (wholeDegrees). See Turnable.
	int wholeDegrees(float degrees) noexcept;

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

		// The drawing at heading 0: the picture, or the lines drawn. What a
		// sweep through a turn asks about a point at any heading (touchesAt).
		std::shared_ptr<const Bitmap> upright;

		// Whether the drawing turned to `degrees` (not rounded), drawn in a
		// square `side` big, touches with the point x, y from its top left:
		// the point turned back onto the upright drawing, as turnBitmap does.
		bool touchesAt(float degrees, float side, float x, float y) const noexcept
		{
			if (!upright) { return false; }
			const float angle = degrees * 3.14159265358979323846f / 180.0f;
			const float c = std::cos(angle);
			const float s = std::sin(angle);
			const float dx = x - side / 2.0f;
			const float dy = y - side / 2.0f;
			const int sx = static_cast<int>(std::floor(dx * c + dy * s + static_cast<float>(upright->width) / 2.0f + 0.0001f));
			const int sy = static_cast<int>(std::floor(-dx * s + dy * c + static_cast<float>(upright->height) / 2.0f + 0.0001f));
			return upright->touchesAt(sx, sy);
		}

		// The drawing at the heading: the lines turned if there are lines,
		// otherwise the picture turned.
		Bitmap at(float degrees) const
		{
			return lines.empty() ? turnBitmap(*picture, degrees) : rasterizeTurned(lines, degrees);
		}
	};
}
