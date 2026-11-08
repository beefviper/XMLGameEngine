// image.h
// XML Game Engine
// author: beefviper
// date: Nov 8, 2026

#pragma once

#include "picture/bitmap.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A picture file (PNG, JPEG, BMP...) of equal cells, `columns` across and
	// `rows` down, read by the engine itself with no window library involved,
	// so what is cut from it goes to every backend as an ordinary picture:
	// each of `cells` (a column and row, from 0) made toWidth by toHeight.
	// Each new pixel is the average of the old ones it covers (a part of a
	// pixel counted by how much of it is covered, colors weighed by their
	// alpha), so a big picture shrinks smoothly, and a whole-number shrink of
	// pixel art gives its pixels back exactly.
	//
	// Read with the image loader of the SVG library's drawing library
	// (plutovg); nothing outside picture/image.cpp includes it. The file is
	// held only while it is cut.
	//
	// Throws std::runtime_error, saying which file, if it cannot be read or
	// does not split evenly into the cells, and std::invalid_argument for a
	// cell outside the grid or a size under 1.
	std::vector<Bitmap> cutImage(const std::string& path, int columns, int rows, const std::vector<std::pair<int, int>>& cells, int toWidth, int toHeight);
}
