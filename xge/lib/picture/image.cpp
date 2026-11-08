// image.cpp
// XML Game Engine
// author: beefviper
// date: Nov 8, 2026

#include "picture/image.h"

#include <plutovg.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

namespace xge
{
	namespace
	{
		struct SurfaceDeleter
		{
			void operator()(plutovg_surface_t* surface) const noexcept { plutovg_surface_destroy(surface); }
		};

		// One cell of the picture, resized. The surface's pixels are 32-bit
		// words, alpha in the top byte, the colors premultiplied by it, so a
		// plain average of them is already weighed by alpha.
		Bitmap cut(const plutovg_surface_t* surface, int x, int y, int width, int height, int toWidth, int toHeight)
		{
			const unsigned char* data = plutovg_surface_get_data(surface);
			const std::size_t stride = static_cast<std::size_t>(plutovg_surface_get_stride(surface));
			Bitmap made;
			made.width = toWidth;
			made.height = toHeight;
			made.rgba.assign(static_cast<std::size_t>(toWidth) * static_cast<std::size_t>(toHeight) * 4, 0);
			const double stepX = static_cast<double>(width) / static_cast<double>(toWidth);
			const double stepY = static_cast<double>(height) / static_cast<double>(toHeight);

			for (int row = 0; row < toHeight; ++row)
			{
				const double top = static_cast<double>(row) * stepY;
				const double bottom = top + stepY;
				for (int column = 0; column < toWidth; ++column)
				{
					const double left = static_cast<double>(column) * stepX;
					const double right = left + stepX;
					double sums[4]{};
					double area = 0.0;
					for (int oldY = static_cast<int>(std::floor(top)); oldY < std::min(height, static_cast<int>(std::ceil(bottom))); ++oldY)
					{
						const double high = std::min(bottom, static_cast<double>(oldY + 1)) - std::max(top, static_cast<double>(oldY));
						const unsigned char* line = data + static_cast<std::size_t>(y + oldY) * stride;
						for (int oldX = static_cast<int>(std::floor(left)); oldX < std::min(width, static_cast<int>(std::ceil(right))); ++oldX)
						{
							const double share = (std::min(right, static_cast<double>(oldX + 1)) - std::max(left, static_cast<double>(oldX))) * high;
							std::uint32_t pixel{};
							std::memcpy(&pixel, line + static_cast<std::size_t>(x + oldX) * 4, 4);
							sums[0] += share * static_cast<double>((pixel >> 16) & 0xFFu);
							sums[1] += share * static_cast<double>((pixel >> 8) & 0xFFu);
							sums[2] += share * static_cast<double>(pixel & 0xFFu);
							sums[3] += share * static_cast<double>(pixel >> 24);
							area += share;
						}
					}
					if (!(area > 0.0) || !(sums[3] > 0.0)) { continue; }
					const std::size_t to = (static_cast<std::size_t>(row) * static_cast<std::size_t>(toWidth) + static_cast<std::size_t>(column)) * 4;
					for (std::size_t c = 0; c < 3; ++c)
					{
						made.rgba[to + c] = static_cast<std::uint8_t>(std::lround(std::clamp(sums[c] * 255.0 / sums[3], 0.0, 255.0)));
					}
					made.rgba[to + 3] = static_cast<std::uint8_t>(std::lround(std::clamp(sums[3] / area, 0.0, 255.0)));
				}
			}
			return made;
		}
	}

	std::vector<Bitmap> cutImage(const std::string& path, int columns, int rows, const std::vector<std::pair<int, int>>& cells, int toWidth, int toHeight)
	{
		if (columns < 1 || rows < 1 || toWidth < 1 || toHeight < 1) { throw std::invalid_argument("a picture's cells and their size must be at least 1 by 1"); }
		const std::unique_ptr<plutovg_surface_t, SurfaceDeleter> surface(plutovg_surface_load_from_image_file(path.c_str()));
		if (!surface) { throw std::runtime_error("could not read the picture " + path + " (missing, or not a picture file)"); }
		const int width = plutovg_surface_get_width(surface.get());
		const int height = plutovg_surface_get_height(surface.get());
		if (width % columns != 0 || height % rows != 0)
		{
			throw std::runtime_error(path + " is " + std::to_string(width) + " by " + std::to_string(height) + ", which does not split into "
				+ std::to_string(columns) + " columns and " + std::to_string(rows) + " rows of equal cells");
		}
		const int cellWidth = width / columns;
		const int cellHeight = height / rows;

		std::vector<Bitmap> made;
		made.reserve(cells.size());
		for (const auto& [column, row] : cells)
		{
			if (column < 0 || row < 0 || column >= columns || row >= rows)
			{
				throw std::invalid_argument("no cell at column " + std::to_string(column + 1) + ", row " + std::to_string(row + 1) + " of " + path);
			}
			made.push_back(cut(surface.get(), column * cellWidth, row * cellHeight, cellWidth, cellHeight, toWidth, toHeight));
		}
		return made;
	}
}
