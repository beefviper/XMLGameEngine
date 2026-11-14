// font.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "picture/bitmap.h"
#include "picture/color.h"

#include <string>
#include <vector>

namespace xge
{
	// The font file a text is drawn with: the one its <font> (or its
	// style's) names (Picture::font), else assets/tuffy.ttf. Relative to
	// the games folder, the working directory.
	const std::string& textFontFile(const std::string& font);

	// Draws `text` in the font file at `size` pixels to the em, in `color`,
	// onto a transparent Bitmap: what every window shows for a text, so a
	// text looks and measures the same whichever library draws the window,
	// and its size is known with no window at all. The bitmap is the box the
	// ink covers, top to bottom, and left to right with any spaces at either
	// end; a '\n' starts a new line one line height down (the font's ascent,
	// descent and line gap). Edges are smoothed (antialiased). A font file
	// that cannot be opened is said once on std::cout, and its texts are
	// drawn in the built-in font (rasterizeText). Returns an empty Bitmap for
	// empty text; throws std::invalid_argument for a size too large
	// (refusePictureSize). Fonts are read with plutovg, opened once and kept.
	Bitmap drawText(const std::string& text, int size, const Color& color, const std::string& fontFile);

	// The font the engine carries inside itself, for when a game's own font
	// file cannot be found (see the window backends): an 8 by 8 pixel glyph
	// for each printable ASCII character, stored in picture/font.cpp, so text
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
