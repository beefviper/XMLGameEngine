// vulkan.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "window/window.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// The Vulkan Window backend: GLFW makes the window and reads the keys
	// (window/glfw.h, shared with OpenGL), and every object is drawn by Vulkan
	// as one textured rectangle. volk finds the graphics driver's own Vulkan
	// when the window opens, so no Vulkan SDK is needed to build or run it.
	// Nothing outside this file, window/vulkan.cpp and WindowFactory::create
	// names a GLFW or Vulkan type: they are all kept behind `Parts`.
	//
	// The window can be resized: the game is fitted to it with its
	// proportions, centred, and its pictures are drawn sharp at that size once
	// a resize settles, as in the SFML window.
	class VulkanWindow : public Window
	{
	public:
		// Throws std::runtime_error if there is no Vulkan, or the window cannot
		// be made.
		explicit VulkanWindow(const WindowDesc& windowDesc);
		~VulkanWindow() override;

		VulkanWindow(const VulkanWindow&) = delete;
		VulkanWindow& operator=(const VulkanWindow&) = delete;

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
