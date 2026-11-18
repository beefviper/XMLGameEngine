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
		Bitmap capture() override;

	private:
		// <window><scale>: window pixels to one of the game's.
		float scale{ 1.0f };

		// What an object shows: the texture of its picture, which objects
		// showing the same picture share (id 0 for an empty picture).
		struct CachedVisual
		{
			Texture2D texture{};
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
			bool built{ false };
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
		Textures<Texture2D> textures{ *this };
		std::unordered_map<std::string, CachedVisual> visuals;  // by object name

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
