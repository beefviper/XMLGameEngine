// bitmap.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "bitmap.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace xge
{
	namespace
	{
		struct Point
		{
			int x{};
			int y{};
		};

		Point rounded(float x, float y)
		{
			return { static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)) };
		}

		void stamp(Bitmap& bitmap, int left, int top, int size, const Color& color)
		{
			for (int y = top; y < top + size; ++y)
			{
				for (int x = left; x < left + size; ++x)
				{
					if (x < 0 || y < 0 || x >= bitmap.width || y >= bitmap.height) { continue; }

					const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(bitmap.width) + static_cast<std::size_t>(x)) * 4;
					bitmap.rgba[at + 0] = color.r;
					bitmap.rgba[at + 1] = color.g;
					bitmap.rgba[at + 2] = color.b;
					bitmap.rgba[at + 3] = color.a;
				}
			}
		}
	}

	Bitmap rasterizeLines(const std::vector<LineSegment>& lines)
	{
		Bitmap bitmap;
		if (lines.empty()) { return bitmap; }

		int right = 0;
		int bottom = 0;

		for (const LineSegment& line : lines)
		{
			if (line.x1 < 0 || line.y1 < 0 || line.x2 < 0 || line.y2 < 0)
			{
				throw std::invalid_argument("a line has a negative coordinate; a sprite's lines are measured from its top left corner");
			}

			const int thickness = std::max(line.thickness, 1);
			for (const Point end : { rounded(line.x1, line.y1), rounded(line.x2, line.y2) })
			{
				right = std::max(right, end.x + thickness);
				bottom = std::max(bottom, end.y + thickness);
			}
		}

		bitmap.width = right;
		bitmap.height = bottom;
		bitmap.rgba.assign(static_cast<std::size_t>(right) * static_cast<std::size_t>(bottom) * 4, 0);

		for (const LineSegment& line : lines)
		{
			const int thickness = std::max(line.thickness, 1);
			const Point from = rounded(line.x1, line.y1);
			const Point to = rounded(line.x2, line.y2);

			// Bresenham: every pixel from one end to the other, with no gaps.
			const int dx = std::abs(to.x - from.x);
			const int dy = -std::abs(to.y - from.y);
			const int stepX = from.x < to.x ? 1 : -1;
			const int stepY = from.y < to.y ? 1 : -1;
			int error = dx + dy;
			int x = from.x;
			int y = from.y;

			while (true)
			{
				stamp(bitmap, x, y, thickness, line.color);

				if (x == to.x && y == to.y) { break; }

				const int doubled = 2 * error;
				if (doubled >= dy) { error += dy; x += stepX; }
				if (doubled <= dx) { error += dx; y += stepY; }
			}
		}

		return bitmap;
	}
}
