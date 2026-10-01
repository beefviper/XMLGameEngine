// game_view.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "bitmap.h"
#include "key_queue.h"

#include <QImage>

#ifdef XGE_QT_OPENGL
#include <QOpenGLWidget>
#else
#include <QWidget>
#endif

#include <utility>
#include <vector>

class QPainter;

namespace xge
{
#ifdef XGE_QT_OPENGL
	// A QOpenGLWidget swaps its picture onto the screen on the display's
	// vertical blank, which is what stops a moving picture tearing; a plain
	// QWidget is copied to the window whenever it is ready.
	using GameViewBase = QOpenGLWidget;
#else
	using GameViewBase = QWidget;
#endif

	// A widget that shows a picture the program drew itself: the picture the
	// QtWindow backend (qt_window.h) draws a frame into, or the pixels a window
	// library drew to a back buffer (showFrame), shown as large as fits without
	// changing its proportions. It owns no game logic. The engine draws into
	// frame() (or a library's frame is handed to showFrame()), calls present()
	// when a frame is finished, and the keys go to the GameStage's KeyQueue.
	class GameView : public GameViewBase
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

		// Takes a finished frame from a window library that drew it off screen
		// (opaque pixels, red, green, blue, alpha, row by row from the top) and
		// shows it.
		void showFrame(const Bitmap& bitmap);

		// The keys that changed since the last call, as {key, pressed}.
		std::vector<std::pair<KeyCode, bool>> takeKeyEvents();

	protected:
#ifdef XGE_QT_OPENGL
		void paintGL() override;
#else
		void paintEvent(QPaintEvent* event) override;
#endif
		void keyPressEvent(QKeyEvent* event) override;
		void keyReleaseEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void focusOutEvent(QFocusEvent* event) override;

		// Tab is a key a game may use, not a way to leave the view.
		bool focusNextPrevChild(bool next) override;

	private:
		KeyQueue& keys;
		QImage backBuffer;

		void paintFrame(QPainter& painter);
	};
}
