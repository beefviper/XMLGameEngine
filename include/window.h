// window.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "object.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// What Engine needs from a window each frame - lifecycle, drawing, and
	// keyboard input - expressed entirely in engine-level terms (WindowDesc,
	// xge::Object, key names as strings matching a State's
	// <input button="..."> attribute), so no particular windowing/graphics
	// library's types leak into Engine itself.
	//
	// SFML 3 is the only backend today (see window_sfml.h/.cpp and
	// WindowFactory below) - this interface is the same "swappable
	// windowing backend (SDL2, SFML3)" seam from the ground-up engine
	// rewrite that's been planned, brought into the current engine as its
	// own step rather than waiting on the whole rewrite.
	class Window
	{
	public:
		virtual ~Window() = default;

		virtual bool isOpen() const = 0;
		virtual void close() = 0;

		// Pumps the backend's event queue once. Returns every key that
		// changed state since the last call, as {keyName, pressed} pairs -
		// keyName is whatever a State's <input button="..."> would use
		// (e.g. "space", "escape", "a"). A close-request event closes the
		// window directly rather than being reported here, so callers only
		// need to recheck isOpen() afterward, not handle a "closed" event
		// themselves.
		virtual std::vector<std::pair<std::string, bool>> pollEvents() = 0;

		// colorName matches a sprite's color argument (e.g. "color.black") -
		// see WindowDesc::background and game_sfml's sprite color handling.
		virtual void clear(const std::string& colorName) = 0;
		virtual void draw(const Object& object) = 0;
		virtual void display() = 0;
	};

	// Builds the concrete Window for the given description. The one place
	// that knows which backend is in use today (SFML 3); a future backend
	// would only need a branch added here, not any change to Engine or
	// anything else that calls WindowFactory::create.
	class WindowFactory
	{
	public:
		static std::unique_ptr<Window> create(const WindowDesc& windowDesc);
	};
}
