// qt_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "qt_window.h"

#include "picture/color.h"
#include "game_view.h"

#include <QColor>
#include <QPainter>

#include <algorithm>
#include <cmath>
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
		// The frame is drawn at the pictures' scale, in the game's pixels.
		view.setFrameScale(pictureScale());
		frame.background = toQColor(colorName);
		frame.scale = view.frameScale();
		frame.placed.clear();
	}

	void QtWindow::draw(Object& object)
	{
		Visual& visual = visuals[object.name];

		if (object.drawn.visualDirty || visual.image.isNull() || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}

		// The screen pixels it covers: its size in the game's pixels at the
		// frame's scale.
		const double scale = frame.scale;
		const Vector2f size = object.pictureSize();
		const int left = static_cast<int>(std::lround(static_cast<double>(object.position.x) * scale));
		const int top = static_cast<int>(std::lround(static_cast<double>(object.position.y) * scale));
		const int right = static_cast<int>(std::lround(static_cast<double>(object.position.x + size.x) * scale));
		const int bottom = static_cast<int>(std::lround(static_cast<double>(object.position.y + size.y) * scale));
		const QRect where(left, top, std::max(right - left, 1), std::max(bottom - top, 1));

		// Drawn as it is when it is that size already; else stretched once,
		// smoothed for a text or a shape, hard edged for a bitmap, and kept.
		if (visual.image.size() == where.size())
		{
			frame.placed.push_back({ visual.image, where });
			return;
		}
		if (visual.sized.size() != where.size() || visual.sizedFrom != visual.image.cacheKey())
		{
			visual.sized = visual.image.scaled(where.size(), Qt::IgnoreAspectRatio, object.drawnAtScale() ? Qt::SmoothTransformation : Qt::FastTransformation);
			visual.sizedFrom = visual.image.cacheKey();
		}
		frame.placed.push_back({ visual.sized, where });
	}

	void QtWindow::display()
	{
		if (frame == painted)
		{
			return;
		}
		QImage& picture = view.frame();
		picture.fill(frame.background);
		QPainter painter(&picture);
		for (const Placed& each : frame.placed)
		{
			painter.drawImage(each.where.topLeft(), each.image);
		}
		painter.end();
		painted = frame;
		view.present();
	}

	// The engine's picture of the object, at pictureScale().
	void QtWindow::build(Object& object, Visual& visual)
	{
		visual.image = images.of(pictureOf(object), [](const Bitmap& picture, QImage& made)
		{
			if (picture.width <= 0 || picture.height <= 0 || picture.rgba.empty())
			{
				made = emptyImage(1, 1);
			}
			else
			{
				const QImage wrapped(picture.rgba.data(), picture.width, picture.height, picture.width * 4, QImage::Format_RGBA8888);
				made = wrapped.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			}
		});
		visual.drawnAt = pictureScale();
		object.drawn.visualDirty = false;
	}
}
