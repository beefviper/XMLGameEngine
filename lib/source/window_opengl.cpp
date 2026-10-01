// window_opengl.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "window_opengl.h"

#include "builtin_font.h"
#include "color.h"

// GLFW's own header brings in OpenGL 1.1: what the system's OpenGL library
// exports as it is. It has to come before SDL's, which only want their own.
#include <GLFW/glfw3.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace xge
{
	namespace
	{
		// OpenGL 3.0's framebuffer objects are the one thing used here that
		// the system's OpenGL library does not export (on Windows it stops at
		// 1.1), so they are looked up through GLFW, once the context exists.
		constexpr unsigned int kFramebuffer = 0x8D40;
		constexpr unsigned int kColorAttachment0 = 0x8CE0;
		constexpr unsigned int kFramebufferComplete = 0x8CD5;

		struct FramebufferApi
		{
			void (APIENTRY* generate)(GLsizei, GLuint*){ nullptr };
			void (APIENTRY* bind)(GLenum, GLuint){ nullptr };
			void (APIENTRY* attach)(GLenum, GLenum, GLenum, GLuint, GLint){ nullptr };
			GLenum (APIENTRY* check)(GLenum){ nullptr };
			void (APIENTRY* destroy)(GLsizei, const GLuint*){ nullptr };

			bool loaded() const noexcept { return generate && bind && attach && check && destroy; }

			void load() noexcept
			{
				generate = reinterpret_cast<decltype(generate)>(glfwGetProcAddress("glGenFramebuffers"));
				bind = reinterpret_cast<decltype(bind)>(glfwGetProcAddress("glBindFramebuffer"));
				attach = reinterpret_cast<decltype(attach)>(glfwGetProcAddress("glFramebufferTexture2D"));
				check = reinterpret_cast<decltype(check)>(glfwGetProcAddress("glCheckFramebufferStatus"));
				destroy = reinterpret_cast<decltype(destroy)>(glfwGetProcAddress("glDeleteFramebuffers"));
			}
		};

		// One at a time is all there is: a window makes its own context current
		// and these belong to it.
		FramebufferApi framebufferApi;

		bool imageSizeMatches(const SDL_Surface& surface) noexcept
		{
			return surface.w > 0 && surface.h > 0 && surface.pixels != nullptr;
		}

		// Copies an SDL surface into a Bitmap, whatever format the surface is in.
		Bitmap bitmapFromSurface(SDL_Surface* surface)
		{
			Bitmap bitmap;

			SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
			if (!rgba || !imageSizeMatches(*rgba))
			{
				if (rgba) { SDL_FreeSurface(rgba); }
				return bitmap;
			}

			bitmap.width = rgba->w;
			bitmap.height = rgba->h;
			bitmap.rgba.resize(static_cast<std::size_t>(rgba->w) * static_cast<std::size_t>(rgba->h) * 4);

			const auto* source = static_cast<const std::uint8_t*>(rgba->pixels);
			for (int row = 0; row < rgba->h; ++row)
			{
				std::memcpy(bitmap.rgba.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(rgba->w) * 4,
					source + static_cast<std::size_t>(row) * static_cast<std::size_t>(rgba->pitch),
					static_cast<std::size_t>(rgba->w) * 4);
			}

			SDL_FreeSurface(rgba);
			return bitmap;
		}

		Bitmap filledBitmap(int width, int height)
		{
			Bitmap bitmap;
			bitmap.width = width > 0 ? width : 1;
			bitmap.height = height > 0 ? height : 1;
			bitmap.rgba.assign(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4, 0);
			return bitmap;
		}

		void setPixel(Bitmap& bitmap, int x, int y, const Color& color) noexcept
		{
			if (x < 0 || y < 0 || x >= bitmap.width || y >= bitmap.height)
			{
				return;
			}

			std::uint8_t* pixel = bitmap.rgba.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(bitmap.width) + static_cast<std::size_t>(x)) * 4;
			pixel[0] = color.r;
			pixel[1] = color.g;
			pixel[2] = color.b;
			pixel[3] = color.a;
		}
	}

	OpenGLWindow::CachedVisual::~CachedVisual()
	{
		if (texture)
		{
			glDeleteTextures(1, &texture);
		}
	}

	OpenGLWindow::OpenGLWindow(const WindowDesc& windowDesc, const WindowTarget& target) :
		offscreen(target.kind == WindowTarget::Kind::BackBuffer),
		width(static_cast<int>(windowDesc.width)),
		height(static_cast<int>(windowDesc.height))
	{
		if (!glfwInit())
		{
			throw std::runtime_error("GLFW could not start");
		}

		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_VISIBLE, offscreen ? GLFW_FALSE : GLFW_TRUE);

		GLFWmonitor* monitor = (!offscreen && windowDesc.fullscreen == "true") ? glfwGetPrimaryMonitor() : nullptr;
		window = glfwCreateWindow(width, height, windowDesc.name.c_str(), monitor, nullptr);

		if (!window)
		{
			glfwTerminate();
			throw std::runtime_error("GLFW could not make an OpenGL window");
		}

		glfwMakeContextCurrent(window);

		// The window waits out the frame time itself (display()); waiting for
		// the display's refresh as well would make a 60 frame a second game
		// run at whatever the refresh divides into.
		glfwSwapInterval(0);

		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, &OpenGLWindow::keyCallback);

		ttfInitialized = (TTF_Init() == 0);
		if (!ttfInitialized)
		{
			std::cout << "error: failed to initialize SDL2_ttf: " << TTF_GetError() << std::endl;
		}

		// PNG is needed; JPEG is a bonus (a vcpkg SDL2_image built without
		// libjpeg-turbo has none), so a missing JPEG is reported but does not
		// stop the images that can be read.
		const int imgLoaded = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
		imgInitialized = (imgLoaded & IMG_INIT_PNG) != 0;
		if (!imgInitialized)
		{
			std::cout << "error: failed to initialize SDL2_image: " << IMG_GetError() << std::endl;
		}
		else if ((imgLoaded & IMG_INIT_JPG) == 0)
		{
			std::cout << "warning: this SDL2_image has no JPEG support, so JPEG images will not load" << std::endl;
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		framePeriod = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<double>(windowDesc.framerate > 0 ? 1.0 / windowDesc.framerate : 0.0));
		lastFrame = std::chrono::steady_clock::now();

		if (offscreen)
		{
			captured = filledBitmap(width, height);

			// Draw to a texture of the game's size. If this OpenGL has no
			// framebuffer objects the frame is drawn to the window's own
			// back buffer instead, which a hidden window usually allows.
			framebufferApi.load();
			if (framebufferApi.loaded())
			{
				glGenTextures(1, &targetTexture);
				glBindTexture(GL_TEXTURE_2D, targetTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

				framebufferApi.generate(1, &framebuffer);
				framebufferApi.bind(kFramebuffer, framebuffer);
				framebufferApi.attach(kFramebuffer, kColorAttachment0, GL_TEXTURE_2D, targetTexture, 0);

				if (framebufferApi.check(kFramebuffer) != kFramebufferComplete)
				{
					framebufferApi.bind(kFramebuffer, 0);
					framebufferApi.destroy(1, &framebuffer);
					framebuffer = 0;
					glDeleteTextures(1, &targetTexture);
					targetTexture = 0;
				}
			}
		}
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
		releaseTarget();

		for (auto& [size, loadedFont] : fontsBySize)
		{
			if (loadedFont) { TTF_CloseFont(loadedFont); }
		}
		fontsBySize.clear();

		if (imgInitialized) { IMG_Quit(); }
		if (ttfInitialized) { TTF_Quit(); }

		glfwDestroyWindow(window);
		window = nullptr;
		glfwTerminate();
	}

	void OpenGLWindow::releaseTarget()
	{
		if (framebuffer && framebufferApi.loaded())
		{
			framebufferApi.bind(kFramebuffer, 0);
			framebufferApi.destroy(1, &framebuffer);
			framebuffer = 0;
		}

		if (targetTexture)
		{
			glDeleteTextures(1, &targetTexture);
			targetTexture = 0;
		}
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

	void OpenGLWindow::activate()
	{
		if (window)
		{
			glfwMakeContextCurrent(window);
		}
	}

	void OpenGLWindow::init(std::vector<Object>& objects)
	{
		// Object::position is already final by now - see the identical
		// comment in SFMLWindow::init() (window_sfml.cpp).
		for (auto& object : objects)
		{
			CachedVisual& visual = visuals[object.name];

			buildShapeOnly(object, visual);
			finalizeVisual(object, visual);
		}
	}

	std::vector<std::pair<KeyCode, bool>> OpenGLWindow::pollEvents()
	{
		// A hidden window has no keyboard: the front end that owns the real
		// one passes the keys on itself.
		if (!offscreen)
		{
			glfwPollEvents();
		}

		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(pendingKeys);
		return events;
	}

	void OpenGLWindow::setUpProjection() const
	{
		glViewport(0, 0, width, height);

		// One unit is one pixel, with 0, 0 at the top left like everywhere
		// else in the engine.
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	void OpenGLWindow::clear(const std::string& colorName)
	{
		if (framebuffer)
		{
			framebufferApi.bind(kFramebuffer, framebuffer);
		}

		setUpProjection();

		const Color c = colorFromName(colorName);
		glClearColor(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	void OpenGLWindow::draw(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.texture)
		{
			// Never repeats init()'s grid math - see the matching comment in
			// SFMLWindow::draw() (window_sfml.cpp).
			buildShapeOnly(object, visual);
			finalizeVisual(object, visual);
		}

		if (!visual.texture)
		{
			return;
		}

		// Whole pixels, as the SDL2 backend draws them: a texture sampled
		// halfway between two pixels is smeared.
		const float x0 = static_cast<float>(static_cast<int>(object.position.x));
		const float y0 = static_cast<float>(static_cast<int>(object.position.y));
		const float x1 = x0 + static_cast<float>(visual.width);
		const float y1 = y0 + static_cast<float>(visual.height);

		const float u0 = visual.flipHorizontal ? 1.0f : 0.0f;
		const float u1 = visual.flipHorizontal ? 0.0f : 1.0f;
		const float v0 = visual.flipVertical ? 1.0f : 0.0f;
		const float v1 = visual.flipVertical ? 0.0f : 1.0f;

		glBindTexture(GL_TEXTURE_2D, visual.texture);
		glBegin(GL_QUADS);
		glTexCoord2f(u0, v0); glVertex2f(x0, y0);
		glTexCoord2f(u1, v0); glVertex2f(x1, y0);
		glTexCoord2f(u1, v1); glVertex2f(x1, y1);
		glTexCoord2f(u0, v1); glVertex2f(x0, y1);
		glEnd();
	}

	void OpenGLWindow::display()
	{
		if (offscreen)
		{
			// Read the finished frame, right way up (OpenGL's rows start at the
			// bottom) and opaque (the background was).
			std::vector<std::uint8_t> raw(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

			if (!framebuffer)
			{
				glReadBuffer(GL_BACK);
			}

			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());

			const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
			for (int row = 0; row < height; ++row)
			{
				std::uint8_t* destination = captured.rgba.data() + static_cast<std::size_t>(row) * rowBytes;
				std::memcpy(destination, raw.data() + static_cast<std::size_t>(height - 1 - row) * rowBytes, rowBytes);

				for (std::size_t alpha = 3; alpha < rowBytes; alpha += 4)
				{
					destination[alpha] = 255;
				}
			}

			if (framebuffer)
			{
				framebufferApi.bind(kFramebuffer, 0);
			}

			return;
		}

		glfwSwapBuffers(window);

		// Wait out what is left of the frame time. Sleeping is only good to
		// a few milliseconds, so the last of it is spent yielding.
		const auto target = lastFrame + framePeriod;
		for (;;)
		{
			const auto now = std::chrono::steady_clock::now();
			if (now >= target)
			{
				break;
			}

			const auto remaining = target - now;
			if (remaining > std::chrono::milliseconds(2))
			{
				std::this_thread::sleep_for(remaining - std::chrono::milliseconds(2));
			}
			else
			{
				std::this_thread::yield();
			}
		}

		lastFrame = std::chrono::steady_clock::now();
	}

	const Bitmap* OpenGLWindow::backBuffer() const
	{
		return offscreen ? &captured : nullptr;
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

	TTF_Font* OpenGLWindow::getFont(int pointSize)
	{
		auto it = fontsBySize.find(pointSize);
		if (it != fontsBySize.end())
		{
			return it->second;
		}

		TTF_Font* loaded = ttfInitialized ? TTF_OpenFont("assets/tuffy.ttf", pointSize) : nullptr;
		if (!loaded)
		{
			// Said once, not once for every size asked for.
			if (!fontMissing)
			{
				std::cout << "error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead" << std::endl;
			}
			fontMissing = true;
		}

		fontsBySize.emplace(pointSize, loaded);
		return loaded;
	}

	void OpenGLWindow::upload(CachedVisual& visual, const Bitmap& bitmap)
	{
		const Bitmap blank = bitmap.rgba.empty() ? filledBitmap(1, 1) : Bitmap{};
		const Bitmap& source = bitmap.rgba.empty() ? blank : bitmap;

		if (!visual.texture)
		{
			glGenTextures(1, &visual.texture);
		}

		glBindTexture(GL_TEXTURE_2D, visual.texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, source.width, source.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, source.rgba.data());

		visual.width = source.width;
		visual.height = source.height;
	}

	void OpenGLWindow::buildShapeOnly(Object& object, CachedVisual& visual)
	{
		visual.flipHorizontal = false;
		visual.flipVertical = false;

		switch (object.shapeKind)
		{
		case ShapeKind::Circle:    buildCircle(object, visual); break;
		case ShapeKind::Rectangle: buildRectangle(object, visual); break;
		case ShapeKind::Text:      buildText(object, visual); break;
		case ShapeKind::Image:     buildImage(object, visual); break;
		// A sprite of lines was already drawn, pixel by pixel, when the game
		// loaded (see Bitmap); all that is left is to show that picture.
		case ShapeKind::Line:      upload(visual, *object.bitmap); break;
		case ShapeKind::Unknown:   upload(visual, Bitmap{}); break;
		}
	}

	void OpenGLWindow::buildCircle(Object& object, CachedVisual& visual)
	{
		const float radiusF = std::stof(object.spriteParams.at(1));
		const int radius = static_cast<int>(radiusF);
		const int diameter = static_cast<int>(std::ceil(radiusF * 2.0f));

		Bitmap bitmap = filledBitmap(diameter, diameter);
		const Color c = colorFromName(object.spriteParams.at(3));

		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y <= radius * radius)
				{
					setPixel(bitmap, radius + x, radius + y, c);
				}
			}
		}

		upload(visual, bitmap);
	}

	void OpenGLWindow::buildRectangle(Object& object, CachedVisual& visual)
	{
		const int w = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(1))));
		const int h = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(2))));

		Bitmap bitmap = filledBitmap(w, h);
		const Color c = colorFromName(object.spriteParams.at(3));

		for (int y = 0; y < bitmap.height; ++y)
		{
			for (int x = 0; x < bitmap.width; ++x)
			{
				setPixel(bitmap, x, y, c);
			}
		}

		upload(visual, bitmap);
	}

	void OpenGLWindow::buildText(Object& object, CachedVisual& visual)
	{
		const std::string& text = object.spriteParams.at(1);
		const int requestedSize = std::stoi(object.spriteParams.at(2));
		const Color c = colorFromName(object.spriteParams.at(3));

		TTF_Font* font = getFont(requestedSize);
		if (!font)
		{
			upload(visual, rasterizeText(text, requestedSize, c));
			return;
		}

		const SDL_Color sdlColor{ c.r, c.g, c.b, c.a };
		SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlColor);
		if (!surface)
		{
			// Nothing to draw for an empty text.
			if (!text.empty())
			{
				std::cout << "error: SDL2_ttf: failed to render text: " << TTF_GetError() << std::endl;
			}

			upload(visual, Bitmap{});
			return;
		}

		upload(visual, bitmapFromSurface(surface));
		SDL_FreeSurface(surface);
	}

	void OpenGLWindow::buildImage(Object& object, CachedVisual& visual)
	{
		const std::string& imageFile = object.spriteParams.at(1);
		SDL_Surface* surface = imgInitialized ? IMG_Load(imageFile.c_str()) : nullptr;
		if (!surface)
		{
			throw std::runtime_error("failed to load " + imageFile + ": " + IMG_GetError());
		}

		upload(visual, bitmapFromSurface(surface));
		SDL_FreeSurface(surface);

		if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.horizontal")
		{
			visual.flipHorizontal = true;
		}
		else if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.vertical")
		{
			visual.flipVertical = true;
		}
	}

	void OpenGLWindow::finalizeVisual(Object& object, CachedVisual& visual)
	{
		object.size.x = static_cast<float>(visual.width);
		object.size.y = static_cast<float>(visual.height);
		object.sizeKnown = true;
		object.visualDirty = false;
	}
}
