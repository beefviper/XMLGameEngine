// test_builtin_font.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for the font the engine carries inside itself
// (lib/source/builtin_font.cpp), which the window backends draw text with when
// a game's font file cannot be found: its glyphs, the whole-number scaling,
// lines, and what happens to characters it does not have.

#include "builtin_font.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <string>

using namespace xge;

namespace
{
	const Color kWhite{ 255, 255, 255, 255 };

	// One row of a bitmap as text: '#' where something is drawn, '.' where not.
	std::string rowOf(const Bitmap& bitmap, int row, int fromColumn = 0, int columns = -1)
	{
		const int end = columns < 0 ? bitmap.width : fromColumn + columns;

		std::string text;
		for (int x = fromColumn; x < end; ++x)
		{
			text += bitmap.solidAt(x, row) ? '#' : '.';
		}
		return text;
	}

	int pixelsDrawn(const Bitmap& bitmap)
	{
		int count = 0;
		for (int y = 0; y < bitmap.height; ++y)
		{
			for (int x = 0; x < bitmap.width; ++x)
			{
				if (bitmap.solidAt(x, y)) { ++count; }
			}
		}
		return count;
	}
}

TEST_CASE("a size becomes a whole-number scale of the 8 pixel glyphs, about half of it", "[builtin_font]")
{
	// About half the size: a size of 16 is 1x, 32 is 2x, 48 is 3x.
	CHECK(builtinFontScale(0) == 1);
	CHECK(builtinFontScale(-5) == 1);
	CHECK(builtinFontScale(8) == 1);
	CHECK(builtinFontScale(23) == 1);
	CHECK(builtinFontScale(24) == 2);
	CHECK(builtinFontScale(32) == 2);
	CHECK(builtinFontScale(48) == 3);
	CHECK(builtinFontScale(64) == 4);
	CHECK(builtinFontScale(128) == 8);
}

TEST_CASE("a cell is as tall as the scale says and half as wide, never under 8", "[builtin_font]")
{
	// size   scale   cell (width x height)
	//    8     1       8 x 8
	//   16     1       8 x 8
	//   32     2       8 x 16
	//   48     3       12 x 24
	//   64     4       16 x 32
	//  128     8       32 x 64
	CHECK(builtinCellWidth(8) == 8);
	CHECK(builtinCellHeight(8) == 8);
	CHECK(builtinCellWidth(16) == 8);
	CHECK(builtinCellHeight(16) == 8);
	CHECK(builtinCellWidth(32) == 8);
	CHECK(builtinCellHeight(32) == 16);
	CHECK(builtinCellWidth(48) == 12);
	CHECK(builtinCellHeight(48) == 24);
	CHECK(builtinCellWidth(64) == 16);
	CHECK(builtinCellHeight(64) == 32);
	CHECK(builtinCellWidth(128) == 32);
	CHECK(builtinCellHeight(128) == 64);
}

TEST_CASE("a letter is drawn as its eight by eight glyph, leftmost pixel first", "[builtin_font]")
{
	const Bitmap a = rasterizeText("A", 8, kWhite);

	REQUIRE(a.width == 8);
	REQUIRE(a.height == 8);

	// The glyph for A: 0x0C 0x1E 0x33 0x33 0x3F 0x33 0x33 0x00, lowest bit left.
	CHECK(rowOf(a, 0) == "..##....");
	CHECK(rowOf(a, 1) == ".####...");
	CHECK(rowOf(a, 2) == "##..##..");
	CHECK(rowOf(a, 3) == "##..##..");
	CHECK(rowOf(a, 4) == "######..");
	CHECK(rowOf(a, 5) == "##..##..");
	CHECK(rowOf(a, 6) == "##..##..");
	CHECK(rowOf(a, 7) == "........");
}

TEST_CASE("only the pixels of the glyph are drawn, in the colour asked for", "[builtin_font]")
{
	const Bitmap bitmap = rasterizeText("I", 8, Color{ 10, 20, 30, 200 });

	// The top row of an I is ".####...": pixels 1 to 4.
	const auto at = [&](int x, int y) { return static_cast<std::size_t>((y * bitmap.width + x) * 4); };

	CHECK(bitmap.rgba[at(1, 0) + 0] == 10);
	CHECK(bitmap.rgba[at(1, 0) + 1] == 20);
	CHECK(bitmap.rgba[at(1, 0) + 2] == 30);
	CHECK(bitmap.rgba[at(1, 0) + 3] == 200);

	// Nothing drawn is fully transparent.
	CHECK(bitmap.rgba[at(0, 0) + 3] == 0);
	CHECK(!bitmap.solidAt(7, 7));
}

TEST_CASE("a bigger size makes every glyph pixel a block that is taller than it is wide", "[builtin_font]")
{
	const Bitmap small = rasterizeText("A", 8, kWhite);
	const Bitmap big = rasterizeText("A", 48, kWhite); // cell 12 wide, 24 tall

	REQUIRE(big.width == 12);
	REQUIRE(big.height == 24);

	// Each pixel of the small glyph is 3 pixels tall, and 12 / 8 = 1.5 wide: a
	// column at x spans from x * 12 / 8 up to (x + 1) * 12 / 8, so the
	// columns alternate between 1 and 2 pixels and add up to the cell.
	int widest = 0;
	int narrowest = 99;
	for (int x = 0; x < 8; ++x)
	{
		const int left = x * 12 / 8;
		const int right = (x + 1) * 12 / 8;
		widest = std::max(widest, right - left);
		narrowest = std::min(narrowest, right - left);

		for (int y = 0; y < 8; ++y)
		{
			const bool expected = small.solidAt(x, y);
			for (int blockX = left; blockX < right; ++blockX)
			{
				CHECK(big.solidAt(blockX, y * 3) == expected);
				CHECK(big.solidAt(blockX, y * 3 + 2) == expected);
			}
		}
	}
	CHECK(narrowest == 1);
	CHECK(widest == 2);

	// At a size where the width is a whole multiple, the blocks are even:
	// 64 is a cell 16 wide, so every glyph pixel is 2 wide and 4 tall.
	const Bitmap even = rasterizeText("A", 64, kWhite);
	REQUIRE(even.width == 16);
	REQUIRE(even.height == 32);
	CHECK(pixelsDrawn(even) == pixelsDrawn(small) * 8);
}

TEST_CASE("every character takes one cell, and a newline starts another line", "[builtin_font]")
{
	const Bitmap word = rasterizeText("HELLO", 32, kWhite); // scale 2: cells 8 wide, 16 tall
	CHECK(word.width == 5 * 8);
	CHECK(word.height == 16);

	const Bitmap lines = rasterizeText("AB\nCDEF\nG", 8, kWhite);
	CHECK(lines.width == 4 * 8);  // the longest line
	CHECK(lines.height == 3 * 8);

	// The second line starts below the first: C is drawn in its first cell.
	const Bitmap c = rasterizeText("C", 8, kWhite);
	for (int y = 0; y < 8; ++y)
	{
		CHECK(rowOf(lines, 8 + y, 0, 8) == rowOf(c, y));
	}

	// Text with a tab or a carriage return still measures sensibly.
	CHECK(rasterizeText("A\tB", 8, kWhite).width == 3 * 8);
	CHECK(rasterizeText("A\r\nB", 8, kWhite).height == 2 * 8);
}

TEST_CASE("empty text is an empty picture", "[builtin_font]")
{
	CHECK(rasterizeText("", 24, kWhite).rgba.empty());
	CHECK(rasterizeText("", 24, kWhite).width == 0);
	CHECK(rasterizeText("\n\n", 24, kWhite).rgba.empty());
}

TEST_CASE("a character the font does not have is drawn as a question mark", "[builtin_font]")
{
	const Bitmap question = rasterizeText("?", 8, kWhite);

	CHECK(rowOf(rasterizeText("\x01", 8, kWhite), 0) == rowOf(question, 0));

	// A multi-byte UTF-8 character is one question mark, not one per byte.
	const Bitmap accented = rasterizeText("\xC3\xA9", 8, kWhite); // e with an acute accent
	REQUIRE(accented.width == 8);
	for (int y = 0; y < 8; ++y)
	{
		CHECK(rowOf(accented, y) == rowOf(question, y));
	}

	const Bitmap emoji = rasterizeText("\xF0\x9F\x98\x80", 8, kWhite);
	CHECK(emoji.width == 8);
}

TEST_CASE("every printable character has a glyph, and only the space is blank", "[builtin_font]")
{
	for (int character = 33; character <= 126; ++character)
	{
		INFO("character " << character << " '" << static_cast<char>(character) << "'");
		CHECK(pixelsDrawn(rasterizeText(std::string(1, static_cast<char>(character)), 8, kWhite)) > 0);
	}

	const Bitmap space = rasterizeText(" ", 8, kWhite);
	REQUIRE(space.width == 8);
	CHECK(pixelsDrawn(space) == 0);

	// Different characters look different.
	CHECK(rowOf(rasterizeText("O", 8, kWhite), 3) != rowOf(rasterizeText("0", 8, kWhite), 3));
	CHECK(rasterizeText("l", 8, kWhite).rgba != rasterizeText("I", 8, kWhite).rgba);
}

TEST_CASE("tails go below the line in the last row, capitals leave it empty", "[builtin_font]")
{
	for (const char tail : { 'g', 'j', 'p', 'q', 'y', '_' })
	{
		INFO(tail);
		CHECK(rowOf(rasterizeText(std::string(1, tail), 8, kWhite), 7).find('#') != std::string::npos);
	}

	for (const char flat : { 'A', 'E', 'H', 'a', 'e', 'o', '0', '9' })
	{
		INFO(flat);
		CHECK(rowOf(rasterizeText(std::string(1, flat), 8, kWhite), 7).find('#') == std::string::npos);
	}
}
