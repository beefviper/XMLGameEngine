// pictures.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The pictures a generated game draws itself when it starts, rather than
// loading them from a file: one written as rows of text (a '.' is a clear
// pixel and a '*' a solid one) and one made of straight lines. Each comes out
// as an sf::Image, pixel for pixel what the engine draws (lib/source/bitmap.cpp),
// to be loaded into a texture. Header only, and nothing but SFML's graphics
// types, so it can be copied into any SFML 3 program.

#pragma once

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace pictures
{
	// One straight line, in pixels from the picture's top left, and how many
	// pixels thick (a square that size is stamped all along it).
	struct Line
	{
		sf::Vector2f from;
		sf::Vector2f to;
		sf::Color color;
		int thickness;
	};

	// A size by size square of the color, its top left at x, y, as far as it
	// is inside the picture.
	inline void stamp(sf::Image& picture, int x, int y, int size, sf::Color color)
	{
		const sf::Vector2u extent = picture.getSize();
		for (int row = y; row < y + size; ++row)
		{
			for (int column = x; column < x + size; ++column)
			{
				if (column >= 0 && row >= 0 && static_cast<unsigned int>(column) < extent.x && static_cast<unsigned int>(row) < extent.y)
				{
					picture.setPixel({static_cast<unsigned int>(column), static_cast<unsigned int>(row)}, color);
				}
			}
		}
	}

	// Rows of text, one character to a pixel, every character a scale by scale
	// block. All the rows are the same width.
	inline sf::Image rows(const std::vector<std::string>& text, unsigned int scale, sf::Color color)
	{
		const unsigned int columns = static_cast<unsigned int>(text.front().size());
		sf::Image picture({columns * scale, static_cast<unsigned int>(text.size()) * scale}, sf::Color::Transparent);
		for (std::size_t row = 0; row < text.size(); ++row)
		{
			for (std::size_t column = 0; column < text[row].size(); ++column)
			{
				if (text[row][column] == '*')
				{
					stamp(picture, static_cast<int>(column * scale), static_cast<int>(row * scale), static_cast<int>(scale), color);
				}
			}
		}
		return picture;
	}

	// The lines, in order (a later one covers an earlier one), each end
	// rounded to a whole pixel, on a picture just big enough to hold them.
	inline sf::Image lines(const std::vector<Line>& drawing)
	{
		const auto rounded = [](sf::Vector2f point)
		{
			return sf::Vector2i(static_cast<int>(std::lround(point.x)), static_cast<int>(std::lround(point.y)));
		};

		int right = 0;
		int bottom = 0;
		for (const Line& line : drawing)
		{
			const int thickness = std::max(line.thickness, 1);
			for (const sf::Vector2i end : {rounded(line.from), rounded(line.to)})
			{
				right = std::max(right, end.x + thickness);
				bottom = std::max(bottom, end.y + thickness);
			}
		}

		sf::Image picture({static_cast<unsigned int>(right), static_cast<unsigned int>(bottom)}, sf::Color::Transparent);
		for (const Line& line : drawing)
		{
			const int thickness = std::max(line.thickness, 1);
			const sf::Vector2i from = rounded(line.from);
			const sf::Vector2i to = rounded(line.to);

			// Bresenham: every pixel from one end to the other, with no gaps.
			const int dx = std::abs(to.x - from.x);
			const int dy = -std::abs(to.y - from.y);
			const int stepX = from.x < to.x ? 1 : -1;
			const int stepY = from.y < to.y ? 1 : -1;
			int error = dx + dy;
			sf::Vector2i at = from;

			while (true)
			{
				stamp(picture, at.x, at.y, thickness, line.color);
				if (at == to)
				{
					break;
				}

				const int doubled = 2 * error;
				if (doubled >= dy)
				{
					error += dy;
					at.x += stepX;
				}
				if (doubled <= dx)
				{
					error += dx;
					at.y += stepY;
				}
			}
		}
		return picture;
	}
}
