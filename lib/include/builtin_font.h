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

	// Draws `text` onto a transparent Bitmap in `color`. The height and the
	// width are scaled separately, so the letters are taller than wide, like
	// the real font's: the height of a glyph is the scale (see
	// builtinFontScale) times 8 pixels, and the width is half of that, but
	// never fewer than the 8 pixels the glyph is drawn with. Every character
	// takes a cell of builtinCellWidth() by builtinCellHeight() pixels, so a
	// line is characters * that wide and each line that tall; a '\n' starts a
	// new line. Pixels are whole-number blocks, except that where the width is
	// an odd multiple of 4 the columns are alternately a pixel wider. A
	// character the font does not have (anything but printable ASCII, a
	// multi-byte UTF-8 character counting as one) is drawn as '?'. Returns an
	// empty Bitmap for empty text.
	Bitmap rasterizeText(const std::string& text, int size, const Color& color);

	// The vertical scale rasterizeText() uses for a size: size / 16, rounded,
	// at least 1 (the size of a pixel of the glyph, top to bottom).
	int builtinFontScale(int size) noexcept;

	// The width and height of one character's cell for a size: 8 * scale tall,
	// and half that wide (4 * scale), at least 8.
	int builtinCellWidth(int size) noexcept;
	int builtinCellHeight(int size) noexcept;
}
