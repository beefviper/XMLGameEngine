// native_surface.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "native_surface.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include <cstdint>

namespace xge
{
	NativeSurface::NativeSurface(KeyQueue& keys, QWidget* parent) :
		QWidget(parent),
		keys(keys)
	{
		setAttribute(Qt::WA_NativeWindow);
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setFocusPolicy(Qt::StrongFocus);
	}

	void NativeSurface::setGameSize(int width, int height)
	{
		setFixedSize(width, height);
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
