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
		return backBuffer.isNull() ? QSize(640, 360) : backBuffer.size() * shownScale;
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

		// As large as fits, centred, with the game's own proportions.
		const double scale = std::min(static_cast<double>(width()) / backBuffer.width(), static_cast<double>(height()) / backBuffer.height());
		const double w = backBuffer.width() * scale;
		const double h = backBuffer.height() * scale;
		const QRectF target((width() - w) / 2, (height() - h) / 2, w, h);

		painter.setRenderHint(QPainter::SmoothPixmapTransform, scale < 1.0);
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
		const double scale = std::min(static_cast<double>(width()) / backBuffer.width(), static_cast<double>(height()) / backBuffer.height());
		const double left = (width() - backBuffer.width() * scale) / 2;
		const double top = (height() - backBuffer.height() * scale) / 2;
		const QPointF at = mapFromGlobal(QCursor::pos());
		const double x = (at.x() - left) / scale;
		const double y = (at.y() - top) / scale;
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
