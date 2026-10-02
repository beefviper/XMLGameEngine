// embedded_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"

#include <memory>

class QOpenGLContext;
class QSurface;

namespace xge
{
	class GameStage;

	// Qt keeps its own record of which OpenGL context is current, and trusts
	// it: when it thinks its context is already current it skips making it
	// current again. A window library (raylib, GLFW, SFML) makes its own
	// context current behind Qt's back, so after every call into one, the
	// context Qt had is made current again. Otherwise Qt's next drawing goes
	// into the library's context (and the library's into Qt's): pictures from
	// one end up in the other, and with a strict driver, a crash. One of these
	// lives for the length of each call into a library; it does nothing when Qt
	// has no OpenGL context current, or Qt was built without OpenGL.
	class QtContextKeeper
	{
	public:
		QtContextKeeper();
		~QtContextKeeper();

		QtContextKeeper(const QtContextKeeper&) = delete;
		QtContextKeeper& operator=(const QtContextKeeper&) = delete;

	private:
		QOpenGLContext* context{ nullptr };
		QSurface* surface{ nullptr };
	};

	// A window library's Window made to work inside the Qt application: it does
	// what the library's Window does and changes only what is Qt's to decide.
	//
	//   keys     come from Qt (the GameStage's KeyQueue) rather than from the
	//            library, whose window never has the keyboard focus here
	//   frames   a window that drew to a back buffer has each finished frame
	//            handed to the GameStage's view to show
	//   context  the library's graphics context is made current before each
	//            call into it, and Qt's is made current again afterwards (see
	//            QtContextKeeper), because Qt's own OpenGL widgets share the
	//            thread; the same goes for freeing the library's window
	class EmbeddedWindow : public Window
	{
	public:
		EmbeddedWindow(std::unique_ptr<Window> inner, GameStage& stage);
		~EmbeddedWindow() override;

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;
		const Bitmap* backBuffer() const override;
		void activate() override;

	private:
		std::unique_ptr<Window> inner;
		GameStage& stage;

		// Between clear() and display() the library is drawing a frame, and its
		// context stays current: draw() is called once per object, and giving
		// the context back to Qt in between would only cost time. Qt does not
		// run in the middle of a frame.
		std::unique_ptr<QtContextKeeper> frameKeeper;
	};
}
