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
#include <optional>
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
	//   3. make each object's picture in init(), and again when
	//      visualDirty is set (prepare()), clearing visualDirty; sizes are
	//      the game's own (Object::pictureSize), never the window's;
	//   4. make a picture from Object::drawnPicture(pictureScale()), the
	//      engine's own bitmap of it, and make it again when pictureScale()
	//      has changed for an object that is drawnAtScale();
	//   5. draw each object's picture over pictureSize() at its position,
	//      its top left, between clear() and display(), smoothed if it is
	//      drawnAtScale() and with hard edges if not;
	//   6. report keys that changed (pollEvents()) and close when asked;
	//   7. report the mouse, in the game's pixels, and the pads (controls());
	//   8. say how many of its pixels go to one of the game's now (fit()),
	//      and start pictureScale() at that when it opens.
	// It may assume positions and sizes are the game's own pixels, a picture
	// changes only with visualDirty or pictureScale(), a bitmap is straight
	// (not premultiplied) RGBA from the top left, and every call comes from
	// one thread. It changes nothing on an object but visualDirty.
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
		// constructor): makes every object's picture (rule 3 above) and clears
		// Object::visualDirty. Positions and sizes are already final: the
		// game works them out when it loads.
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

		// Makes an object's picture again if its visualDirty flag is set (a
		// text's number changed, a new look) and clears the flag. draw()
		// calls it first.
		virtual void prepare(Object& object) { (void)object; }

		virtual void draw(Object& object) = 0;

		virtual void display() = 0;

		// What has been drawn since clear(), asked for before display(): the
		// window's own pixels, straight RGBA from the top left (alpha not
		// meaningful). Empty from a window that cannot read its pixels back.
		// For tests that look at what a library really shows.
		virtual Bitmap capture() { return {}; }

		// How many of the window's pixels show one of the game's now: the
		// game's <scale>, or what fits after a resize. 1 for a window that
		// is not one of its own.
		virtual float fit() const { return 1.0f; }

		// The scale the pictures are drawn at (Object::drawnPicture). The
		// engine sets it to fit() once a resize has settled (SettledFit), so
		// text and shapes are drawn again, sharp, at the window's size.
		float pictureScale() const noexcept { return drawnScale; }
		void setPictureScale(float scale) noexcept { drawnScale = scale > 0.0f ? scale : 1.0f; }

	private:
		float drawnScale{ 1.0f };
	};

	// When a window's pictures are drawn again at its size: once its fit()
	// has held still a quarter of a second, so a window being dragged
	// stretches what it has and redraws once, when it is let go.
	class SettledFit
	{
	public:
		static constexpr double kSettle = 0.25;   // seconds

		explicit SettledFit(float start = 1.0f) noexcept : drawn(start), seen(start) {}

		// The fit now, at `seconds` on any steady clock. Gives the scale to
		// draw at, once, when it has settled at a new one; nothing otherwise.
		std::optional<float> update(float fit, double seconds) noexcept
		{
			if (fit != seen)
			{
				seen = fit;
				since = seconds;
				return std::nullopt;
			}
			if (seen == drawn || seconds - since < kSettle) { return std::nullopt; }
			drawn = seen;
			return drawn;
		}

	private:
		float drawn;
		float seen;
		double since{ 0.0 };
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
		Direct3D9,    // Windows only
		Direct3D11,   // Windows only
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
		// "sdl2", "opengl", "direct3d9", "direct3d11", "none".
		static std::string name(WindowBackend backend);
	};
}
