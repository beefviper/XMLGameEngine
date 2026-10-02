// game_view.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "key_queue.h"

#include <QImage>
#include <QWidget>

#include <utility>
#include <vector>

class QPainter;

namespace xge
{
	// A widget that shows the picture the QtWindow backend (qt_window.h) draws
	// a frame into, as large as fits without changing its proportions. It owns
	// no game logic. The engine draws into frame() and calls present() when a
	// frame is finished, and the keys go to the GameStage's KeyQueue.
	//
	// An ordinary widget, drawn by Qt's raster engine: nothing in the
	// application uses OpenGL through Qt, so a video library's OpenGL context
	// (SDL2, raylib, GLFW) is the only one on the thread.
	class GameView : public QWidget
	{
	public:
		GameView(KeyQueue& keys, QWidget* parent = nullptr);

		// Sizes the picture to the game's window (WindowDesc). The widget itself
		// can be any size: the picture is scaled to fit it.
		void setGameSize(int width, int height);

		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

		// The picture the engine draws into.
		QImage& frame() noexcept { return backBuffer; }

		// A frame is finished: show it.
		void present();

		// The keys that changed since the last call, as {key, pressed}.
		std::vector<std::pair<KeyCode, bool>> takeKeyEvents();

	protected:
		void paintEvent(QPaintEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;
		void keyReleaseEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void focusOutEvent(QFocusEvent* event) override;

		// Tab is a key a game may use, not a way to leave the view.
		bool focusNextPrevChild(bool next) override;

	private:
		KeyQueue& keys;
		QImage backBuffer;
	};
}
