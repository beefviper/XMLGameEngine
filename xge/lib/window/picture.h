// picture.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "window/window.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A window that is a picture: the engine draws the frame into one image,
	// from the same pictures every window shows (Object::drawnPicture), with
	// no library and no screen. xgecli --picture writes it as a PNG, so a game
	// can be looked at with no window at all.
	//
	// It is the game's size times its <scale>, as a window would open. Like
	// NullWindow it is never closed by anyone and has no keys.
	class PictureWindow : public Window
	{
	public:
		explicit PictureWindow(const WindowDesc& windowDesc);

		bool isOpen() const override { return open; }
		void close() override { open = false; }
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string& colorName) override;
		void prepare(Object& object) override { object.visualDirty = false; }
		void draw(Object& object) override;
		void display() override {}
		float fit() const override { return scale; }

		// The last frame drawn, whole: it stays until the next clear().
		Bitmap capture() override { return frame; }

	private:
		bool open{ true };
		float scale{ 1.0f };
		Bitmap frame;
	};

	// Writes the picture as a PNG file; false if the file cannot be written.
	bool writePng(const Bitmap& picture, const std::string& file);
}
