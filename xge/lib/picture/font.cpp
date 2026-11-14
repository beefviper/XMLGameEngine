// font.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "picture/font.h"

#include <plutovg.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace xge
{
	const std::string& textFontFile(const std::string& font)
	{
		static const std::string standard{ "assets/tuffy.ttf" };
		return font.empty() ? standard : font;
	}

	namespace
	{
		constexpr unsigned char kFirstGlyph = 32; // space
		constexpr unsigned char kLastGlyph = 126; // ~

		// 8x8 monochrome bitmap font for the printable ASCII characters.
		// Author: Daniel Hepper (font8x8), based on the IBM VGA font; public
		// domain (https://github.com/dhepper/font8x8).
		//
		// Eight bytes per character, one for each row from the top. Within a
		// row the lowest bit is the leftmost pixel. Rows below the baseline
		// (the tails of g, j, p, q, y and the underscore) use the last row.
		constexpr std::array<std::array<std::uint8_t, 8>, kLastGlyph - kFirstGlyph + 1> kGlyphs
		{ {
			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // space
			{ 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 },   // !
			{ 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // "
			{ 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 },   // #
			{ 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 },   // $
			{ 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 },   // %
			{ 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 },   // &
			{ 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 },   // '
			{ 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 },   // (
			{ 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 },   // )
			{ 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 },   // *
			{ 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 },   // +
			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 },   // ,
			{ 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 },   // -
			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 },   // .
			{ 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 },   // /
			{ 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 },   // 0
			{ 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 },   // 1
			{ 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 },   // 2
			{ 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 },   // 3
			{ 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 },   // 4
			{ 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 },   // 5
			{ 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 },   // 6
			{ 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 },   // 7
			{ 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 },   // 8
			{ 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 },   // 9
			{ 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 },   // :
			{ 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 },   // ;
			{ 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 },   // <
			{ 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 },   // =
			{ 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 },   // >
			{ 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 },   // ?
			{ 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 },   // @
			{ 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 },   // A
			{ 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 },   // B
			{ 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 },   // C
			{ 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 },   // D
			{ 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 },   // E
			{ 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 },   // F
			{ 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 },   // G
			{ 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 },   // H
			{ 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 },   // I
			{ 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 },   // J
			{ 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 },   // K
			{ 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 },   // L
			{ 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 },   // M
			{ 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 },   // N
			{ 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 },   // O
			{ 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 },   // P
			{ 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 },   // Q
			{ 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 },   // R
			{ 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 },   // S
			{ 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 },   // T
			{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 },   // U
			{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 },   // V
			{ 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 },   // W
			{ 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 },   // X
			{ 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 },   // Y
			{ 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 },   // Z
			{ 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 },   // [
			{ 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 },   // backslash
			{ 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 },   // ]
			{ 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 },   // ^
			{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF },   // _
			{ 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 },   // `
			{ 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 },   // a
			{ 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 },   // b
			{ 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 },   // c
			{ 0x38, 0x30, 0x30, 0x3E, 0x33, 0x33, 0x6E, 0x00 },   // d
			{ 0x00, 0x00, 0x1E, 0x33, 0x3F, 0x03, 0x1E, 0x00 },   // e
			{ 0x1C, 0x36, 0x06, 0x0F, 0x06, 0x06, 0x0F, 0x00 },   // f
			{ 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F },   // g
			{ 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 },   // h
			{ 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 },   // i
			{ 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E },   // j
			{ 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 },   // k
			{ 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 },   // l
			{ 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 },   // m
			{ 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 },   // n
			{ 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 },   // o
			{ 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F },   // p
			{ 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 },   // q
			{ 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 },   // r
			{ 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 },   // s
			{ 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 },   // t
			{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 },   // u
			{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 },   // v
			{ 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 },   // w
			{ 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 },   // x
			{ 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F },   // y
			{ 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 },   // z
			{ 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 },   // {
			{ 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 },   // |
			{ 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 },   // }
			{ 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   // ~
		} };

		// What the glyph for `character` is: itself when printable ASCII,
		// otherwise the question mark.
		const std::array<std::uint8_t, 8>& glyphFor(unsigned char character) noexcept
		{
			if (character < kFirstGlyph || character > kLastGlyph)
			{
				character = '?';
			}

			return kGlyphs[character - kFirstGlyph];
		}
	}

	int builtinFontScale(int size) noexcept
	{
		// A glyph cell is 8 pixels tall at scale 1, and the real font's
		// letters average about half the size across (0.4 to 0.6 of it), so
		// a size of 16 is a scale of 1.
		return std::max(1, (size + kBuiltinGlyphSize) / (2 * kBuiltinGlyphSize));
	}

	int builtinCellHeight(int size) noexcept
	{
		return kBuiltinGlyphSize * builtinFontScale(size);
	}

	int builtinCellWidth(int size) noexcept
	{
		// Half the height, which is 4 pixels for each step of the scale, but
		// not narrower than the 8 pixels a glyph is made of.
		return std::max(kBuiltinGlyphSize, builtinCellHeight(size) / 2);
	}

	Bitmap rasterizeText(const std::string& text, int size, const Color& color)
	{
		const int scale = builtinFontScale(size);
		const int cellWidth = builtinCellWidth(size);
		const int cellHeight = builtinCellHeight(size);

		// Split into lines of characters, one entry per character; a
		// multi-byte UTF-8 character is one entry, drawn as '?'.
		std::vector<std::vector<unsigned char>> lines(1);
		for (const char raw : text)
		{
			const auto character = static_cast<unsigned char>(raw);

			if (character == '\n') { lines.emplace_back(); continue; }
			if (character == '\r') { continue; }
			if (character == '\t') { lines.back().push_back(' '); continue; }

			// A continuation byte belongs to the character before it.
			if ((character & 0xC0) == 0x80) { continue; }

			lines.back().push_back(character >= 0x80 ? static_cast<unsigned char>('?') : character);
		}

		std::size_t longest = 0;
		for (const auto& line : lines) { longest = std::max(longest, line.size()); }

		Bitmap bitmap;
		if (longest == 0)
		{
			return bitmap;
		}

		refusePictureSize(static_cast<double>(longest) * cellWidth, static_cast<double>(lines.size()) * cellHeight, "a text");
		bitmap.width = static_cast<int>(longest) * cellWidth;
		bitmap.height = static_cast<int>(lines.size()) * cellHeight;
		bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4, 0);

		for (std::size_t row = 0; row < lines.size(); ++row)
		{
			for (std::size_t column = 0; column < lines[row].size(); ++column)
			{
				const auto& glyph = glyphFor(lines[row][column]);

				for (int y = 0; y < kBuiltinGlyphSize; ++y)
				{
					for (int x = 0; x < kBuiltinGlyphSize; ++x)
					{
						if (((glyph[static_cast<std::size_t>(y)] >> x) & 1) == 0) { continue; }

						// The glyph pixel becomes a block: scale pixels tall, and
						// as wide as the cell's width makes it (the columns'
						// edges are the cell width times x / 8, so they add up
						// to the cell exactly).
						const int cellLeft = static_cast<int>(column) * cellWidth;
						const int left = cellLeft + x * cellWidth / kBuiltinGlyphSize;
						const int right = cellLeft + (x + 1) * cellWidth / kBuiltinGlyphSize;
						const int top = static_cast<int>(row) * cellHeight + y * scale;

						for (int blockY = top; blockY < top + scale; ++blockY)
						{
							for (int blockX = left; blockX < right; ++blockX)
							{
								const std::size_t at = (static_cast<std::size_t>(blockY) * static_cast<std::size_t>(bitmap.width) + static_cast<std::size_t>(blockX)) * 4;
								bitmap.rgba[at + 0] = color.r;
								bitmap.rgba[at + 1] = color.g;
								bitmap.rgba[at + 2] = color.b;
								bitmap.rgba[at + 3] = color.a;
							}
						}
					}
				}
			}
		}

		return bitmap;
	}

	namespace
	{
		struct FaceCloser
		{
			void operator()(plutovg_font_face_t* face) const noexcept { plutovg_font_face_destroy(face); }
		};

		// Every font file opened, by its name, kept for the life of the
		// program; null for one that could not be opened (said once). The
		// lock covers the faces too: plutovg fills a face's glyph cache as it
		// reads.
		std::mutex facesLock;
		std::map<std::string, std::unique_ptr<plutovg_font_face_t, FaceCloser>> faces;

		plutovg_font_face_t* faceFor(const std::string& file)
		{
			const auto found = faces.find(file);
			if (found != faces.end()) { return found->second.get(); }
			plutovg_font_face_t* face = plutovg_font_face_load_from_file(file.c_str(), 0);
			if (face == nullptr)
			{
				std::cout << "error: failed to load font: " << file << " - drawing text with the built-in 8x8 font instead" << std::endl;
			}
			return faces.emplace(file, face).first->second.get();
		}

		// The text's lines, each its characters as Unicode code points: '\n'
		// starts a line, '\r' is dropped and a tab is a space.
		std::vector<std::vector<plutovg_codepoint_t>> codepointLines(const std::string& text)
		{
			std::vector<std::vector<plutovg_codepoint_t>> lines(1);
			plutovg_text_iterator_t it;
			plutovg_text_iterator_init(&it, text.data(), static_cast<int>(text.size()), PLUTOVG_TEXT_ENCODING_UTF8);
			while (plutovg_text_iterator_has_next(&it))
			{
				const plutovg_codepoint_t codepoint = plutovg_text_iterator_next(&it);
				if (codepoint == '\n') { lines.emplace_back(); }
				else if (codepoint == '\t') { lines.back().push_back(' '); }
				else if (codepoint != '\r') { lines.back().push_back(codepoint); }
			}
			return lines;
		}
	}

	Bitmap drawText(const std::string& text, int size, const Color& color, const std::string& fontFile)
	{
		std::lock_guard<std::mutex> hold(facesLock);
		plutovg_font_face_t* face = faceFor(fontFile);
		if (face == nullptr) { return rasterizeText(text, size, color); }

		const float em = static_cast<float>(size);
		float ascent = 0.0f;
		float descent = 0.0f;
		float gap = 0.0f;
		plutovg_font_face_get_metrics(face, em, &ascent, &descent, &gap, nullptr);
		const float lineHeight = ascent - descent + gap;

		// The box the ink covers, with each line's baseline at y = line *
		// lineHeight; a space or other glyph with no ink still counts across.
		const auto lines = codepointLines(text);
		float left = std::numeric_limits<float>::max();
		float top = left;
		float right = std::numeric_limits<float>::lowest();
		float bottom = right;
		for (std::size_t line = 0; line < lines.size(); ++line)
		{
			const float baseline = static_cast<float>(line) * lineHeight;
			float pen = 0.0f;
			for (const plutovg_codepoint_t codepoint : lines[line])
			{
				float advance = 0.0f;
				plutovg_rect_t ink{};
				plutovg_font_face_get_glyph_metrics(face, em, codepoint, &advance, nullptr, &ink);
				if (ink.w > 0.0f && ink.h > 0.0f)
				{
					left = std::min(left, pen + ink.x);
					right = std::max(right, pen + ink.x + ink.w);
					top = std::min(top, baseline + ink.y);
					bottom = std::max(bottom, baseline + ink.y + ink.h);
				}
				else
				{
					left = std::min(left, pen);
					right = std::max(right, pen + advance);
				}
				pen += advance;
			}
		}

		Bitmap bitmap;
		if (right <= left) { return bitmap; }
		if (bottom <= top) { top = 0.0f; bottom = 1.0f; }   // spaces alone: one clear row
		// An edge within a 64th of a pixel of a whole one is on it: a font's
		// units seldom divide into pixels exactly.
		constexpr float slack = 1.0f / 64.0f;
		const float x0 = std::floor(left + slack);
		const float y0 = std::floor(top + slack);
		const double wide = std::max(1.0f, std::ceil(right - slack) - x0);
		const double high = std::max(1.0f, std::ceil(bottom - slack) - y0);
		refusePictureSize(wide, high, "a text");
		bitmap.width = static_cast<int>(wide);
		bitmap.height = static_cast<int>(high);

		plutovg_surface_t* surface = plutovg_surface_create(bitmap.width, bitmap.height);
		plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
		plutovg_canvas_set_font(canvas, face, em);
		plutovg_canvas_set_rgba(canvas, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
		for (std::size_t line = 0; line < lines.size(); ++line)
		{
			float pen = -x0;
			const float baseline = static_cast<float>(line) * lineHeight - y0;
			for (const plutovg_codepoint_t codepoint : lines[line])
			{
				pen += plutovg_canvas_add_glyph(canvas, codepoint, pen, baseline);
			}
		}
		plutovg_canvas_fill(canvas);
		plutovg_canvas_destroy(canvas);

		bitmap.rgba.resize(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4);
		plutovg_convert_argb_to_rgba(bitmap.rgba.data(), plutovg_surface_get_data(surface), bitmap.width, bitmap.height, plutovg_surface_get_stride(surface));
		plutovg_surface_destroy(surface);
		return bitmap;
	}
}
