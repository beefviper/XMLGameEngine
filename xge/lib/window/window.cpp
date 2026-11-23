// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window/window.h"
#include "picture/color.h"
#include "picture/font.h"
#include "picture/shapes.h"

#include <cmath>
#include <cstdio>

namespace xge
{
	namespace
	{
		// A number in a picture's key, exactly.
		std::string keyNumber(float number)
		{
			char text[32];
			const int written = std::snprintf(text, sizeof text, "%.9g", static_cast<double>(number));
			return std::string(text, static_cast<std::size_t>(written));
		}
	}

	PictureId Window::pictureOf(const Object& object)
	{
		const PictureId shown = pictureNow(object);
		return shown.get() ? shown : gamePictures->picture("nothing", [] { return Bitmap{}; });
	}

	PictureId Window::pictureNow(const Object& object)
	{
		const Picture& picture = object.drawn.picture;
		const float scale = pictureScale();
		switch (picture.kind)
		{
		case ShapeKind::Line:
			if (object.drawn.bitmap) { return object.drawn.bitmap; }
			break;
		case ShapeKind::Image:
			if (picture.image) { return picture.image; }
			break;
		case ShapeKind::Circle:
			return gamePictures->picture("circle|" + keyNumber(picture.radius) + "|" + picture.color.str() + "|" + keyNumber(scale),
				[&] { return drawCircle(picture.radius, colorFromName(picture.color), scale); });
		case ShapeKind::Rectangle:
			return gamePictures->picture("rectangle|" + keyNumber(picture.size.x) + "," + keyNumber(picture.size.y) + "|" + picture.color.str() + "|" + keyNumber(scale),
				[&] { return drawRectangle(picture.size.x, picture.size.y, colorFromName(picture.color), scale); });
		case ShapeKind::Text:
		{
			const int pixels = std::max(1, static_cast<int>(std::lround(static_cast<float>(picture.textSize) * scale)));
			const std::string from = picture.text + '\n' + picture.font.str() + '\n' + std::to_string(pixels) + '\n' + picture.color.str();
			DrawnText& text = texts[object.id.index];
			if (text.picture && text.drawnFrom == from) { return text.picture; }
			Bitmap drawn = drawText(picture.text, pixels, colorFromName(picture.color), textFontFile(picture.font));
			if (text.picture) { gamePictures->replace(text.picture, std::move(drawn)); }
			else { text.picture = gamePictures->own(std::move(drawn)); }
			text.drawnFrom = from;
			return text.picture;
		}
		case ShapeKind::Unknown:
			break;
		}
		return gamePictures->picture("nothing", [] { return Bitmap{}; });
	}
}
