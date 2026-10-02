// bitmap.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "bitmap.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

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

	Bitmap rasterizeRows(const std::vector<std::string>& rows, int scale, const Color& color)
	{
		if (rows.empty()) { throw std::invalid_argument("a bitmap has no <row>s"); }
		if (scale < 1) { throw std::invalid_argument("a bitmap's <scale> is " + std::to_string(scale) + "; expected 1 or more"); }

		const std::size_t columns = rows.front().size();
		for (std::size_t row = 0; row < rows.size(); ++row)
		{
			const std::string& text = rows[row];
			const std::string name = "row " + std::to_string(row + 1) + " of a bitmap";

			if (text.empty()) { throw std::invalid_argument(name + " is empty"); }
			if (text.size() != columns)
			{
				throw std::invalid_argument(name + " is " + std::to_string(text.size()) + " characters wide, but row 1 is "
					+ std::to_string(columns) + "; every row must be the same width");
			}

			for (std::size_t column = 0; column < text.size(); ++column)
			{
				if (text[column] != '.' && text[column] != '*')
				{
					throw std::invalid_argument(name + " has '" + std::string(1, text[column]) + "' as character " + std::to_string(column + 1)
						+ "; use '.' for a clear pixel and '*' for a solid one");
				}
			}
		}

		Bitmap bitmap;
		bitmap.width = static_cast<int>(columns) * scale;
		bitmap.height = static_cast<int>(rows.size()) * scale;
		bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4, 0);

		for (std::size_t row = 0; row < rows.size(); ++row)
		{
			for (std::size_t column = 0; column < columns; ++column)
			{
				if (rows[row][column] == '*')
				{
					stamp(bitmap, static_cast<int>(column) * scale, static_cast<int>(row) * scale, scale, color);
				}
			}
		}

		return bitmap;
	}

	Bitmap rasterizeLines(const std::vector<LineSegment>& lines, int minWidth, int minHeight)
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

		bitmap.width = std::max(right, minWidth);
		bitmap.height = std::max(bottom, minHeight);
		bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4, 0);

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

	namespace
	{
		// A heading as the whole degrees, from 0 up to 359, that it is drawn at.
		int wholeDegrees(float degrees)
		{
			long whole = std::lround(degrees) % 360;
			if (whole < 0) { whole += 360; }
			return static_cast<int>(whole);
		}
	}

	Bitmap turnBitmap(const Bitmap& picture, float degrees)
	{
		if (picture.width < 1 || picture.height < 1) { return Bitmap{}; }

		const float width = static_cast<float>(picture.width);
		const float height = static_cast<float>(picture.height);
		const int side = std::max({ static_cast<int>(std::ceil(std::hypot(width, height))), picture.width, picture.height });
		const float middle = static_cast<float>(side) / 2.0f;
		const float pi = 3.14159265358979323846f;

		const float angle = 2.0f * pi * static_cast<float>(wholeDegrees(degrees)) / 360.0f;
		const float c = std::cos(angle);
		const float s = std::sin(angle);

		Bitmap turned;
		turned.width = side;
		turned.height = side;
		turned.rgba.assign(static_cast<std::size_t>(side) * static_cast<std::size_t>(side) * 4, 0);

		// 90, 180 and 270 degrees should be exact, not a hair off.
		const int whole = wholeDegrees(degrees);
		const float cosine = whole == 90 || whole == 270 ? 0.0f : c;
		const float sine = whole == 0 || whole == 180 ? 0.0f : s;

		for (int y = 0; y < side; ++y)
		{
			for (int x = 0; x < side; ++x)
			{
				// The pixel's centre, turned back the other way (a clockwise
				// turn on a screen, where y grows downward) to find what lies
				// under it in the original. The small amount keeps a pixel
				// that lands exactly on an edge from tipping the wrong way
				// through rounding.
				const float dx = static_cast<float>(x) + 0.5f - middle;
				const float dy = static_cast<float>(y) + 0.5f - middle;
				const int sx = static_cast<int>(std::floor(dx * cosine + dy * sine + width / 2.0f + 0.0001f));
				const int sy = static_cast<int>(std::floor(-dx * sine + dy * cosine + height / 2.0f + 0.0001f));

				if (!picture.solidAt(sx, sy)) { continue; }

				const std::size_t from = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(picture.width) + static_cast<std::size_t>(sx)) * 4;
				const std::size_t to = (static_cast<std::size_t>(y) * static_cast<std::size_t>(side) + static_cast<std::size_t>(x)) * 4;
				for (std::size_t k = 0; k < 4; ++k) { turned.rgba[to + k] = picture.rgba[from + k]; }
			}
		}

		return turned;
	}

	Bitmap rasterizeTurned(const std::vector<LineSegment>& lines, float degrees)
	{
		if (lines.empty()) { return Bitmap{}; }

		float left = lines.front().x1;
		float right = left;
		float top = lines.front().y1;
		float bottom = top;
		int thickest = 1;

		for (const LineSegment& line : lines)
		{
			if (line.x1 < 0 || line.y1 < 0 || line.x2 < 0 || line.y2 < 0)
			{
				throw std::invalid_argument("a line has a negative coordinate; a sprite's lines are measured from its top left corner");
			}

			for (const float x : { line.x1, line.x2 }) { left = std::min(left, x); right = std::max(right, x); }
			for (const float y : { line.y1, line.y2 }) { top = std::min(top, y); bottom = std::max(bottom, y); }
			thickest = std::max(thickest, line.thickness);
		}

		const float middleX = (left + right) / 2.0f;
		const float middleY = (top + bottom) / 2.0f;

		// The furthest any end lies from the middle is how far the drawing can
		// reach at any heading; a line is drawn by stamping a square of its
		// thickness at each point, so that much more on the far side.
		float reach = 0.0f;
		for (const LineSegment& line : lines)
		{
			reach = std::max(reach, std::hypot(line.x1 - middleX, line.y1 - middleY));
			reach = std::max(reach, std::hypot(line.x2 - middleX, line.y2 - middleY));
		}

		const int half = static_cast<int>(std::ceil(reach)) + thickest;
		const int side = 2 * half + 1;
		const float pi = 3.14159265358979323846f;

		const float angle = 2.0f * pi * static_cast<float>(wholeDegrees(degrees)) / 360.0f;
		const float c = std::cos(angle);
		const float s = std::sin(angle);

		// A clockwise turn on a screen, where y grows downward.
		const auto place = [&](float x, float y)
		{
			const float dx = x - middleX;
			const float dy = y - middleY;
			return std::pair<float, float>{ static_cast<float>(half) + dx * c - dy * s, static_cast<float>(half) + dx * s + dy * c };
		};

		std::vector<LineSegment> turnedLines = lines;
		for (LineSegment& line : turnedLines)
		{
			const auto from = place(line.x1, line.y1);
			const auto to = place(line.x2, line.y2);
			line.x1 = from.first;
			line.y1 = from.second;
			line.x2 = to.first;
			line.y2 = to.second;
		}

		return rasterizeLines(turnedLines, side, side);
	}
}
