// window_sfml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window.h"

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <memory>
#include <unordered_map>

namespace xge
{
	// The SFML 3 Window backend. Nothing outside this file, window_sfml.cpp,
	// and WindowFactory::create (the one place that constructs one) ever
	// names an SFML type - see window.h.
	//
	// Absorbs what used to be the standalone game_sfml class: building each
	// object's visual from its spriteParams (circle/rectangle/text/image) is
	// backend work, not Game's, so it lives here now as SFMLWindow's own
	// per-object cache instead of being baked into Object itself.
	class SFMLWindow : public Window
	{
	public:
		// Throws std::runtime_error if the window cannot be made.
		explicit SFMLWindow(const WindowDesc& windowDesc);

		bool isOpen() const override;
		void close() override;
		std::pair<int, int> position() const override;
		void setPosition(int x, int y) override;
		void setTitle(const std::string& title) override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;

	private:
		// What Object used to own directly (renderTexture + sprite) - now
		// kept here instead, one per object name, so Object itself never
		// has to know an sf::Sprite exists.
		struct CachedVisual
		{
			sf::RenderTexture renderTexture;
			std::unique_ptr<sf::Sprite> sprite;
		};

		sf::RenderWindow window;
		sf::Font font;
		bool fontLoaded{ false };
		// Set when the font file could not be loaded: text is then drawn from
		// the engine's built-in font (see builtin_font.h).
		bool fontMissing{ false };
		std::unordered_map<std::string, CachedVisual> visuals;

		static KeyCode sfmlKeyToKeyCode(sf::Keyboard::Key key) noexcept;
		const sf::Font& getFont();

		// Resizes/draws visual.renderTexture from object.spriteParams alone -
		// no position/grid math, no sprite (re)creation. Shared by init()
		// (which still needs to do its own grid math afterward, using the
		// size this just measured) and draw()'s on-demand rebuild path
		// (which never repeats grid math - see the comment in draw()).
		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void buildLines(Object& object, CachedVisual& visual);
		void buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap);

		// Finishes a visual after buildShapeOnly() (and, in init()'s case,
		// after grid position math): measures object.size, finalizes the
		// render texture, (re)builds the sprite at object.position, and
		// clears object.visualDirty.
		void finalizeVisual(Object& object, CachedVisual& visual);
	};
}
