// sfml.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window/window.h"

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <memory>
#include <unordered_map>

namespace xge
{
	// The SFML 3 Window backend. Nothing outside this file, window/sfml.cpp,
	// and WindowFactory::create (the one place that constructs one) ever
	// names an SFML type - see window/window.h.
	//
	// Absorbs what used to be the standalone game_sfml class: building each
	// object's visual from its picture (circle/rectangle/text/image) is
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
		Controls controls() override;
		void clear(const std::string& colorName) override;
		void prepare(Object& object) override;
		void draw(Object& object) override;
		void display() override;
		float fit() const override;
		Bitmap capture() override;

	private:
		// Each object's picture as a texture, by the object's name.
		struct CachedVisual
		{
			sf::Texture texture;
			std::unique_ptr<sf::Sprite> sprite;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		sf::RenderWindow window;
		sf::View view;  // the game's own pixels, placed in the window by fitView

		void fitView();
		float fitNow{ 1.0f };  // window pixels to a game pixel, as fitView left it
		std::unordered_map<std::string, CachedVisual> visuals;

		static KeyCode sfmlKeyToKeyCode(sf::Keyboard::Key key) noexcept;

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
