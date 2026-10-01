// native_surface.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "key_queue.h"

#include <QWidget>

namespace xge
{
	// An empty widget with a platform window of its own (an HWND on Windows),
	// for a window library that can draw into a window someone else made (SFML3
	// and SDL2 can: see Embedding in window.h) to draw into. Qt paints nothing
	// on it. It is as large as fits the pane, in the game's proportions: the
	// library is told to draw the whole game scaled to whatever size this is
	// (see WindowTarget in window.h). The keyboard is read from here by Qt (into the GameStage's
	// KeyQueue), not by the library: Qt is what has the focus.
	class NativeSurface : public QWidget
	{
	public:
		NativeSurface(KeyQueue& keys, QWidget* parent = nullptr);

		// The game's size in pixels. The widget starts out that big; fitTo
		// then scales it to the pane.
		void setGameSize(int width, int height);

		// Resizes to the largest size in the game's proportions that fits in
		// the area.
		void fitTo(const QSize& area);

		// The platform's window handle, as a library takes it (see
		// WindowTarget::nativeHandle). Makes the platform window if it is not
		// there yet.
		void* nativeHandle();

		// Qt has nothing to draw here, and says so, so it does not paint over
		// what the library drew.
		QPaintEngine* paintEngine() const override { return nullptr; }

	protected:
		void keyPressEvent(QKeyEvent* event) override;
		void keyReleaseEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void focusOutEvent(QFocusEvent* event) override;
		bool focusNextPrevChild(bool next) override;

	private:
		int gameWidth{ 1 };
		int gameHeight{ 1 };
		KeyQueue& keys;
	};
}
