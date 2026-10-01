// game_view.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "keycode.h"

#include <QImage>

#ifdef XGE_QT_OPENGL
#include <QOpenGLWidget>
#else
#include <QWidget>
#endif

#include <array>
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

	// The widget the game is shown in: the picture the QtWindow backend
	// (qt_window.h) draws a frame into, shown as large as fits without changing
	// its proportions, and the place the keyboard goes while it has focus. It owns no game logic. The engine draws into
	// frame(), calls present() when a frame is finished, and asks for the keys
	// pressed since the last frame with takeKeyEvents(), the same job pollEvents()
	// does for the other backends.
	class GameView : public GameViewBase
	{
	public:
		explicit GameView(QWidget* parent = nullptr);

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
		QImage backBuffer;
		std::vector<std::pair<KeyCode, bool>> pending;
		std::array<bool, static_cast<std::size_t>(KeyCode::Count)> down{};

		void queueKey(KeyCode key, bool pressed);
		void paintFrame(QPainter& painter);
	};
}
