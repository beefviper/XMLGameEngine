// window_sdl2.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <unordered_map>

namespace xge
{
	// The SDL2 Window backend (+ SDL2_ttf for text, SDL2_image for image
	// loading - plain SDL2 has neither). Nothing outside this file,
	// window_sdl2.cpp, and WindowFactory::create (the one place that
	// constructs one) ever names an SDL type - see window.h. Same
	// bake-once-then-rebuild-on-visualDirty architecture as SFMLWindow and
	// RaylibWindow, built on SDL_Texture render targets instead of
	// sf::RenderTexture/RenderTexture2D.
	class SDL2Window : public Window
	{
	public:
		explicit SDL2Window(const WindowDesc& windowDesc);
		~SDL2Window() override;

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;

	private:
		// Circle/rectangle are drawn straight into this texture (as an
		// SDL_TEXTUREACCESS_TARGET); text/image instead replace it outright
		// with a texture built from an SDL_ttf/SDL2_image surface - either
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
		bool isOpenFlag{ false };
		// Set when the font file could not be loaded: text is then drawn from
		// the engine's built-in font (see builtin_font.h).
		bool fontMissing{ false };
		bool ttfInitialized{ false };
		bool imgInitialized{ false };

		// TTF_Font is opened at a fixed point size (unlike SFML's sf::Font or
		// raylib's Font, either of which can be measured/drawn at any size
		// from one loaded font) - cached per size actually used, opened once
		// on first use rather than reopened on every buildText() call.
		std::unordered_map<int, TTF_Font*> fontsBySize;

		std::unordered_map<std::string, CachedVisual> visuals;

		static KeyCode sdlKeyToKeyCode(SDL_Keycode key) noexcept;
		TTF_Font* getFont(int pointSize);
		void reloadTargetTexture(CachedVisual& visual, int width, int height);

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
