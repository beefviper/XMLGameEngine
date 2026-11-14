// opengl.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "window/opengl.h"

#include "picture/color.h"
#include "window/glfw.h"

// GLFW's own header brings in OpenGL 1.1: what the system's OpenGL library
// exports as it is.
#include <algorithm>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace xge
{
	namespace
	{
		Bitmap filledBitmap(int width, int height)
		{
			Bitmap bitmap;
			bitmap.width = width > 0 ? width : 1;
			bitmap.height = height > 0 ? height : 1;
			bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4, 0);
			return bitmap;
		}
	}

	OpenGLWindow::CachedVisual::~CachedVisual()
	{
		if (texture)
		{
			glDeleteTextures(1, &texture);
		}
	}

	OpenGLWindow::OpenGLWindow(const WindowDesc& windowDesc) :
		width(static_cast<int>(windowDesc.width)),
		height(static_cast<int>(windowDesc.height)),
		scale(std::max(1, windowDesc.scale))
	{
		if (!glfwInit())
		{
			throw std::runtime_error("GLFW could not start");
		}

		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		GLFWmonitor* monitor = (windowDesc.fullscreen == "true") ? glfwGetPrimaryMonitor() : nullptr;
		window = glfwCreateWindow(width * scale, height * scale, windowDesc.name.c_str(), monitor, nullptr);

		if (!window)
		{
			glfwTerminate();
			throw std::runtime_error("GLFW could not make an OpenGL window");
		}

		glfwMakeContextCurrent(window);

		// The engine sets the game's speed (Engine::loop); the window only waits
		// for the screen's refresh, so a picture is never torn.
		glfwSwapInterval(1);

		setPictureScale(static_cast<float>(scale));
		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, &OpenGLWindow::keyCallback);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	OpenGLWindow::~OpenGLWindow()
	{
		if (!window)
		{
			return;
		}

		// Everything OpenGL made goes while its context is current.
		glfwMakeContextCurrent(window);
		visuals.clear();


		glfwDestroyWindow(window);
		window = nullptr;
		glfwTerminate();
	}

	bool OpenGLWindow::isOpen() const
	{
		return window && !glfwWindowShouldClose(window);
	}

	void OpenGLWindow::close()
	{
		// Not torn down here: the frame that is running still has to draw. The
		// destructor does that.
		if (window)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	std::pair<int, int> OpenGLWindow::position() const
	{
		int x = 0;
		int y = 0;
		glfwGetWindowPos(window, &x, &y);
		return { x, y };
	}

	void OpenGLWindow::setPosition(int x, int y)
	{
		glfwSetWindowPos(window, x, y);
	}

	void OpenGLWindow::setTitle(const std::string& title)
	{
		glfwSetWindowTitle(window, title.c_str());
	}

	void OpenGLWindow::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
		}
	}

	Controls OpenGLWindow::controls()
	{
		Controls controls;
		if (!window || glfwGetWindowAttrib(window, GLFW_FOCUSED) == 0)
		{
			return controls;
		}

		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(window, &x, &y);
		int shownWidth = 0;
		int shownHeight = 0;
		glfwGetWindowSize(window, &shownWidth, &shownHeight);
		controls.pointerInside = x >= 0.0 && y >= 0.0 && x < shownWidth && y < shownHeight;
		// The game is drawn across the whole window (setUpProjection).
		controls.pointerX = shownWidth > 0 ? static_cast<float>(x * width / shownWidth) : 0.0f;
		controls.pointerY = shownHeight > 0 ? static_cast<float>(y * height / shownHeight) : 0.0f;
		controls.mouse = { glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS,
			glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS };

		readGlfwPads(controls);
		return controls;
	}

	std::vector<std::pair<KeyCode, bool>> OpenGLWindow::pollEvents()
	{
		glfwPollEvents();

		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(pendingKeys);
		return events;
	}

	void OpenGLWindow::setUpProjection() const
	{
		glViewport(0, 0, width * scale, height * scale);

		// One unit is one of the game's pixels (scale window pixels across), with 0, 0 at the top left like everywhere
		// else in the engine.
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	void OpenGLWindow::clear(const std::string& colorName)
	{
		setUpProjection();

		const Color c = colorFromName(colorName);
		glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	void OpenGLWindow::prepare(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.texture || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}
	}

	void OpenGLWindow::draw(Object& object)
	{
		prepare(object);
		CachedVisual& visual = visuals[object.name];

		if (!visual.texture)
		{
			return;
		}

		// Whole game pixels, as the SDL2 backend draws them: a texture
		// sampled halfway between two pixels is smeared. Stretched over the
		// object's size in the game's pixels.
		const Vector2f size = object.pictureSize();
		const float x0 = std::floor(object.position.x);
		const float y0 = std::floor(object.position.y);
		const float x1 = x0 + size.x;
		const float y1 = y0 + size.y;

		glBindTexture(GL_TEXTURE_2D, visual.texture);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y0);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y0);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y1);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y1);
		glEnd();
	}

	float OpenGLWindow::fit() const
	{
		return static_cast<float>(scale);
	}

	void OpenGLWindow::display()
	{
		glfwSwapBuffers(window);
	}

	void OpenGLWindow::keyCallback(GLFWwindow* window, int key, int, int action, int)
	{
		// A held key repeats; the engine only wants the first press.
		if (action == GLFW_REPEAT)
		{
			return;
		}

		auto* self = static_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
		const KeyCode code = keyFromGlfw(key);

		if (self && code != KeyCode::Unknown)
		{
			self->pendingKeys.emplace_back(code, action == GLFW_PRESS);
		}
	}

	Bitmap OpenGLWindow::capture()
	{
		Bitmap bitmap;
		glfwGetFramebufferSize(window, &bitmap.width, &bitmap.height);
		if (bitmap.width <= 0 || bitmap.height <= 0) { return {}; }
		const std::size_t row = static_cast<std::size_t>(bitmap.width) * 4;
		std::vector<std::uint8_t> upsideDown(row * static_cast<std::size_t>(bitmap.height));
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, bitmap.width, bitmap.height, GL_RGBA, GL_UNSIGNED_BYTE, upsideDown.data());
		// OpenGL reads from the bottom row up.
		bitmap.rgba.resize(upsideDown.size());
		for (int y = 0; y < bitmap.height; ++y)
		{
			std::copy_n(upsideDown.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(bitmap.height - 1 - y)), row,
				bitmap.rgba.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(y)));
		}
		return bitmap;
	}

	// The engine's picture of the object, at pictureScale().
	void OpenGLWindow::build(Object& object, CachedVisual& visual)
	{
		object.visualDirty = false;
		visual.drawnAt = pictureScale();

		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		const Bitmap blank = picture->rgba.empty() ? filledBitmap(1, 1) : Bitmap{};
		const Bitmap& source = picture->rgba.empty() ? blank : *picture;

		if (!visual.texture)
		{
			glGenTextures(1, &visual.texture);
		}

		const GLint filter = object.drawnAtScale() ? GL_LINEAR : GL_NEAREST;
		glBindTexture(GL_TEXTURE_2D, visual.texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, source.width, source.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, source.rgba.data());
	}
}
