// web.h
// XML Game Engine
// author: beefviper
// date: Nov 23, 2026

#pragma once

#include "window/window.h"

#include <string>
#include <unordered_map>

namespace xge
{
	// A canvas on a web page, for a game built with Emscripten (the web-asm
	// generator, games/generators/web-asm/). The page's JavaScript keeps the
	// canvas, a canvas of its own a picture, and the keys, mouse and pads it
	// heard; this only hands it pictures and asks what happened. The canvas
	// fills the browser's window, the game's proportions kept.
	class WebWindow : public Window
	{
	public:
		explicit WebWindow(const WindowDesc& windowDesc);
		~WebWindow() override;

		WebWindow(const WebWindow&) = delete;
		WebWindow& operator=(const WebWindow&) = delete;

		// A page is closed by closing its tab, which ends everything with it.
		bool isOpen() const override { return true; }
		void close() override {}
		void setTitle(const std::string& title) override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		Controls controls() override;
		void clear(const std::string& colorName) override;
		void prepare(Object& object) override;
		void draw(Object& object) override;
		void display() override {}
		float fit() const override;

	private:
		// What an object shows: the page's canvas of its picture (0 for an
		// empty picture), which objects showing the same picture share.
		struct CachedVisual
		{
			int canvas{ 0 };
			bool smooth{ false };
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
			bool built{ false };
		};

		Textures<int> textures{ *this };
		std::unordered_map<std::string, CachedVisual> visuals;  // by object name

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
