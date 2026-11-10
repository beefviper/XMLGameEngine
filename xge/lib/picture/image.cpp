// image.cpp
// XML Game Engine
// author: beefviper
// date: Nov 8, 2026

#include "picture/image.h"
#include "picture/svg.h"

#include <plutovg.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

namespace xge
{
	namespace
	{
		bool isSvg(const std::string& path)
		{
			if (path.size() < 4) { return false; }
			std::string end = path.substr(path.size() - 4);
			std::transform(end.begin(), end.end(), end.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return end == ".svg";
		}

		// `from`'s part made toWidth by toHeight, each new pixel the average
		// of the old ones it covers. `at(x, y, sums)` adds a pixel's colors,
		// premultiplied by its alpha, and its alpha (0 to 255), times sums[4].
		template <typename Pixel>
		Bitmap shrink(const SheetPart& part, int toWidth, int toHeight, int fromWidth, int fromHeight, Pixel&& at)
		{
			Bitmap made;
			made.width = toWidth;
			made.height = toHeight;
			made.rgba.assign(static_cast<std::size_t>(toWidth) * static_cast<std::size_t>(toHeight) * 4, 0);
			const double stepX = static_cast<double>(part.width) / static_cast<double>(toWidth);
			const double stepY = static_cast<double>(part.height) / static_cast<double>(toHeight);

			for (int row = 0; row < toHeight; ++row)
			{
				const double top = static_cast<double>(part.y) + static_cast<double>(row) * stepY;
				const double bottom = top + stepY;
				for (int column = 0; column < toWidth; ++column)
				{
					const double left = static_cast<double>(part.x) + static_cast<double>(column) * stepX;
					const double right = left + stepX;
					double sums[4]{};
					double area = 0.0;
					for (int oldY = std::max(0, static_cast<int>(std::floor(top))); oldY < std::min(fromHeight, static_cast<int>(std::ceil(bottom))); ++oldY)
					{
						const double high = std::min(bottom, static_cast<double>(oldY + 1)) - std::max(top, static_cast<double>(oldY));
						for (int oldX = std::max(0, static_cast<int>(std::floor(left))); oldX < std::min(fromWidth, static_cast<int>(std::ceil(right))); ++oldX)
						{
							const double share = (std::min(right, static_cast<double>(oldX + 1)) - std::max(left, static_cast<double>(oldX))) * high;
							at(oldX, oldY, share, sums);
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

	struct SheetPicture::Picture
	{
		plutovg_surface_t* surface{ nullptr };   // a picture file; null for an SVG
		SvgRegion size;

		~Picture()
		{
			if (surface != nullptr) { plutovg_surface_destroy(surface); }
		}
	};

	SheetPicture::SheetPicture(const Sheet& given) : picture(std::make_unique<Picture>()), sheet(given)
	{
		if (isSvg(sheet.path))
		{
			picture->size = svgSize(sheet.path);
			return;
		}
		picture->surface = plutovg_surface_load_from_image_file(sheet.path.c_str());
		if (picture->surface == nullptr) { throw std::runtime_error("could not read the picture " + sheet.path + " (missing, or not a picture file)"); }
		picture->size = { 0.0f, 0.0f, static_cast<float>(plutovg_surface_get_width(picture->surface)), static_cast<float>(plutovg_surface_get_height(picture->surface)) };
	}

	SheetPicture::~SheetPicture() = default;

	float SheetPicture::width() const noexcept { return picture->size.width; }
	float SheetPicture::height() const noexcept { return picture->size.height; }

	SheetPart SheetPicture::partOf(const SheetCell& cell) const
	{
		const std::string named = "the sheet \"" + sheet.name + "\" (" + sheet.path + ")";
		SheetPart part;
		if (cell.free)
		{
			part = { cell.x, cell.y, cell.width, cell.height };
		}
		else
		{
			if (!sheet.hasGrid()) { throw std::invalid_argument(named + " has no <columns> and <rows> or <cell> size, so a picture on it is taken by <x>, <y>, <width> and <height>"); }
			float cellWidth = sheet.cellWidth;
			float cellHeight = sheet.cellHeight;
			if (sheet.columns > 0 && sheet.rows > 0)
			{
				cellWidth = (width() - 2.0f * sheet.margin - static_cast<float>(sheet.columns - 1) * sheet.gap) / static_cast<float>(sheet.columns);
				cellHeight = (height() - 2.0f * sheet.margin - static_cast<float>(sheet.rows - 1) * sheet.gap) / static_cast<float>(sheet.rows);
				if (cell.column > sheet.columns || cell.row > sheet.rows)
				{
					throw std::invalid_argument("no cell at column " + std::to_string(cell.column) + ", row " + std::to_string(cell.row) + " of " + named
						+ ", which has " + std::to_string(sheet.columns) + " columns and " + std::to_string(sheet.rows) + " rows");
				}
			}
			if (cell.column < 1 || cell.row < 1) { throw std::invalid_argument("a sheet's columns and rows count from 1"); }
			part = { sheet.margin + static_cast<float>(cell.column - 1) * (cellWidth + sheet.gap), sheet.margin + static_cast<float>(cell.row - 1) * (cellHeight + sheet.gap), cellWidth, cellHeight };
		}
		if (!(part.width > 0.0f) || !(part.height > 0.0f)) { throw std::invalid_argument("a picture on " + named + " has no size"); }
		if (part.x < 0.0f || part.y < 0.0f || part.x + part.width > width() + 0.01f || part.y + part.height > height() + 0.01f)
		{
			throw std::invalid_argument("the part at " + std::to_string(part.x) + ", " + std::to_string(part.y) + " of " + std::to_string(part.width) + " by " + std::to_string(part.height)
				+ " is not inside " + named + ", which is " + std::to_string(width()) + " by " + std::to_string(height()));
		}
		return part;
	}

	Bitmap SheetPicture::cut(const SheetPart& part, int toWidth, int toHeight) const
	{
		if (toWidth < 1 || toHeight < 1) { throw std::invalid_argument("a picture cut from a sheet must be at least 1 by 1"); }
		refusePictureSize(toWidth, toHeight, "a picture cut from " + sheet.path);
		if (picture->surface == nullptr)
		{
			const Bitmap drawn = rasterizeSvg(sheet.path, { part.x, part.y, part.width, part.height }, static_cast<float>(toWidth) / part.width, sheet.hide);
			if (drawn.width == toWidth && drawn.height == toHeight) { return drawn; }
			// Drawn at the width asked for; its height made to fit.
			return shrink({ 0.0f, 0.0f, static_cast<float>(drawn.width), static_cast<float>(drawn.height) }, toWidth, toHeight, drawn.width, drawn.height,
				[&](int x, int y, double share, double* sums)
				{
					const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(drawn.width) + static_cast<std::size_t>(x)) * 4;
					const double alpha = drawn.rgba[at + 3];
					sums[0] += share * drawn.rgba[at] * alpha / 255.0;
					sums[1] += share * drawn.rgba[at + 1] * alpha / 255.0;
					sums[2] += share * drawn.rgba[at + 2] * alpha / 255.0;
					sums[3] += share * alpha;
				});
		}
		const unsigned char* data = plutovg_surface_get_data(picture->surface);
		const std::size_t stride = static_cast<std::size_t>(plutovg_surface_get_stride(picture->surface));
		return shrink(part, toWidth, toHeight, plutovg_surface_get_width(picture->surface), plutovg_surface_get_height(picture->surface),
			[&](int x, int y, double share, double* sums)
			{
				// 32-bit words, alpha in the top byte, colors premultiplied.
				std::uint32_t pixel{};
				std::memcpy(&pixel, data + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) * 4, 4);
				sums[0] += share * static_cast<double>((pixel >> 16) & 0xFFu);
				sums[1] += share * static_cast<double>((pixel >> 8) & 0xFFu);
				sums[2] += share * static_cast<double>(pixel & 0xFFu);
				sums[3] += share * static_cast<double>(pixel >> 24);
			});
	}

	Bitmap SheetPicture::cut(const SheetPart& part, float scale) const
	{
		if (!(scale > 0.0f)) { throw std::invalid_argument("a sheet picture's <scale> must be above 0"); }
		if (picture->surface == nullptr) { return rasterizeSvg(sheet.path, { part.x, part.y, part.width, part.height }, scale, sheet.hide); }
		return cut(part, std::max(1, static_cast<int>(std::lround(part.width * scale))), std::max(1, static_cast<int>(std::lround(part.height * scale))));
	}
}
