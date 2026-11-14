// qt_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "qt_window.h"

#include "picture/color.h"
#include "game_view.h"

#include <QColor>
#include <QPainter>

#include <memory>

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
		setPictureScale(static_cast<float>(view.shownFactor()));
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

	float QtWindow::fit() const
	{
		return static_cast<float>(view.shownFactor());
	}

	void QtWindow::clear(const std::string& colorName)
	{
		// A painter still open from a frame that never reached display() has
		// to end before the picture is touched.
		framePainter.reset();

		// The frame is drawn at the pictures' scale, in the game's pixels.
		view.setFrameScale(pictureScale());
		view.frame().fill(toQColor(colorName));
		framePainter = std::make_unique<QPainter>(&view.frame());
		framePainter->scale(view.frameScale(), view.frameScale());
	}

	void QtWindow::draw(Object& object)
	{
		Visual& visual = visuals[object.name];

		if (object.visualDirty || visual.image.isNull() || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}

		// Stretched over the object's size in the game's pixels: smoothed
		// for a text or a shape, hard edged for a bitmap.
		const Vector2f size = object.pictureSize();
		const QRectF where(object.position.x, object.position.y, size.x, size.y);
		if (framePainter)
		{
			framePainter->setRenderHint(QPainter::SmoothPixmapTransform, object.drawnAtScale());
			framePainter->drawImage(where, visual.image);
			return;
		}
		QPainter painter(&view.frame());
		painter.scale(view.frameScale(), view.frameScale());
		painter.setRenderHint(QPainter::SmoothPixmapTransform, object.drawnAtScale());
		painter.drawImage(where, visual.image);
	}

	void QtWindow::display()
	{
		framePainter.reset();
		view.present();
	}

	// The engine's picture of the object, at pictureScale().
	void QtWindow::build(Object& object, Visual& visual)
	{
		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		if (picture->width <= 0 || picture->height <= 0 || picture->rgba.empty())
		{
			visual.image = emptyImage(1, 1);
		}
		else
		{
			const QImage wrapped(picture->rgba.data(), picture->width, picture->height, picture->width * 4, QImage::Format_RGBA8888);
			visual.image = wrapped.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		}
		visual.drawnAt = pictureScale();
		object.visualDirty = false;
	}
}
