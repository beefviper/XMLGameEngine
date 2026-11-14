// null.h
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#pragma once

#include "window/window.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A window that is never opened (-w none), as NullAudio is sound that is
	// never heard: the game runs as it would in a real one, the window's size
	// the game's own, but nothing is drawn and no key is ever pressed. Every
	// size is the game's own already (Object::pictureSize), so it only marks
	// each picture made.
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
		void prepare(Object& object) override { object.visualDirty = false; }
		void draw(Object& object) override { prepare(object); }
		void display() override {}

	private:
		bool open{ true };
	};
}
