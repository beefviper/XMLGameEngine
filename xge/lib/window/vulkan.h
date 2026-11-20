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
	// GLFW makes the window (window/glfw.h); each object is a textured
	// rectangle drawn by Vulkan. volk finds the driver's Vulkan as the window
	// opens, so no Vulkan SDK is needed. Vulkan's types stay behind `Parts`.
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
