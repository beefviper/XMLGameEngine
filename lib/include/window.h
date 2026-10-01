// window.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "bitmap.h"
#include "keycode.h"
#include "object.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// What Engine needs from a window each frame - lifecycle, drawing, and
	// keyboard input - expressed entirely in engine-level terms (WindowDesc,
	// xge::Object, xge::KeyCode), so no particular windowing/graphics
	// library's types leak into Engine or Game themselves. Four backends
	// implement this today - SFML3, Raylib, SDL2 and OpenGL (see
	// window_sfml.h, window_raylib.h, window_sdl2.h, window_opengl.h, and
	// WindowFactory below).
	class Window
	{
	public:
		virtual ~Window() = default;

		virtual bool isOpen() const = 0;
		virtual void close() = 0;

		// Called once, right after the window is created (see Engine's
		// constructor): builds and measures every object's initial visual
		// from its spriteParams. Object::position (including any <grid>
		// spacing) is already final by this point - Game finalizes all of
		// that itself, with no Window needed (see game_expr.cpp and
		// measureShapeSize in command.cpp) - so this only ever populates
		// Object::size with each object's real rendered footprint (exact,
		// unlike measureShapeSize's spriteParams-only approximation) and
		// clears Object::visualDirty, for every object it touches.
		virtual void init(std::vector<Object>& objects) = 0;

		// Pumps the backend's event queue once. Returns every key that
		// changed state since the last call, as {key, pressed} pairs. A
		// close-request event closes the window directly rather than being
		// reported here, so callers only need to recheck isOpen() afterward,
		// not handle a "closed" event themselves.
		virtual std::vector<std::pair<KeyCode, bool>> pollEvents() = 0;

		// colorName matches a sprite's color argument (e.g. "color.black") -
		// see WindowDesc::background and xge::colorFromName (color.h).
		virtual void clear(const std::string& colorName) = 0;

		// Draws one object. Non-const: an object whose visualDirty flag is
		// set (e.g. a bound text display's number just changed - see
		// Game::incrementText/resetObject/resetAll) gets its cached visual
		// rebuilt here, on demand, and its size/visualDirty brought back up
		// to date - the same one-time-build-then-cache pattern as init()
		// above, just for a single object instead of all of them at once.
		virtual void draw(Object& object) = 0;

		virtual void display() = 0;

		// The frame display() just finished, as pixels, when this window was
		// made to draw to a back buffer (WindowTarget::Kind::BackBuffer)
		// instead of to the screen; null for a window that draws to the screen.
		// The pointer stays good until the next display(). This is how a front
		// end that owns the screen itself (the Qt application) shows a library
		// that cannot draw into a window of its own.
		virtual const Bitmap* backBuffer() const { return nullptr; }

		// Makes this window's graphics context the current one on the calling
		// thread. Meant for a front end that shares the thread with another
		// user of OpenGL (Qt's widgets), which may have made its own context
		// current since this window last drew: it calls this before running a
		// frame. A window with no context of its own does nothing.
		virtual void activate() {}
	};

	enum class WindowBackend
	{
		SFML3,
		Raylib,
		SDL2,
		OpenGL
	};

	// Where a Window draws. By default it opens a window of its own, which is
	// what a program that runs the game by itself wants. A front end that has
	// a window already asks for one of the other two.
	struct WindowTarget
	{
		enum class Kind
		{
			// Opens its own window (the default).
			OwnWindow,

			// Draws into a window the front end made (nativeHandle). The front
			// end owns it and keeps it alive for as long as the Window lives;
			// the Window never closes or destroys it.
			NativeWindow,

			// Opens no visible window and draws to a buffer of the game's size,
			// handed over after every frame by Window::backBuffer().
			BackBuffer
		};

		Kind kind{ Kind::OwnWindow };

		// The platform's window handle: an HWND on Windows, an X11 window id
		// on Linux. Only for Kind::NativeWindow.
		void* nativeHandle{ nullptr };
	};

	// How a front end can show each backend inside a window of its own: the
	// libraries that can draw into a window someone else made (SFML3, SDL2)
	// are given one directly; the ones that cannot (raylib and GLFW always
	// make their own window) draw to a back buffer that is shown instead.
	enum class Embedding
	{
		NativeWindow,
		BackBuffer
	};

	// Builds the concrete Window for the given description and backend - the
	// one place that knows about all the backends. Adding another would only
	// need a branch added here, in embedding() and in WindowBackend; Engine
	// and everything else that calls WindowFactory::create stays untouched.
	class WindowFactory
	{
	public:
		// Throws std::invalid_argument if the backend cannot draw to the kind
		// of target asked for (see embedding()), and std::runtime_error if the
		// library cannot start.
		static std::unique_ptr<Window> create(const WindowDesc& windowDesc, WindowBackend backend = WindowBackend::SFML3,
			const WindowTarget& target = {});

		// The only kind of embedded target the backend supports (it always
		// supports OwnWindow).
		static Embedding embedding(WindowBackend backend) noexcept;
	};
}
