// qt_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "qt_window.h"

#include "builtin_font.h"
#include "color.h"
#include "game_view.h"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
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

	QtWindow::QtWindow(GameView& view) :
		view(view)
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

	void QtWindow::clear(const std::string& colorName)
	{
		view.frame().fill(toQColor(colorName));
	}

	void QtWindow::draw(Object& object)
	{
		QImage& visual = visuals[object.name];

		if (object.visualDirty || visual.isNull())
		{
			build(object, visual);
		}

		QPainter painter(&view.frame());
		painter.drawImage(QPointF(object.position.x, object.position.y), visual);
	}

	void QtWindow::display()
	{
		view.present();
	}

	void QtWindow::build(Object& object, QImage& visual)
	{
		switch (object.shapeKind)
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
		const double radius = std::stod(object.spriteParams.at(1));
		const int side = static_cast<int>(std::ceil(radius * 2));

		QImage image = emptyImage(side, side);
		QPainter painter(&image);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(Qt::NoPen);
		painter.setBrush(toQColor(object.spriteParams.at(3)));
		painter.drawEllipse(QRectF(0, 0, radius * 2, radius * 2));
		return image;
	}

	QImage QtWindow::buildRectangle(const Object& object)
	{
		const double width = std::stod(object.spriteParams.at(1));
		const double height = std::stod(object.spriteParams.at(2));

		QImage image = emptyImage(static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)));
		QPainter painter(&image);
		painter.fillRect(QRectF(0, 0, width, height), toQColor(object.spriteParams.at(3)));
		return image;
	}

	void QtWindow::loadFont()
	{
		if (fontLoaded)
		{
			return;
		}
		fontLoaded = true;

		const std::string fontFile{ "assets/tuffy.ttf" };
		const int id = QFontDatabase::addApplicationFont(QString::fromStdString(fontFile));
		if (id >= 0 && !QFontDatabase::applicationFontFamilies(id).isEmpty())
		{
			fontFamily = QFontDatabase::applicationFontFamilies(id).first();
		}
		else
		{
			std::cout << "error: failed to load font: " << fontFile << " - drawing text with the built-in 8x8 font instead" << std::endl;
		}
	}

	QImage QtWindow::buildText(const Object& object)
	{
		loadFont();

		const std::string& content = object.spriteParams.at(1);
		const int size = std::stoi(object.spriteParams.at(2));

		if (fontFamily.isEmpty())
		{
			return fromBitmap(rasterizeText(content, size, colorFromName(object.spriteParams.at(3))));
		}

		QFont font(fontFamily);
		font.setPixelSize(size);

		const QString text = QString::fromStdString(content);
		const QFontMetricsF metrics(font);
		const QRectF bounds = metrics.tightBoundingRect(text);

		QImage image = emptyImage(static_cast<int>(std::ceil(bounds.width())), static_cast<int>(std::ceil(bounds.height())));
		QPainter painter(&image);
		painter.setRenderHint(QPainter::TextAntialiasing);
		painter.setFont(font);
		painter.setPen(toQColor(object.spriteParams.at(3)));

		// The text is drawn from its baseline; shift it so its tight bounds
		// start at the top left of the picture.
		painter.drawText(QPointF(-bounds.left(), -bounds.top()), text);
		return image;
	}

	QImage QtWindow::buildImage(const Object& object)
	{
		const std::string& imageFile = object.spriteParams.at(1);

		QImage image(QString::fromStdString(imageFile));
		if (image.isNull())
		{
			std::cout << "error: Qt Image: failed to load " << imageFile << '\n';
			return emptyImage(1, 1);
		}

		image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

		const std::string& flip = object.spriteParams.at(2);
		if (flip == "flip.horizontal")
		{
			image = image.mirrored(true, false);
		}
		else if (flip == "flip.vertical")
		{
			image = image.mirrored(false, true);
		}

		return image;
	}

	// A picture the engine drew itself (a sprite of lines, or text in the
	// built-in font): four bytes a pixel, red, green, blue, alpha.
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
