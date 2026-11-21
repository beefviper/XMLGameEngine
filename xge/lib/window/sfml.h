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
		// What an object shows: a sprite over the texture of its picture,
		// which objects showing the same picture share.
		struct CachedVisual
		{
			std::unique_ptr<sf::Sprite> sprite;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		sf::RenderWindow window;
		sf::View view;  // the game's own pixels, placed in the window by fitView

		void fitView();
		float fitNow{ 1.0f };  // window pixels to a game pixel, as fitView left it
		Textures<sf::Texture> textures{ *this };
		std::unordered_map<std::string, CachedVisual> visuals;  // by object name

		static KeyCode sfmlKeyToKeyCode(sf::Keyboard::Key key) noexcept;

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
