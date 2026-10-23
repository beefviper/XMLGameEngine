// window.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "picture/bitmap.h"
#include "engine/keys.h"
#include "game/object.h"

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
	// window/sfml.h, window/raylib.h, window/sdl2.h, window/opengl.h, and
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
		// from its spriteParams. Object::position (including a group's
		// cells) is already final by this point - Game finalizes all of
		// that itself, with no Window needed (see load/expr.cpp and
		// measureShapeSize in game/command.cpp) - so this only ever populates
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

		// What the mouse and the gamepads are doing now, asked for once a
		// frame after pollEvents (Engine::handleControls makes presses of
		// what changed). A window with neither says nothing is there.
		virtual Controls controls() { return {}; }

		// colorName matches a sprite's color argument (e.g. "color.black") -
		// see WindowDesc::background and xge::colorFromName (picture/color.h).
		virtual void clear(const std::string& colorName) = 0;

		// Draws one object. Non-const: an object whose visualDirty flag is
		// set (e.g. a bound text display's number just changed - see
		// Game::incrementText/resetObject/resetAll) gets its cached visual
		// rebuilt here, on demand, and its size/visualDirty brought back up
		// to date - the same one-time-build-then-cache pattern as init()
		// above, just for a single object instead of all of them at once.
		// Builds an object's visual now if it is out of date, so that its size
		// is true before anything is drawn: a text whose number just changed
		// can then be placed by the size it has, not the one it had.
		virtual void prepare(Object& object) { (void)object; }

		virtual void draw(Object& object) = 0;

		virtual void display() = 0;
	};

	// Draws one object, unless it is <dark> and unlit or blinked out, raised
	// off the ground by its <altitude> (its own position stays on the ground).
	inline void drawObject(Window& window, Object& object)
	{
		if (object.inDark || object.blinkedOut()) { return; }
		if (object.altitude == 0.0f)
		{
			window.draw(object);
			return;
		}
		const float ground = object.position.y;
		object.position.y = ground - object.altitude;
		window.draw(object);
		object.position.y = ground;
	}

	// One picture of a game: the background, then every object in play, in
	// the order they are listed. What the engine draws each frame, and a
	// generated game too.
	template <typename InPlay>
	void drawFrame(Window& window, std::vector<Object>& objects, const std::string& background, InPlay inPlay)
	{
		window.clear(background);

		for (auto& object : objects)
		{
			if (inPlay(object))
			{
				drawObject(window, object);
			}
		}

		window.display();
	}

	// The same, with what is drawn still first, underneath (a paused game
	// behind its pause): every object `still` says, then every object in play.
	template <typename Still, typename InPlay>
	void drawFrame(Window& window, std::vector<Object>& objects, const std::string& background, Still still, InPlay inPlay)
	{
		window.clear(background);

		for (auto& object : objects)
		{
			if (still(object))
			{
				drawObject(window, object);
			}
		}
		for (auto& object : objects)
		{
			if (inPlay(object))
			{
				drawObject(window, object);
			}
		}

		window.display();
	}

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
	//
	// A program is built with some of the backends, not necessarily all of
	// them (options.cmake: SFML 3 alone by default, every one when the tests
	// are built). WindowBackend names all four always, so that a name on the
	// command line or in a settings file still means something, and available()
	// says which can be made.
	class WindowFactory
	{
	public:
		// Throws std::runtime_error if the library cannot start, or was not built
		// into this program (the message says which are).
		static std::unique_ptr<Window> create(const WindowDesc& windowDesc, WindowBackend backend = defaultBackend());

		// Whether this program was built with the backend.
		static bool available(WindowBackend backend);

		// The backends this program was built with, in WindowBackend's order.
		static std::vector<WindowBackend> availableBackends();

		// SFML 3 when it is built, otherwise the first one that is.
		static WindowBackend defaultBackend();

		// The backend's plain lower case name, as -w takes it: "sfml3", "raylib",
		// "sdl2", "opengl".
		static std::string name(WindowBackend backend);
	};
}
