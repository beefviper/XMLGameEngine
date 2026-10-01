// builtin_font.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "bitmap.h"
#include "color.h"

#include <string>

namespace xge
{
	// The font the engine carries inside itself, for when a game's own font
	// file cannot be found (see the window backends): an 8 by 8 pixel glyph
	// for each printable ASCII character, stored in builtin_font.cpp, so text
	// can always be drawn, with no file to lose.
	constexpr int kBuiltinGlyphSize = 8;

	// Draws `text` onto a transparent Bitmap in `color`, each glyph scaled by a
	// whole number so the pixels stay square and crisp: the scale is the
	// requested `size` (the same number a <text> sprite's <size> gives)
	// divided by 8 and rounded, never less than 1. Every character takes a
	// cell of 8 * scale pixels, so a line is characters * 8 * scale wide and
	// each line 8 * scale tall; a '\n' starts a new line. A character the font
	// does not have (anything but printable ASCII, a multi-byte UTF-8
	// character counting as one) is drawn as '?'. Returns an empty Bitmap for
	// empty text.
	Bitmap rasterizeText(const std::string& text, int size, const Color& color);

	// The scale rasterizeText() would use for a size: size / 8, rounded, at least 1.
	int builtinFontScale(int size) noexcept;
}
