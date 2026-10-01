// window_raylib.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	// The Raylib Window backend. Nothing outside this file, window_raylib.cpp,
	// and WindowFactory::create (the one place that constructs one) ever
	// names a raylib type - see window.h. Same bake-once-to-a-render-texture,
	// rebuild-on-visualDirty architecture as SFMLWindow (see window_sfml.h),
	// re-implemented against raylib's own render-texture/font/image API -
	// raylib bundles its own image and TrueType font loading, so unlike the
	// SDL2 backend this needs no extra dependencies for text/image objects.
	class RaylibWindow : public Window
	{
	public:
		// With a WindowTarget of Kind::BackBuffer, the window is hidden and the
		// game is drawn to a texture instead, handed over by backBuffer() after
		// each frame (raylib always makes a window of its own, so it cannot draw
		// into one a front end made).
		explicit RaylibWindow(const WindowDesc& windowDesc, const WindowTarget& target = {});
		~RaylibWindow() override;

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
		// What SFMLWindow keeps a RenderTexture+Sprite pair for, raylib only
		// needs the RenderTexture2D itself for - draw() blits its .texture
		// straight at object.position, no separate sprite object needed.
		struct CachedVisual
		{
			RenderTexture2D renderTexture{};
			bool loaded{ false };

			CachedVisual() = default;
			~CachedVisual() { if (loaded) { UnloadRenderTexture(renderTexture); } }
			CachedVisual(const CachedVisual&) = delete;
			CachedVisual& operator=(const CachedVisual&) = delete;
			CachedVisual(CachedVisual&&) = delete;
			CachedVisual& operator=(CachedVisual&&) = delete;
		};

		bool isOpenFlag{ false };

		// Drawing to a texture instead of the screen (see the constructor).
		bool offscreen{ false };
		RenderTexture2D backTarget{};
		Bitmap captured;

		// raylib's GLFW window, as the front end's other OpenGL users may have
		// taken the context since (see Window::activate()). Null when it cannot
		// be reached.
		void* graphicsContext{ nullptr };

		void captureBackTarget();

		// How each key (by raylib's key number) was the last time
		// pollEvents() looked, so it can report only what changed.
		std::array<bool, 512> keyWasDown{};

		// Whether display() has to finish the frame itself - swap the buffers,
		// read the keyboard and window, and wait out the frame time - because
		// this raylib was built with SUPPORT_CUSTOM_FRAME_CONTROL, which makes
		// EndDrawing() do none of that. Worked out once, after the first frame.
		bool frameControlChecked{ false };
		bool manualFrameControl{ false };
		double frameTarget{ 0.0 };
		double frameStart{ 0.0 };
		Font font{};
		bool fontLoaded{ false };
		bool customFont{ false };
		// Set when the font file could not be loaded: text is then drawn from
		// the engine's built-in font (see builtin_font.h).
		bool fontMissing{ false };
		std::unordered_map<std::string, CachedVisual> visuals;

		const Font& getFont();
		static void reloadTexture(CachedVisual& visual, int width, int height);

		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void buildLines(Object& object, CachedVisual& visual);
		void buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap);

		void finalizeVisual(Object& object, CachedVisual& visual);
	};
}
