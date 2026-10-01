// window_opengl.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"

#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct GLFWwindow;
struct _TTF_Font;

namespace xge
{
	// The OpenGL Window backend: GLFW makes the window and the OpenGL context
	// and reads the keyboard, and everything is drawn with OpenGL itself, as
	// textured quads. Nothing outside this file, window_opengl.cpp, and
	// WindowFactory::create (the one place that constructs one) ever names a
	// GLFW or OpenGL type - see window.h.
	//
	// The drawing is plain OpenGL 1.1 (a projection of the window in pixels,
	// one textured rectangle for each object), which every driver has and the
	// system's OpenGL library exports as it is, so there is no OpenGL loader
	// library to depend on. Only the framebuffer calls used for the back
	// buffer route (below) are looked up at runtime, through GLFW.
	//
	// The same bake-once-then-rebuild-on-visualDirty architecture as the other
	// backends. Each object's picture is made on the CPU - circles and
	// rectangles are filled in, a sprite of lines is already a Bitmap - and
	// uploaded as a texture. Text and images are decoded by SDL2_ttf and
	// SDL2_image, which the library already links for the SDL2 backend; with
	// no font file, text falls back to the engine's built-in font.
	//
	// With a WindowTarget of Kind::BackBuffer the window is hidden and the
	// frame is drawn to a framebuffer object, read back after display() and
	// handed over by backBuffer().
	class OpenGLWindow : public Window
	{
	public:
		explicit OpenGLWindow(const WindowDesc& windowDesc, const WindowTarget& target = {});
		~OpenGLWindow() override;

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;
		const Bitmap* backBuffer() const override;
		void activate() override;

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
		bool offscreen{ false };
		bool ttfInitialized{ false };
		bool imgInitialized{ false };
		bool fontMissing{ false };
		int width{ 0 };
		int height{ 0 };

		// Keys the GLFW callback has seen since pollEvents() last looked.
		std::vector<std::pair<KeyCode, bool>> pendingKeys;

		// The time one frame takes, and when the last one ended: a window of
		// its own waits out what is left of the frame time (as SFML's frame
		// limit does), because the display's refresh may be faster than the game.
		std::chrono::steady_clock::duration framePeriod{};
		std::chrono::steady_clock::time_point lastFrame{};

		// The back buffer route: a framebuffer object, the texture it draws to,
		// and the last frame read back from it.
		unsigned int framebuffer{ 0 };
		unsigned int targetTexture{ 0 };
		Bitmap captured;

		std::unordered_map<int, _TTF_Font*> fontsBySize;
		std::unordered_map<std::string, CachedVisual> visuals;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static KeyCode glfwKeyToKeyCode(int key) noexcept;

		void setUpProjection() const;
		void releaseTarget();
		_TTF_Font* getFont(int pointSize);

		void upload(CachedVisual& visual, const Bitmap& bitmap);
		void buildShapeOnly(Object& object, CachedVisual& visual);
		void buildCircle(Object& object, CachedVisual& visual);
		void buildRectangle(Object& object, CachedVisual& visual);
		void buildText(Object& object, CachedVisual& visual);
		void buildImage(Object& object, CachedVisual& visual);
		void finalizeVisual(Object& object, CachedVisual& visual);
	};
}
