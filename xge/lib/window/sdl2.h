// sdl2.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window/window.h"

#include <SDL.h>

#include <array>
#include <unordered_map>

namespace xge
{
	// The SDL2 Window backend. Nothing outside this file,
	// window/sdl2.cpp, and WindowFactory::create (the one place that
	// constructs one) ever names an SDL type - see window/window.h. Same
	// bake-once-then-rebuild-on-visualDirty architecture as SFMLWindow and
	// RaylibWindow, built on SDL_Texture render targets instead of
	// sf::RenderTexture/RenderTexture2D.
	class SDL2Window : public Window
	{
	public:
		// Throws std::runtime_error if SDL2 cannot make a window.
		explicit SDL2Window(const WindowDesc& windowDesc);
		~SDL2Window() override;

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
		// Circle/rectangle are drawn straight into this texture (as an
		// SDL_TEXTUREACCESS_TARGET); a bitmap or an image instead replaces it
		// outright with a texture built from its pixels - either
		// way, draw() only ever needs the finished texture (+ how to flip it
		// for an image asked to mirror).
		struct CachedVisual
		{
			SDL_Texture* texture{ nullptr };
			SDL_RendererFlip flip{ SDL_FLIP_NONE };

			CachedVisual() = default;
			~CachedVisual() { if (texture) { SDL_DestroyTexture(texture); } }
			CachedVisual(const CachedVisual&) = delete;
			CachedVisual& operator=(const CachedVisual&) = delete;
			CachedVisual(CachedVisual&&) = delete;
			CachedVisual& operator=(CachedVisual&&) = delete;
		};

		SDL_Window* window{ nullptr };
		SDL_Renderer* renderer{ nullptr };
		// <window><scale>: window pixels to one of the game's.
		float scale{ 1.0f };

		// The pads opened so far (controls()), and whether SDL's game
		// controller part started.
		std::array<SDL_GameController*, padCount> pads{};
		bool padsInitialized{ false };
		bool isOpenFlag{ false };
		// The close button was pressed. isOpen() turns false at once, but
		// nothing is torn down until close() (the destructor), because the
		// frame that read the event is still going to clear and draw.
		bool closeRequested{ false };

		std::unordered_map<std::string, CachedVisual> visuals;

		static KeyCode sdlKeyToKeyCode(SDL_Keycode key) noexcept;

		void reloadTargetTexture(CachedVisual& visual, int width, int height);

		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void buildLines(Object& object, CachedVisual& visual);
		void buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap);
	};
}
