// window.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "picture/bitmap.h"
#include "engine/keys.h"
#include "game/object.h"

#include <algorithm>
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
	//
	// The contract, in full in docs/reference/engine.md "What a window must
	// do". A window must:
	//   1. open at the game's size times its scale, or throw runtime_error;
	//   2. keep the game's proportions when resized (fit, centre, bars);
	//   3. make each object's picture in init(), and again only when
	//      visualDirty is set (prepare()), setting size and clearing
	//      visualDirty;
	//   4. make a picture by its kind: a Line is object.bitmap as it is, a
	//      text is drawText()'s bitmap (picture/font.h), an image file is
	//      loadPicture()'s (picture/image.h), a circle or a rectangle is
	//      filled;
	//   5. draw each object at its position, its top left, between clear()
	//      and display();
	//   6. report keys that changed (pollEvents()) and close when asked;
	//   7. report the mouse, in the game's pixels, and the pads (controls()).
	// It may assume positions and sizes are the game's own pixels, a picture
	// changes only with visualDirty, a bitmap is straight (not premultiplied)
	// RGBA from the top left, and every call comes from one thread. It
	// changes nothing on an object but size and visualDirty.
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
		// from its picture. Object::position (including a group's
		// cells) is already final by this point - Game finalizes all of
		// that itself, with no Window needed (see load/expr.cpp and
		// measureShapeSize in game/command.cpp) - so this only ever populates
		// Object::size with each object's real rendered footprint (exact,
		// unlike measureShapeSize's picture-only approximation) and
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
		// Game::setVariable/resetObject/resetAll) gets its cached visual
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
	// The objects `which` says, in the order they are listed, except that
	// those with a drawOrder (cards, a hand) come after the rest, lowest
	// first.
	template <typename Which>
	void drawInOrder(Window& window, std::vector<Object>& objects, Which which)
	{
		static std::vector<Object*> later;
		later.clear();
		for (auto& object : objects)
		{
			if (!which(object)) { continue; }
			if (object.drawOrder != 0) { later.push_back(&object); }
			else { drawObject(window, object); }
		}
		if (later.empty()) { return; }
		std::stable_sort(later.begin(), later.end(), [](const Object* a, const Object* b) { return a->drawOrder < b->drawOrder; });
		for (Object* object : later) { drawObject(window, *object); }
	}

	template <typename InPlay>
	void drawFrame(Window& window, std::vector<Object>& objects, const std::string& background, InPlay inPlay)
	{
		window.clear(background);
		drawInOrder(window, objects, inPlay);
		window.display();
	}

	// The same, with what is drawn still first, underneath (a paused game
	// behind its pause): every object `still` says, then every object in play.
	template <typename Still, typename InPlay>
	void drawFrame(Window& window, std::vector<Object>& objects, const std::string& background, Still still, InPlay inPlay)
	{
		window.clear(background);
		drawInOrder(window, objects, still);
		drawInOrder(window, objects, inPlay);
		window.display();
	}

	enum class WindowBackend
	{
		SFML3,
		Raylib,
		SDL2,
		OpenGL,
		None   // a window never opened (window/null.h): always built, never the default
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

		// SFML 3 when it is built, otherwise the first one that is; None only
		// when none is.
		static WindowBackend defaultBackend();

		// The backend's plain lower case name, as -w takes it: "sfml3", "raylib",
		// "sdl2", "opengl", "none".
		static std::string name(WindowBackend backend);
	};
}
