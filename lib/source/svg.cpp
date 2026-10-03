// svg.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#include "svg.h"

#include <lunasvg.h>

#include <algorithm>
#include <cctype>
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
		// An id that cannot break out of the style rule it is put in.
		bool isPlainId(const std::string& id)
		{
			if (id.empty()) { return false; }

			return std::all_of(id.begin(), id.end(), [](unsigned char c)
			{
				return std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.' || c == ':';
			});
		}
	}

	Bitmap rasterizeSvg(const std::string& path, const SvgRegion& region, float scale, const std::vector<std::string>& hide)
	{
		if (!(scale > 0.0f)) { throw std::invalid_argument("an svg's <scale> is " + std::to_string(scale) + "; expected a number above 0"); }

		const std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromFile(path);
		if (!document) { throw std::runtime_error("could not read the svg file " + path + " (missing, or not an SVG)"); }

		std::string style;
		for (const std::string& id : hide)
		{
			if (!isPlainId(id)) { throw std::invalid_argument("an svg <hide> of '" + id + "' is not an element id"); }
			style += "#" + id + "{display:none}";
		}
		if (!style.empty()) { document->applyStyleSheet(style); }

		const float drawingWidth = document->width();
		const float drawingHeight = document->height();

		SvgRegion part = region;
		if (region.isWhole())
		{
			if (region.width != 0.0f || region.height != 0.0f || region.x != 0.0f || region.y != 0.0f)
			{
				throw std::invalid_argument("an svg's region needs a width and a height above 0, or none of x, y, width and height at all");
			}
			part = { 0.0f, 0.0f, drawingWidth, drawingHeight };
		}

		if (part.x >= drawingWidth || part.y >= drawingHeight || part.x + part.width <= 0.0f || part.y + part.height <= 0.0f)
		{
			throw std::invalid_argument("an svg's region lies outside the drawing in " + path + ", which is "
				+ std::to_string(drawingWidth) + " by " + std::to_string(drawingHeight));
		}

		const int width = std::max(1, static_cast<int>(std::ceil(part.width * scale - 0.001f)));
		const int height = std::max(1, static_cast<int>(std::ceil(part.height * scale - 0.001f)));

		lunasvg::Bitmap drawn(width, height);
		if (drawn.isNull()) { throw std::runtime_error("could not make a " + std::to_string(width) + " by " + std::to_string(height) + " picture for " + path); }
		drawn.clear(0x00000000);

		// Scale the drawing, and slide the wanted part to the top left.
		document->render(drawn, lunasvg::Matrix(scale, 0.0f, 0.0f, scale, -part.x * scale, -part.y * scale));
		drawn.convertToRGBA();

		Bitmap bitmap;
		bitmap.width = width;
		bitmap.height = height;
		bitmap.rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

		const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
		for (int row = 0; row < height; ++row)
		{
			std::memcpy(bitmap.rgba.data() + static_cast<std::size_t>(row) * rowBytes,
				drawn.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(drawn.stride()), rowBytes);
		}

		return bitmap;
	}
}
