// shapes.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "picture/shapes.h"

#include <plutovg.h>

#include <cmath>
#include <cstddef>

namespace xge
{
	namespace
	{
		int wholePixels(float length, float scale, const char* what)
		{
			const double pixels = std::ceil(static_cast<double>(length) * static_cast<double>(scale));
			refusePictureSize(pixels, 1.0, what);
			return pixels < 1.0 ? 1 : static_cast<int>(pixels);
		}
	}

	Bitmap drawCircle(float radius, const Color& color, float scale)
	{
		const int side = wholePixels(radius * 2.0f, scale, "a circle");
		refusePictureSize(side, side, "a circle");

		Bitmap bitmap;
		bitmap.width = side;
		bitmap.height = side;
		bitmap.rgba.resize(static_cast<std::size_t>(side) * static_cast<std::size_t>(side) * 4);

		plutovg_surface_t* surface = plutovg_surface_create(side, side);
		plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
		const float r = radius * scale;
		plutovg_canvas_circle(canvas, r, r, r);
		plutovg_canvas_set_rgba(canvas, color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
		plutovg_canvas_fill(canvas);
		plutovg_canvas_destroy(canvas);

		plutovg_convert_argb_to_rgba(bitmap.rgba.data(), plutovg_surface_get_data(surface), side, side, plutovg_surface_get_stride(surface));
		plutovg_surface_destroy(surface);
		return bitmap;
	}

	Bitmap drawRectangle(float width, float height, const Color& color, float scale)
	{
		const int wide = wholePixels(width, scale, "a rectangle");
		const int high = wholePixels(height, scale, "a rectangle");
		refusePictureSize(wide, high, "a rectangle");

		Bitmap bitmap;
		bitmap.width = wide;
		bitmap.height = high;
		bitmap.rgba.resize(static_cast<std::size_t>(wide) * static_cast<std::size_t>(high) * 4);
		for (std::size_t at = 0; at < bitmap.rgba.size(); at += 4)
		{
			bitmap.rgba[at] = color.r;
			bitmap.rgba[at + 1] = color.g;
			bitmap.rgba[at + 2] = color.b;
			bitmap.rgba[at + 3] = color.a;
		}
		return bitmap;
	}
}
