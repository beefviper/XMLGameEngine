// game_view.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_view.h"

#include <QColor>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include <algorithm>

namespace xge
{
	GameView::GameView(KeyQueue& keys, QWidget* parent) :
		GameViewBase(parent),
		keys(keys)
	{
		setFocusPolicy(Qt::StrongFocus);
#ifndef XGE_QT_OPENGL
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAutoFillBackground(false);
#endif
	}

	void GameView::setGameSize(int width, int height)
	{
		backBuffer = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
		backBuffer.fill(Qt::black);
		updateGeometry();
		update();
	}

	QSize GameView::sizeHint() const
	{
		return backBuffer.isNull() ? QSize(640, 360) : backBuffer.size();
	}

	QSize GameView::minimumSizeHint() const
	{
		return QSize(160, 90);
	}

	void GameView::present()
	{
		update();
	}

	void GameView::showFrame(const Bitmap& bitmap)
	{
		if (bitmap.rgba.empty() || bitmap.width != backBuffer.width() || bitmap.height != backBuffer.height())
		{
			return;
		}

		// The pixels are opaque, so their alpha byte is ignored.
		const QImage wrapped(bitmap.rgba.data(), bitmap.width, bitmap.height, bitmap.width * 4, QImage::Format_RGBX8888);

		QPainter painter(&backBuffer);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(0, 0, wrapped);
		painter.end();

		update();
	}

	std::vector<std::pair<KeyCode, bool>> GameView::takeKeyEvents()
	{
		return keys.take();
	}

#ifdef XGE_QT_OPENGL
	void GameView::paintGL()
	{
		QPainter painter(this);
		paintFrame(painter);
	}
#else
	void GameView::paintEvent(QPaintEvent*)
	{
		QPainter painter(this);
		paintFrame(painter);
	}
#endif

	void GameView::paintFrame(QPainter& painter)
	{
		painter.fillRect(rect(), QColor(0x20, 0x20, 0x20));

		if (backBuffer.isNull())
		{
			return;
		}

		// As large as fits, centred, with the game's own proportions.
		const double scale = std::min(static_cast<double>(width()) / backBuffer.width(), static_cast<double>(height()) / backBuffer.height());
		const double w = backBuffer.width() * scale;
		const double h = backBuffer.height() * scale;
		const QRectF target((width() - w) / 2, (height() - h) / 2, w, h);

		painter.setRenderHint(QPainter::SmoothPixmapTransform, scale < 1.0);
		painter.drawImage(target, backBuffer);
	}

	void GameView::keyPressEvent(QKeyEvent* event)
	{
		if (keys.press(*event))
		{
			event->accept();
		}
		else
		{
			GameViewBase::keyPressEvent(event);
		}
	}

	void GameView::keyReleaseEvent(QKeyEvent* event)
	{
		if (keys.release(*event))
		{
			event->accept();
		}
		else
		{
			GameViewBase::keyReleaseEvent(event);
		}
	}

	void GameView::mousePressEvent(QMouseEvent* event)
	{
		setFocus();
		event->accept();
	}

	void GameView::focusOutEvent(QFocusEvent* event)
	{
		keys.releaseAll();
		GameViewBase::focusOutEvent(event);
	}

	bool GameView::focusNextPrevChild(bool)
	{
		return false;
	}
}
