// opengl.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window/window.h"

#include <SDL_ttf.h>

#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct GLFWwindow;

namespace xge
{
	// The OpenGL Window backend: GLFW makes the window and the OpenGL context
	// and reads the keyboard, and everything is drawn with OpenGL itself, as
	// textured quads. Nothing outside this file, window/opengl.cpp, and
	// WindowFactory::create (the one place that constructs one) ever names a
	// GLFW or OpenGL type - see window/window.h.
	//
	// The drawing is plain OpenGL 1.1 (a projection of the window in pixels,
	// one textured rectangle for each object), which every driver has and the
	// system's OpenGL library exports as it is, so there is no OpenGL loader
	// library to depend on.
	//
	// The same bake-once-then-rebuild-on-visualDirty architecture as the other
	// backends. Each object's picture is made on the CPU - circles and
	// rectangles are filled in, a sprite of lines is already a Bitmap - and
	// uploaded as a texture. Text and images are decoded by SDL2_ttf and
	// SDL2_image, which the library already links for the SDL2 backend; with
	// no font file, text falls back to the engine's built-in font.
	class OpenGLWindow : public Window
	{
	public:
		// Throws std::runtime_error if GLFW cannot make a window.
		explicit OpenGLWindow(const WindowDesc& windowDesc);
		~OpenGLWindow() override;

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

	private:
		// One object's picture, as an OpenGL texture.
		struct CachedVisual
		{
			unsigned int texture{ 0 };
			int width{ 0 };
			int height{ 0 };
			bool flipHorizontal{ false };
			bool flipVertical{ false };

			CachedVisual() = default;
			~CachedVisual();
			CachedVisual(const CachedVisual&) = delete;
			CachedVisual& operator=(const CachedVisual&) = delete;
			CachedVisual(CachedVisual&&) = delete;
			CachedVisual& operator=(CachedVisual&&) = delete;
		};

		GLFWwindow* window{ nullptr };
		bool ttfInitialized{ false };
		bool imgInitialized{ false };
		bool imgTried{ false };
		int width{ 0 };
		int height{ 0 };
		// <window><scale>: window pixels to one of the game's.
		int scale{ 1 };

		// Keys the GLFW callback has seen since pollEvents() last looked.
		std::vector<std::pair<KeyCode, bool>> pendingKeys;

		// The time one frame takes, and when the last one ended: a window of
		// its own waits out what is left of the frame time (as SFML's frame
		// limit does), because the display's refresh may be faster than the game.
		std::chrono::steady_clock::duration framePeriod{};
		std::chrono::steady_clock::time_point lastFrame{};

		std::unordered_map<std::string, TTF_Font*> fontsBySize;   // by file@size
		// The font files that could not be loaded, said once each.
		std::unordered_set<std::string> fontsMissing;
		std::unordered_map<std::string, CachedVisual> visuals;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static KeyCode glfwKeyToKeyCode(int key) noexcept;

		void setUpProjection() const;
		TTF_Font* getFont(const std::string& file, int pointSize);

		// Starts SDL2_image on first use; false if it cannot read PNG.
		bool ensureImages();

		void upload(CachedVisual& visual, const Bitmap& bitmap);
		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void finalizeVisual(Object& object, CachedVisual& visual);
	};
}
