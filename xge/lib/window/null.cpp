// null.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "window/null.h"

#include "picture/color.h"
#include "picture/font.h"
#include "picture/image.h"

#include <cmath>

namespace xge
{
	NullWindow::NullWindow(const WindowDesc& windowDesc)
	{
		(void)windowDesc; // its size is the game's, and nothing is drawn at it
	}

	void NullWindow::init(std::vector<Object>& objects)
	{
		for (Object& object : objects) { prepare(object); }
	}

	void NullWindow::prepare(Object& object)
	{
		if (object.sizeKnown && !object.visualDirty) { return; }
		object.size = measure(object);
		object.sizeKnown = true;
		object.visualDirty = false;
	}

	Vector2f NullWindow::measure(const Object& object)
	{
		const Picture& picture = object.picture;
		switch (picture.kind)
		{
		case ShapeKind::Circle:
		{
			const float across = std::ceil(picture.radius * 2.0f);
			return { across, across };
		}
		case ShapeKind::Rectangle:
			return { std::ceil(picture.size.x), std::ceil(picture.size.y) };
		case ShapeKind::Line:
			return picture.size;
		case ShapeKind::Text:
		{
			const Bitmap drawn = rasterizeText(picture.text, picture.textSize, colorFromName(picture.color));
			return { static_cast<float>(drawn.width), static_cast<float>(drawn.height) };
		}
		case ShapeKind::Image:
		{
			const auto known = imageSizes.find(picture.file);
			if (known != imageSizes.end()) { return known->second; }
			Sheet sheet;
			sheet.name = picture.file;
			sheet.path = picture.file;
			const SheetPicture file(sheet);
			const Vector2f size{ file.width(), file.height() };
			imageSizes.emplace(picture.file, size);
			return size;
		}
		case ShapeKind::Unknown:
		default:
			return {};
		}
	}
}
