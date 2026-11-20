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
	// GLFW makes the window and reads the keys; each object is a textured
	// rectangle in plain OpenGL 1.1, which every system library exports, so
	// no OpenGL loader is needed.
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
		// What an object shows: the OpenGL texture of its picture, which
		// objects showing the same picture share.
		struct CachedVisual
		{
			unsigned int texture{ 0 };
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
		};

		GLFWwindow* window{ nullptr };
		int width{ 0 };
		int height{ 0 };
		// <window><scale>: window pixels to one of the game's.
		int scale{ 1 };

		// Keys the GLFW callback has seen since pollEvents() last looked.
		std::vector<std::pair<KeyCode, bool>> pendingKeys;

		Textures<unsigned int> textures{ *this };
		std::unordered_map<std::string, CachedVisual> visuals;  // by object name

		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

		void setUpProjection() const;

		// Makes the object's picture again, at pictureScale().
		void build(Object& object, CachedVisual& visual);
	};
}
