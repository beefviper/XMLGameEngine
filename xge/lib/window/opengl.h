// opengl.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window/window.h"


#include <string>
#include <unordered_map>
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
	// Like the other backends, each object's picture is the engine's
	// (Object::drawnPicture), uploaded as a texture and made again on
	// visualDirty.
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
		float fit() const override;
		Bitmap capture() override;

	private:
		// One object's picture, as an OpenGL texture.
		struct CachedVisual
		{
			unsigned int texture{ 0 };
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at

			CachedVisual() = default;
			~CachedVisual();
			CachedVisual(const CachedVisual&) = delete;
			CachedVisual& operator=(const CachedVisual&) = delete;
			CachedVisual(CachedVisual&&) = delete;
			CachedVisual& operator=(CachedVisual&&) = delete;
		};

		GLFWwindow* window{ nullptr };
		int width{ 0 };
		int height{ 0 };
		// <window><scale>: window pixels to one of the game's.
		int scale{ 1 };

		// Keys the GLFW callback has seen since pollEvents() last looked.
		std::vector<std::pair<KeyCode, bool>> pendingKeys;

		std::unordered_map<std::string, CachedVisual> visuals;

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		void setUpProjection() const;

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
