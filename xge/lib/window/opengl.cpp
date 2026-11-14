// opengl.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "window/opengl.h"

#include "picture/color.h"

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

		constexpr int buttons[padButtonCount] = {
			GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y,
			GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START,
			GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
		};
		std::size_t pad = 0;
		for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST && pad < padCount; ++joystick)
		{
			GLFWgamepadstate state;
			if (!glfwJoystickIsGamepad(joystick) || !glfwGetGamepadState(joystick, &state)) { continue; }
			Pad& now = controls.pads[pad++];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = state.buttons[buttons[button]] == GLFW_PRESS;
			}
			now.x = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			now.y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		}

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
		const KeyCode code = glfwKeyToKeyCode(key);

		if (self && code != KeyCode::Unknown)
		{
			self->pendingKeys.emplace_back(code, action == GLFW_PRESS);
		}
	}

	KeyCode OpenGLWindow::glfwKeyToKeyCode(int key) noexcept
	{
		if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (key - GLFW_KEY_A));
		}

		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (key - GLFW_KEY_0));
		}

		if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + (key - GLFW_KEY_KP_0));
		}

		if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F15)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (key - GLFW_KEY_F1));
		}

		switch (key)
		{
		case GLFW_KEY_ESCAPE: return KeyCode::Escape;
		case GLFW_KEY_LEFT_CONTROL: return KeyCode::LControl;
		case GLFW_KEY_LEFT_SHIFT: return KeyCode::LShift;
		case GLFW_KEY_LEFT_ALT: return KeyCode::LAlt;
		case GLFW_KEY_LEFT_SUPER: return KeyCode::LSystem;
		case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RControl;
		case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RShift;
		case GLFW_KEY_RIGHT_ALT: return KeyCode::RAlt;
		case GLFW_KEY_RIGHT_SUPER: return KeyCode::RSystem;
		case GLFW_KEY_MENU: return KeyCode::Menu;
		case GLFW_KEY_LEFT_BRACKET: return KeyCode::LBracket;
		case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RBracket;
		case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
		case GLFW_KEY_COMMA: return KeyCode::Comma;
		case GLFW_KEY_PERIOD: return KeyCode::Period;
		case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
		case GLFW_KEY_SLASH: return KeyCode::Slash;
		case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
		case GLFW_KEY_GRAVE_ACCENT: return KeyCode::Grave;
		case GLFW_KEY_EQUAL: return KeyCode::Equal;
		case GLFW_KEY_MINUS: return KeyCode::Hyphen;
		case GLFW_KEY_SPACE: return KeyCode::Space;
		case GLFW_KEY_ENTER:
		case GLFW_KEY_KP_ENTER: return KeyCode::Enter;
		case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
		case GLFW_KEY_TAB: return KeyCode::Tab;
		case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
		case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
		case GLFW_KEY_END: return KeyCode::End;
		case GLFW_KEY_HOME: return KeyCode::Home;
		case GLFW_KEY_INSERT: return KeyCode::Insert;
		case GLFW_KEY_DELETE: return KeyCode::Delete;
		case GLFW_KEY_KP_ADD: return KeyCode::Add;
		case GLFW_KEY_KP_SUBTRACT: return KeyCode::Subtract;
		case GLFW_KEY_KP_MULTIPLY: return KeyCode::Multiply;
		case GLFW_KEY_KP_DIVIDE: return KeyCode::Divide;
		case GLFW_KEY_LEFT: return KeyCode::Left;
		case GLFW_KEY_RIGHT: return KeyCode::Right;
		case GLFW_KEY_UP: return KeyCode::Up;
		case GLFW_KEY_DOWN: return KeyCode::Down;
		case GLFW_KEY_PAUSE: return KeyCode::Pause;
		default: return KeyCode::Unknown;
		}
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
