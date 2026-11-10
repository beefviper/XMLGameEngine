// null.h
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#pragma once

#include "window/window.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A window that is never opened (-w none), as NullAudio is sound that is
	// never heard: the game runs as it would in a real one, the window's size
	// the game's own, but nothing is drawn and no key is ever pressed. It still
	// measures every object as a window would, so sizes and the positions made
	// from them come out right: shapes by their picture, text in the built-in
	// 8 by 8 font (picture/font.h; it cannot know a font file's letters) and
	// an image file by its pixels.
	class NullWindow : public Window
	{
	public:
		NullWindow() = default;
		explicit NullWindow(const WindowDesc& windowDesc);

		bool isOpen() const override { return open; }
		void close() override { open = false; }
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string& colorName) override { (void)colorName; }
		void prepare(Object& object) override;
		void draw(Object& object) override { prepare(object); }
		void display() override {}

		// The size a window would give the object's picture.
		Vector2f measure(const Object& object);

	private:
		bool open{ true };
		// Each image file's size, read once.
		std::map<std::string, Vector2f> imageSizes;
	};
}
