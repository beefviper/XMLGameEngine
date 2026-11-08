// image.h
// XML Game Engine
// author: beefviper
// date: Nov 8, 2026

#pragma once

#include "picture/bitmap.h"

#include <memory>
#include <string>
#include <vector>

namespace xge
{
	// A game's <sheet name="...">: one picture file holding many pictures (an
	// animation's frames, a deck's faces), and how its cells lie: a grid of
	// `columns` by `rows` sharing the picture out, or of cells `cellWidth` by
	// `cellHeight`, with `margin` round the edge and `gap` between cells (in
	// the picture's pixels, or an SVG drawing's units). A picture taken by
	// its own x, y, width and height needs no grid. `hide`: an SVG's
	// elements left out.
	struct Sheet
	{
		std::string name;
		std::string path;
		int columns{ 0 };
		int rows{ 0 };
		float cellWidth{ 0.0f };
		float cellHeight{ 0.0f };
		float margin{ 0.0f };
		float gap{ 0.0f };
		std::vector<std::string> hide;

		bool hasGrid() const noexcept { return (columns > 0 && rows > 0) || (cellWidth > 0.0f && cellHeight > 0.0f); }
	};

	// One picture on a sheet: the cell at `column` and `row` (from 1), or,
	// `free`, the part at x, y, width by height.
	struct SheetCell
	{
		int column{ 1 };
		int row{ 1 };
		bool free{ false };
		float x{ 0.0f };
		float y{ 0.0f };
		float width{ 0.0f };
		float height{ 0.0f };
	};

	// A part of a sheet's picture, in its pixels or units.
	struct SheetPart
	{
		float x{ 0.0f };
		float y{ 0.0f };
		float width{ 0.0f };
		float height{ 0.0f };
	};

	// A sheet's picture, opened to be cut, held only as long as this is. A
	// PNG, JPEG or BMP is read by the engine itself with the image loader of
	// the SVG library's drawing library (plutovg), an SVG drawn by lunasvg,
	// so what is cut goes to every backend as an ordinary picture; nothing
	// outside picture/image.cpp includes plutovg. Throws std::runtime_error,
	// saying which file, if it cannot be read.
	class SheetPicture
	{
	public:
		explicit SheetPicture(const Sheet& sheet);
		~SheetPicture();
		SheetPicture(const SheetPicture&) = delete;
		SheetPicture& operator=(const SheetPicture&) = delete;

		float width() const noexcept;
		float height() const noexcept;

		// Where a cell lies. Throws std::invalid_argument for a cell of a
		// sheet with no grid, outside the picture, or of no size.
		SheetPart partOf(const SheetCell& cell) const;

		// The part made toWidth by toHeight: each new pixel the average of
		// the old ones it covers (a part of a pixel counted by how much of it
		// is covered, colors weighed by their alpha), so a big picture shrinks
		// smoothly and a whole-number shrink of pixel art gives its pixels
		// back exactly. An SVG's part is drawn at that size.
		Bitmap cut(const SheetPart& part, int toWidth, int toHeight) const;

		// The part `scale` pixels to a pixel (or unit) of the picture.
		Bitmap cut(const SheetPart& part, float scale) const;

	private:
		struct Picture;
		std::unique_ptr<Picture> picture;
		Sheet sheet;
	};
}
