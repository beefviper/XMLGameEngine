// native_surface.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "native_surface.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include <algorithm>
#include <cstdint>

namespace xge
{
	NativeSurface::NativeSurface(KeyQueue& keys, QWidget* parent) :
		QWidget(parent),
		keys(keys)
	{
		// Only this widget gets a platform window of its own. Without
		// WA_DontCreateNativeAncestors, Qt makes every widget above it native
		// too (and, without Qt::AA_DontCreateNativeWidgetSiblings, every widget
		// beside those: see main.cpp), and when the game view is a
		// QOpenGLWidget, Qt draws the main window with OpenGL and cannot make its
		// context current on those plain native windows; it then draws with
		// whatever OpenGL context is current, which can be a window library's.
		setAttribute(Qt::WA_DontCreateNativeAncestors);
		setAttribute(Qt::WA_NativeWindow);
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setFocusPolicy(Qt::StrongFocus);
	}

	void NativeSurface::setGameSize(int width, int height)
	{
		gameWidth = width > 0 ? width : 1;
		gameHeight = height > 0 ? height : 1;
		resize(gameWidth, gameHeight);
	}

	void NativeSurface::fitTo(const QSize& area)
	{
		const double scale = std::min(static_cast<double>(area.width()) / gameWidth,
			static_cast<double>(area.height()) / gameHeight);
		const int width = std::max(1, static_cast<int>(gameWidth * scale));
		const int height = std::max(1, static_cast<int>(gameHeight * scale));

		if (width != this->width() || height != this->height())
		{
			resize(width, height);
		}
	}

	void* NativeSurface::nativeHandle()
	{
		return reinterpret_cast<void*>(static_cast<std::uintptr_t>(winId()));
	}

	void NativeSurface::keyPressEvent(QKeyEvent* event)
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

	void NativeSurface::keyReleaseEvent(QKeyEvent* event)
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

	void NativeSurface::mousePressEvent(QMouseEvent* event)
	{
		setFocus();
		event->accept();
	}

	void NativeSurface::focusOutEvent(QFocusEvent* event)
	{
		keys.releaseAll();
		QWidget::focusOutEvent(event);
	}

	bool NativeSurface::focusNextPrevChild(bool)
	{
		return false;
	}
}
