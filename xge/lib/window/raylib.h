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
		float fit() const override;

	private:
		// <window><scale>: window pixels to one of the game's.
		float scale{ 1.0f };

		// Each object's picture as a texture, by the object's name.
		struct CachedVisual
		{
			Texture2D texture{};
			bool loaded{ false };
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at

			CachedVisual() = default;
			~CachedVisual() { if (loaded) { UnloadTexture(texture); } }
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

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
