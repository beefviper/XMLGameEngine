// picture.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/picture.h"

#include "picture/color.h"

#include <plutovg.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace xge
{
	PictureWindow::PictureWindow(const WindowDesc& windowDesc) :
		scale(static_cast<float>(std::max(1, windowDesc.scale)))
	{
		frame.width = std::max(1, static_cast<int>(std::lround(windowDesc.width * scale)));
		frame.height = std::max(1, static_cast<int>(std::lround(windowDesc.height * scale)));
		frame.rgba.assign(static_cast<std::size_t>(frame.width) * static_cast<std::size_t>(frame.height) * 4, 0);
		setPictureScale(scale);
	}

	void PictureWindow::init(std::vector<Object>& objects)
	{
		for (Object& object : objects) { prepare(object); }
	}

	void PictureWindow::clear(const std::string& colorName)
	{
		const Color c = colorFromName(colorName);
		for (std::size_t at = 0; at < frame.rgba.size(); at += 4)
		{
			frame.rgba[at] = c.r;
			frame.rgba[at + 1] = c.g;
			frame.rgba[at + 2] = c.b;
			frame.rgba[at + 3] = 255;
		}
	}

	void PictureWindow::draw(Object& object)
	{
		prepare(object);
		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		if (picture->rgba.empty() || picture->width <= 0 || picture->height <= 0) { return; }

		// Where it goes, in the frame's pixels: from a whole game pixel,
		// stretched over its size, as a window draws it. Each pixel there
		// takes the picture's pixel under its centre, laid over what is
		// already drawn by its alpha.
		const Vector2f size = object.pictureSize();
		const float left = std::floor(object.position.x) * scale;
		const float top = std::floor(object.position.y) * scale;
		const float wide = size.x * scale;
		const float high = size.y * scale;
		if (wide <= 0.0f || high <= 0.0f) { return; }
		const int fromX = std::max(0, static_cast<int>(std::lround(left)));
		const int fromY = std::max(0, static_cast<int>(std::lround(top)));
		const int toX = std::min(frame.width, static_cast<int>(std::lround(left + wide)));
		const int toY = std::min(frame.height, static_cast<int>(std::lround(top + high)));

		for (int y = fromY; y < toY; ++y)
		{
			const int pictureY = std::clamp(static_cast<int>((static_cast<float>(y) + 0.5f - top) / high * static_cast<float>(picture->height)), 0, picture->height - 1);
			for (int x = fromX; x < toX; ++x)
			{
				const int pictureX = std::clamp(static_cast<int>((static_cast<float>(x) + 0.5f - left) / wide * static_cast<float>(picture->width)), 0, picture->width - 1);
				const std::uint8_t* from = picture->rgba.data() + (static_cast<std::size_t>(pictureY) * static_cast<std::size_t>(picture->width) + static_cast<std::size_t>(pictureX)) * 4;
				std::uint8_t* to = frame.rgba.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.width) + static_cast<std::size_t>(x)) * 4;
				const unsigned alpha = from[3];
				for (int channel = 0; channel < 3; ++channel)
				{
					to[channel] = static_cast<std::uint8_t>((from[channel] * alpha + to[channel] * (255u - alpha) + 127u) / 255u);
				}
			}
		}
	}

	bool writePng(const Bitmap& picture, const std::string& file)
	{
		if (picture.width <= 0 || picture.height <= 0) { return false; }
		plutovg_surface_t* surface = plutovg_surface_create(picture.width, picture.height);
		if (!surface) { return false; }
		plutovg_convert_rgba_to_argb(plutovg_surface_get_data(surface), picture.rgba.data(), picture.width, picture.height, plutovg_surface_get_stride(surface));
		const bool written = plutovg_surface_write_to_png(surface, file.c_str());
		plutovg_surface_destroy(surface);
		return written;
	}
}
