// game_view.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_view.h"

#include <QColor>
#include <QCursor>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace xge
{
	GameView::GameView(KeyQueue& keyQueue, QWidget* parent) :
		QWidget(parent),
		keys(keyQueue)
	{
		setFocusPolicy(Qt::StrongFocus);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAutoFillBackground(false);
	}

	void GameView::setGameSize(int width, int height, int scale)
	{
		shownScale = std::max(scale, 1);
		backBuffer = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
		backBuffer.fill(Qt::black);
		updateGeometry();
		update();
	}

	QSize GameView::sizeHint() const
	{
		// In screen pixels, not Qt's (scaled by the display's setting), so a
		// game opens at its own size, each pixel one on the screen.
		if (backBuffer.isNull()) { return QSize(640, 360); }
		const double ratio = devicePixelRatioF();
		return QSize(static_cast<int>(std::ceil(backBuffer.width() * shownScale / ratio)), static_cast<int>(std::ceil(backBuffer.height() * shownScale / ratio)));
	}

	QRectF GameView::shownRect() const
	{
		const double ratio = devicePixelRatioF();
		const double wide = width() * ratio;
		const double high = height() * ratio;
		const double fit = std::min(wide / backBuffer.width(), high / backBuffer.height());
		const double scale = fit >= 1.0 ? std::floor(fit) : fit;
		const double w = backBuffer.width() * scale;
		const double h = backBuffer.height() * scale;
		// On whole screen pixels, so no column of game pixels is drawn wider
		// than the next.
		const double left = std::floor((wide - w) / 2);
		const double top = std::floor((high - h) / 2);
		return QRectF(left / ratio, top / ratio, w / ratio, h / ratio);
	}

	QSize GameView::minimumSizeHint() const
	{
		return QSize(160, 90);
	}

	void GameView::present()
	{
		update();
	}

	std::vector<std::pair<KeyCode, bool>> GameView::takeKeyEvents()
	{
		return keys.take();
	}

	void GameView::paintEvent(QPaintEvent*)
	{
		QPainter painter(this);
		painter.fillRect(rect(), QColor(0x20, 0x20, 0x20));

		if (backBuffer.isNull())
		{
			return;
		}

		const QRectF target = shownRect();
		painter.setRenderHint(QPainter::SmoothPixmapTransform, target.width() * devicePixelRatioF() < backBuffer.width());
		painter.drawImage(target, backBuffer);
	}

	Controls GameView::controls() const
	{
		Controls controls;
		if (backBuffer.isNull() || !hasFocus())
		{
			return controls;
		}

		// The picture's place in the widget, as paintEvent draws it.
		const QRectF shown = shownRect();
		const QPointF at = mapFromGlobal(QCursor::pos());
		const double x = (at.x() - shown.left()) * backBuffer.width() / shown.width();
		const double y = (at.y() - shown.top()) * backBuffer.height() / shown.height();
		controls.pointerInside = x >= 0 && y >= 0 && x < backBuffer.width() && y < backBuffer.height();
		controls.pointerX = static_cast<float>(x);
		controls.pointerY = static_cast<float>(y);

		const Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
		controls.mouse = { buttons.testFlag(Qt::LeftButton), buttons.testFlag(Qt::RightButton), buttons.testFlag(Qt::MiddleButton) };
		return controls;
	}

	void GameView::keyPressEvent(QKeyEvent* event)
	{
		if (keys.press(*event))
		{
			event->accept();
		}
		else
		{
			QWidget::keyPressEvent(event);
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
			QWidget::keyReleaseEvent(event);
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
		QWidget::focusOutEvent(event);
	}

	bool GameView::focusNextPrevChild(bool)
	{
		return false;
	}
}
