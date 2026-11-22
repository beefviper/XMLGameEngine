// window.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "picture/bitmap.h"
#include "engine/keys.h"
#include "engine/resources.h"
#include "game/object.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	// A window, in the engine's terms only. The contract, in full in
	// docs/reference/engine.md "What a window must do". A window must:
	//   1. open at the game's size times its scale, or throw runtime_error;
	//   2. keep the game's proportions when resized (fit, centre, bars);
	//   3. ask each object's picture (pictureOf) in init(), and again when
	//      visualDirty is set (prepare()), clearing visualDirty; sizes are
	//      the game's own (Object::pictureSize), never the window's;
	//   4. keep one texture a picture, by its id (Textures), however many
	//      objects show it, made again only when that picture is drawn anew
	//      (PictureId::version); ask again when pictureScale() has changed
	//      for an object that is drawnAtScale();
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
		// Object::Drawn::visualDirty. Positions and sizes are already final: the
		// game works them out when it loads.
		virtual void init(std::vector<Object>& objects) = 0;

		// Every key that changed since the last call. A close request closes
		// the window; callers check isOpen().
		virtual std::vector<std::pair<KeyCode, bool>> pollEvents() = 0;

		// What the mouse and the gamepads are doing now, asked for once a
		// frame after pollEvents (Engine::handleControls makes presses of
		// what changed). A window with neither says nothing is there.
		virtual Controls controls() { return {}; }

		// colorName matches a sprite's color argument (e.g. "black") -
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

		// The scale the pictures are drawn at (Window::pictureOf). The
		// engine sets it to fit() once a resize has settled (SettledFit), so
		// text and shapes are drawn again, sharp, at the window's size.
		float pictureScale() const noexcept { return drawnScale; }
		void setPictureScale(float scale) noexcept { drawnScale = scale > 0.0f ? scale : 1.0f; }

		// The game's pictures, which the window shows and draws its texts
		// and shapes into (the Engine gives them); until then its own.
		void usePictures(Resources& resources) noexcept { gamePictures = &resources; }

		// The picture to show for the object now: its own bitmap or image,
		// or its circle, rectangle or text drawn at pictureScale(). A circle
		// or rectangle is shared by every object of that size and colour; a
		// text is the object's own picture, drawn again when its words or
		// the scale change.
		PictureId pictureOf(const Object& object);

		// How many textures the window has made since it opened: one a
		// picture, and again for a picture drawn anew.
		std::size_t texturesMade() const noexcept { return madeTextures; }

	protected:
		// One more texture made, for a window that counts without Textures.
		void madeTexture() noexcept { ++madeTextures; }

		// The textures a window keeps, one a picture by its id. `make(bitmap,
		// texture)` makes `texture` (as it was: none yet, or the picture's
		// older pixels) from the picture's pixels, the first time and when it
		// has been drawn anew.
		template <typename Texture>
		class Textures
		{
		public:
			explicit Textures(Window& window) noexcept : owner(window) {}

			template <typename Make>
			Texture& of(PictureId picture, Make&& make)
			{
				Held& held = kept[picture.index()];
				if (!held.made || held.version != picture.version())
				{
					static const Bitmap none;
					make(picture.get() ? *picture : none, held.texture);
					held.made = true;
					held.version = picture.version();
					++owner.madeTextures;
				}
				return held.texture;
			}

			// Each texture, to let go of with the library's own call.
			template <typename Each>
			void forEach(Each&& each)
			{
				for (auto& [index, held] : kept) { (void)index; each(held.texture); }
			}
			void clear() { kept.clear(); }

		private:
			struct Held
			{
				Texture texture{};
				std::uint32_t version{};
				bool made{ false };
			};
			Window& owner;
			std::unordered_map<std::uint32_t, Held> kept;
		};

	private:
		PictureId pictureNow(const Object& object);

		float drawnScale{ 1.0f };
		Resources ownPictures;
		Resources* gamePictures{ &ownPictures };
		std::size_t madeTextures{};
		// Each text's own picture, by its object, with what it was drawn
		// from.
		struct DrawnText
		{
			PictureId picture;
			std::string drawnFrom;
		};
		std::unordered_map<std::uint32_t, DrawnText> texts;
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
		if (object.dark.hidden || object.blinkedOut()) { return; }
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

	// One picture of a game: the background, then the objects `which` says
	// in the order listed, those with a drawOrder (cards, a hand) last,
	// lowest first.
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
		Vulkan,
		Direct3D9,    // Windows only
		Direct3D10,   // Windows only
		Direct3D11,   // Windows only
		Direct3D12,   // Windows only
		None   // a window never opened (window/null.h): always built, never the default
	};

	// The one place that knows every backend. A program is built with some
	// (options.cmake: SFML 3 alone by default, all for the tests);
	// WindowBackend names them all, so a name in a settings file still means
	// something, and available() says which can be made.
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
		// "sdl2", "opengl", "vulkan", "direct3d9", "direct3d10", "direct3d11", "direct3d12", "none".
		static std::string name(WindowBackend backend);
	};
}
