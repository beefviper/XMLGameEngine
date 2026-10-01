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

TEST_CASE("a size becomes a whole-number scale of the 8 pixel glyphs", "[builtin_font]")
{
	CHECK(builtinFontScale(0) == 1);
	CHECK(builtinFontScale(-5) == 1);
	CHECK(builtinFontScale(3) == 1);
	CHECK(builtinFontScale(11) == 1);
	CHECK(builtinFontScale(12) == 2);
	CHECK(builtinFontScale(24) == 3);
	CHECK(builtinFontScale(48) == 6);
	CHECK(builtinFontScale(128) == 16);
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

TEST_CASE("a bigger size makes every glyph pixel a square block", "[builtin_font]")
{
	const Bitmap small = rasterizeText("A", 8, kWhite);
	const Bitmap big = rasterizeText("A", 48, kWhite); // scale 6

	REQUIRE(big.width == 48);
	REQUIRE(big.height == 48);
	CHECK(pixelsDrawn(big) == pixelsDrawn(small) * 36);

	// Each pixel of the small glyph is a 6 by 6 block of the big one.
	for (int y = 0; y < 8; ++y)
	{
		for (int x = 0; x < 8; ++x)
		{
			const bool expected = small.solidAt(x, y);
			CHECK(big.solidAt(x * 6, y * 6) == expected);
			CHECK(big.solidAt(x * 6 + 5, y * 6 + 5) == expected);
		}
	}
}

TEST_CASE("every character takes one cell, and a newline starts another line", "[builtin_font]")
{
	const Bitmap word = rasterizeText("HELLO", 16, kWhite); // scale 2: cells of 16
	CHECK(word.width == 5 * 16);
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
