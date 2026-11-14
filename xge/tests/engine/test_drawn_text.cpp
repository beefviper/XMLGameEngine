// test_drawn_text.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026
//
// Catch2 tests for text drawn by the engine (drawText, picture/font.cpp):
// every window shows this bitmap, so a text's size is known with no window
// at all, and is the same whichever library draws the window.

#include "picture/font.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace xge;

namespace
{
	const Color white{ 255, 255, 255, 255 };
	const std::string pixelFont{ "assets/pressstart2p.ttf" };
	const std::string smoothFont{ "assets/tuffy.ttf" };

	int columnsInked(const Bitmap& bitmap, int x)
	{
		int inked = 0;
		for (int y = 0; y < bitmap.height; ++y) { inked += bitmap.solidAt(x, y) ? 1 : 0; }
		return inked;
	}
}

TEST_CASE("a text is measured with no window, by the box its ink covers", "[text]")
{
	// Press Start 2P: every character is 8 pixels to the em at 8, its ink at
	// most 7 high; capitals stand 7 high with no tail.
	const Bitmap score = drawText("SCORE", 16, white, pixelFont);
	CHECK(score.width == 5 * 16 - 2);
	CHECK(score.height == 14);

	// Twice the size, twice the box.
	const Bitmap bigger = drawText("SCORE", 32, white, pixelFont);
	CHECK(bigger.width == 2 * score.width);
	CHECK(bigger.height == 2 * score.height);

	// The same text in the same font is the same picture every time.
	CHECK(drawText("SCORE", 16, white, pixelFont).rgba == score.rgba);
}

TEST_CASE("a text's ink touches every side of its box", "[text]")
{
	const Bitmap pong = drawText("Pong", 48, white, smoothFont);
	REQUIRE(pong.width > 0);
	CHECK(columnsInked(pong, 0) > 0);
	CHECK(columnsInked(pong, pong.width - 1) > 0);
	bool top = false;
	bool bottom = false;
	for (int x = 0; x < pong.width; ++x)
	{
		top = top || pong.solidAt(x, 0);
		bottom = bottom || pong.solidAt(x, pong.height - 1);
	}
	CHECK(top);
	CHECK(bottom);
}

TEST_CASE("spaces at the ends of a text count across, and a pixel font stays crisp", "[text]")
{
	const Bitmap plain = drawText("A", 16, white, pixelFont);
	const Bitmap spaced = drawText(" A ", 16, white, pixelFont);
	CHECK(spaced.width == plain.width + 2 * 16 + 2);
	CHECK(spaced.height == plain.height);
	CHECK(columnsInked(spaced, 0) == 0);

	// At a multiple of 8 every glyph edge lies on a whole pixel: no pixel
	// is part covered.
	for (std::size_t at = 3; at < plain.rgba.size(); at += 4)
	{
		CHECK((plain.rgba[at] == 0 || plain.rgba[at] == 255));
	}
}

TEST_CASE("a text of lines is a line height apart, and an empty one is nothing", "[text]")
{
	const Bitmap one = drawText("AB", 16, white, pixelFont);
	const Bitmap two = drawText("AB\nAB", 16, white, pixelFont);
	CHECK(two.width == one.width);
	CHECK(two.height == one.height + 16);

	const Bitmap none = drawText("", 16, white, pixelFont);
	CHECK(none.width == 0);
	CHECK(none.rgba.empty());
}

TEST_CASE("a font file that is missing draws in the built-in font", "[text]")
{
	const Bitmap missing = drawText("HI", 16, white, "assets/no-such-font.ttf");
	const Bitmap builtin = rasterizeText("HI", 16, white);
	CHECK(missing.width == builtin.width);
	CHECK(missing.rgba == builtin.rgba);
}
