// pictures.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// The pictures a generated game draws itself when it starts, rather than
// loading them from a file: one written as rows of text (a '.' is a clear
// pixel and a '*' a solid one) and one made of straight lines, and either one
// turned to a heading, drawn again for each whole degree as the engine does.
// They are drawn by bitmap.h and bitmap.cpp, the engine's own, copied in as
// they are, so a generated game's pictures are pixel for pixel the engine's;
// all that is here is handing a picture over as an sf::Image, to be loaded into
// a texture. Header only, and nothing but SFML's graphics types and bitmap.h.

#pragma once

#include "bitmap.h"

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

	// A picture the engine drew, as an sf::Image (a clear one pixel image when
	// there is nothing drawn).
	inline sf::Image imageOf(const xge::Bitmap& picture)
	{
		if (picture.rgba.empty() || picture.width < 1 || picture.height < 1)
		{
			return sf::Image({1, 1}, sf::Color::Transparent);
		}
		return sf::Image(sf::Vector2u(static_cast<unsigned int>(picture.width), static_cast<unsigned int>(picture.height)), picture.rgba.data());
	}

	// An sf::Image as a picture the engine can turn.
	inline xge::Bitmap bitmapOf(const sf::Image& image)
	{
		xge::Bitmap picture;
		picture.width = static_cast<int>(image.getSize().x);
		picture.height = static_cast<int>(image.getSize().y);
		picture.rgba.assign(image.getPixelsPtr(), image.getPixelsPtr() + static_cast<std::size_t>(picture.width) * static_cast<std::size_t>(picture.height) * 4);
		return picture;
	}

	inline std::vector<xge::LineSegment> segmentsOf(const std::vector<Line>& drawing)
	{
		std::vector<xge::LineSegment> segments;
		for (const Line& line : drawing)
		{
			xge::LineSegment segment;
			segment.x1 = line.from.x;
			segment.y1 = line.from.y;
			segment.x2 = line.to.x;
			segment.y2 = line.to.y;
			segment.color = xge::Color{ line.color.r, line.color.g, line.color.b, line.color.a };
			segment.thickness = line.thickness;
			segments.push_back(segment);
		}
		return segments;
	}

	// Rows of text, one character to a pixel, every character a scale by scale
	// block. All the rows are the same width.
	inline sf::Image rows(const std::vector<std::string>& text, unsigned int scale, sf::Color color)
	{
		return imageOf(xge::rasterizeRows(text, static_cast<int>(scale), xge::Color{ color.r, color.g, color.b, color.a }));
	}

	// The lines, in order (a later one covers an earlier one), each end
	// rounded to a whole pixel, on a picture just big enough to hold them (and
	// at least `least` big).
	inline sf::Image lines(const std::vector<Line>& drawing, sf::Vector2u least = {})
	{
		return imageOf(xge::rasterizeLines(segmentsOf(drawing), static_cast<int>(least.x), static_cast<int>(least.y)));
	}

	// A heading as the whole degrees, from 0 up to 359, it is drawn at.
	inline int wholeDegrees(float degrees)
	{
		long whole = std::lround(degrees) % 360;
		if (whole < 0)
		{
			whole += 360;
		}
		return static_cast<int>(whole);
	}

	// The lines turned clockwise by `degrees` about the middle of the box their
	// ends lie in, on a square big enough for them at any heading, the middle
	// of the drawing at the middle of the square: so a thing that turns stays
	// where it is and keeps its size.
	inline sf::Image turnedLines(const std::vector<Line>& drawing, float degrees)
	{
		return imageOf(xge::rasterizeTurned(segmentsOf(drawing), degrees));
	}

	// A picture turned clockwise by `degrees`, each pixel the one of the
	// picture that lies under it (no blending), on a square big enough for it
	// at any heading, the middle of the picture at the middle of the square.
	inline sf::Image turned(const sf::Image& picture, float degrees)
	{
		return imageOf(xge::turnBitmap(bitmapOf(picture), degrees));
	}
}
