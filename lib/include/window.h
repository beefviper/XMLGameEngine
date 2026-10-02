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

		// Where the window is on the screen, so that the next one can open in
		// the same place. A window that is not a window of its own keeps the
		// defaults.
		virtual std::pair<int, int> position() const { return { 0, 0 }; }
		virtual void setPosition(int x, int y) { (void)x; (void)y; }

		// The text on the window's title bar, which a front end can keep up
		// to date (the game's name, whether it is playing, how fast). A
		// window that is not a window of its own has no title bar and ignores
		// it.
		virtual void setTitle(const std::string& title) { (void)title; }

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
	};

	enum class WindowBackend
	{
		SFML3,
		Raylib,
		SDL2,
		OpenGL
	};

	// Builds the concrete Window for the given description and backend - the
	// one place that knows about all the backends. Adding another would only
	// need a branch added here and in WindowBackend; Engine and everything
	// else that calls WindowFactory::create stays untouched. Every backend
	// opens a window of its own.
	class WindowFactory
	{
	public:
		// Throws std::runtime_error if the library cannot start.
		static std::unique_ptr<Window> create(const WindowDesc& windowDesc, WindowBackend backend = WindowBackend::SFML3);
	};
}
