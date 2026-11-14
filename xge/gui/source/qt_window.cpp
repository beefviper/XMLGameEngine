// qt_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "qt_window.h"

#include "picture/font.h"
#include "picture/image.h"
#include "picture/color.h"
#include "game_view.h"

#include <QColor>
#include <QPainter>

#include <cmath>
#include <iostream>

namespace xge
{
	namespace
	{
		QColor toQColor(const std::string& colorName)
		{
			const Color c = colorFromName(colorName);
			return QColor(c.r, c.g, c.b, c.a);
		}

		QImage emptyImage(int width, int height)
		{
			QImage image(width > 0 ? width : 1, height > 0 ? height : 1, QImage::Format_ARGB32_Premultiplied);
			image.fill(Qt::transparent);
			return image;
		}
	}

	QtWindow::QtWindow(GameView& gameView) :
		view(gameView)
	{
	}

	bool QtWindow::isOpen() const
	{
		return open;
	}

	void QtWindow::close()
	{
		open = false;
	}

	void QtWindow::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
		}
	}

	std::vector<std::pair<KeyCode, bool>> QtWindow::pollEvents()
	{
		return view.takeKeyEvents();
	}

	Controls QtWindow::controls()
	{
		return view.controls();
	}

	void QtWindow::clear(const std::string& colorName)
	{
		// A painter still open from a frame that never reached display() has
		// to end before the picture is touched.
		framePainter.reset();

		view.frame().fill(toQColor(colorName));
		framePainter = std::make_unique<QPainter>(&view.frame());
	}

	void QtWindow::draw(Object& object)
	{
		QImage& visual = visuals[object.name];

		if (object.visualDirty || visual.isNull())
		{
			build(object, visual);
		}

		if (framePainter)
		{
			framePainter->drawImage(QPointF(object.position.x, object.position.y), visual);
		}
		else
		{
			QPainter painter(&view.frame());
			painter.drawImage(QPointF(object.position.x, object.position.y), visual);
		}
	}

	void QtWindow::display()
	{
		framePainter.reset();
		view.present();
	}

	void QtWindow::build(Object& object, QImage& visual)
	{
		switch (object.picture.kind)
		{
		case ShapeKind::Circle:    visual = buildCircle(object); break;
		case ShapeKind::Rectangle: visual = buildRectangle(object); break;
		case ShapeKind::Text:      visual = buildText(object); break;
		case ShapeKind::Image:     visual = buildImage(object); break;
		case ShapeKind::Line:      visual = object.bitmap ? fromBitmap(*object.bitmap) : emptyImage(1, 1); break;
		case ShapeKind::Unknown:   visual = emptyImage(1, 1); break;
		}

		object.size.x = static_cast<float>(visual.width());
		object.size.y = static_cast<float>(visual.height());
		object.sizeKnown = true;
		object.visualDirty = false;
	}

	QImage QtWindow::buildCircle(const Object& object)
	{
		const double radius = object.picture.radius;
		const int side = static_cast<int>(std::ceil(radius * 2));

		QImage image = emptyImage(side, side);
		QPainter painter(&image);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(Qt::NoPen);
		painter.setBrush(toQColor(object.picture.color));
		painter.drawEllipse(QRectF(0, 0, radius * 2, radius * 2));
		return image;
	}

	QImage QtWindow::buildRectangle(const Object& object)
	{
		const double width = object.picture.size.x;
		const double height = object.picture.size.y;

		QImage image = emptyImage(static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)));
		QPainter painter(&image);
		painter.fillRect(QRectF(0, 0, width, height), toQColor(object.picture.color));
		return image;
	}

	QImage QtWindow::buildText(const Object& object)
	{
		return fromBitmap(drawText(object.picture.text, object.picture.textSize, colorFromName(object.picture.color), textFontFile(object.picture.font)));
	}

	QImage QtWindow::buildImage(const Object& object)
	{
		return fromBitmap(loadPicture(object.picture.file, object.picture.flipHorizontal, object.picture.flipVertical));
	}

	// A picture the engine drew itself (a sprite of lines, or a text): four
	// bytes a pixel, red, green, blue, alpha.
	QImage QtWindow::fromBitmap(const Bitmap& bitmap)
	{
		if (bitmap.width <= 0 || bitmap.height <= 0 || bitmap.rgba.empty())
		{
			return emptyImage(bitmap.width, bitmap.height);
		}

		const QImage wrapped(bitmap.rgba.data(), bitmap.width, bitmap.height, bitmap.width * 4, QImage::Format_RGBA8888);
		return wrapped.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	}
}
