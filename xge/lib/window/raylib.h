// raylib.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window/window.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	// The Raylib Window backend. Nothing outside this file, window/raylib.cpp,
	// and WindowFactory::create (the one place that constructs one) ever
	// names a raylib type - see window/window.h. Same bake-once-to-a-render-texture,
	// rebuild-on-visualDirty architecture as SFMLWindow (see window/sfml.h),
	// re-implemented against raylib's own render-texture/image API -
	// raylib bundles its own image loading, so unlike the SDL2 backend this
	// needs no extra dependency for image objects.
	class RaylibWindow : public Window
	{
	public:
		// Throws std::runtime_error if raylib cannot start.
		explicit RaylibWindow(const WindowDesc& windowDesc);
		~RaylibWindow() override;

		bool isOpen() const override;
		void close() override;
		std::pair<int, int> position() const override;
		void setPosition(int x, int y) override;
		void setTitle(const std::string& title) override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		Controls controls() override;
		void clear(const std::string& colorName) override;
		void prepare(Object& object) override;
		void draw(Object& object) override;
		void display() override;

	private:
		// <window><scale>: window pixels to one of the game's.
		float scale{ 1.0f };

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

		// How each key (by raylib's key number) was the last time
		// pollEvents() looked, so it can report only what changed.
		std::array<bool, 512> keyWasDown{};

		// Whether display() has to finish the frame itself - swap the buffers,
		// read the keyboard and window - because
		// this raylib was built with SUPPORT_CUSTOM_FRAME_CONTROL, which makes
		// EndDrawing() do neither. Worked out once, after the first frame.
		bool frameControlChecked{ false };
		bool manualFrameControl{ false };
		std::unordered_map<std::string, CachedVisual> visuals;

		static void reloadTexture(CachedVisual& visual, int width, int height);

		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void buildLines(Object& object, CachedVisual& visual);
		void buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap);
	};
}
