// direct3d12.h
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
	// Windows only: a Win32 window (window/win32.h), each object one textured
	// rectangle drawn by Direct3D 12. Direct3D's types stay behind `Parts`.
	class Direct3D12Window : public Window
	{
	public:
		// Throws std::runtime_error if the window or Direct3D 12 cannot start.
		explicit Direct3D12Window(const WindowDesc& windowDesc);
		~Direct3D12Window() override;

		Direct3D12Window(const Direct3D12Window&) = delete;
		Direct3D12Window& operator=(const Direct3D12Window&) = delete;

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
		struct Parts;
		std::unique_ptr<Parts> parts;
	};
}
