// direct3d11.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "window/window.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	// The Direct3D 11 Window backend, on Windows only: a window of Win32's own,
	// its keyboard and mouse read from Win32 and its pads from XInput, and every
	// object drawn by Direct3D 11 as one textured rectangle. Nothing outside
	// this file, window/direct3d11.cpp and WindowFactory::create names a Win32
	// or Direct3D type: they are all kept behind `Parts` (window/window.h).
	//
	// The window can be resized: the game is fitted to it with its
	// proportions, centred, and its pictures are drawn sharp at that size once
	// a resize settles, as in the SFML window.
	class Direct3D11Window : public Window
	{
	public:
		// Throws std::runtime_error if the window or Direct3D 11 cannot start.
		explicit Direct3D11Window(const WindowDesc& windowDesc);
		~Direct3D11Window() override;

		Direct3D11Window(const Direct3D11Window&) = delete;
		Direct3D11Window& operator=(const Direct3D11Window&) = delete;

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

		// Win32 hands the window's messages here (window/direct3d11.cpp).
		struct Parts;

	private:
		std::unique_ptr<Parts> parts;
	};
}
