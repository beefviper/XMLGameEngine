// glfw.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "engine/keys.h"

namespace xge
{
	// What the GLFW windows (window/opengl.cpp, window/vulkan.cpp) share: the
	// engine's name for a GLFW key, and the pads as GLFW reads them.
	KeyCode keyFromGlfw(int key) noexcept;

	// Each pad GLFW knows as a gamepad, in its order, into controls.pads.
	void readGlfwPads(Controls& controls);
}
