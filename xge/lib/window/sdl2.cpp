// sdl2.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window/sdl2.h"

#include "picture/color.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		constexpr std::pair<SDL_Keycode, KeyCode> keyTable[] = {
			{ SDLK_a, KeyCode::A }, { SDLK_b, KeyCode::B }, { SDLK_c, KeyCode::C }, { SDLK_d, KeyCode::D },
			{ SDLK_e, KeyCode::E }, { SDLK_f, KeyCode::F }, { SDLK_g, KeyCode::G }, { SDLK_h, KeyCode::H },
			{ SDLK_i, KeyCode::I }, { SDLK_j, KeyCode::J }, { SDLK_k, KeyCode::K }, { SDLK_l, KeyCode::L },
			{ SDLK_m, KeyCode::M }, { SDLK_n, KeyCode::N }, { SDLK_o, KeyCode::O }, { SDLK_p, KeyCode::P },
			{ SDLK_q, KeyCode::Q }, { SDLK_r, KeyCode::R }, { SDLK_s, KeyCode::S }, { SDLK_t, KeyCode::T },
			{ SDLK_u, KeyCode::U }, { SDLK_v, KeyCode::V }, { SDLK_w, KeyCode::W }, { SDLK_x, KeyCode::X },
			{ SDLK_y, KeyCode::Y }, { SDLK_z, KeyCode::Z },

			{ SDLK_0, KeyCode::Num0 }, { SDLK_1, KeyCode::Num1 }, { SDLK_2, KeyCode::Num2 },
			{ SDLK_3, KeyCode::Num3 }, { SDLK_4, KeyCode::Num4 }, { SDLK_5, KeyCode::Num5 },
			{ SDLK_6, KeyCode::Num6 }, { SDLK_7, KeyCode::Num7 }, { SDLK_8, KeyCode::Num8 },
			{ SDLK_9, KeyCode::Num9 },

			{ SDLK_ESCAPE, KeyCode::Escape },
			{ SDLK_LCTRL, KeyCode::LControl }, { SDLK_LSHIFT, KeyCode::LShift },
			{ SDLK_LALT, KeyCode::LAlt }, { SDLK_LGUI, KeyCode::LSystem },
			{ SDLK_RCTRL, KeyCode::RControl }, { SDLK_RSHIFT, KeyCode::RShift },
			{ SDLK_RALT, KeyCode::RAlt }, { SDLK_RGUI, KeyCode::RSystem },
			{ SDLK_APPLICATION, KeyCode::Menu },

			{ SDLK_LEFTBRACKET, KeyCode::LBracket }, { SDLK_RIGHTBRACKET, KeyCode::RBracket },
			{ SDLK_SEMICOLON, KeyCode::Semicolon }, { SDLK_COMMA, KeyCode::Comma }, { SDLK_PERIOD, KeyCode::Period },
			{ SDLK_QUOTE, KeyCode::Apostrophe }, { SDLK_SLASH, KeyCode::Slash },
			{ SDLK_BACKSLASH, KeyCode::Backslash }, { SDLK_BACKQUOTE, KeyCode::Grave },
			{ SDLK_EQUALS, KeyCode::Equal }, { SDLK_MINUS, KeyCode::Hyphen },

			{ SDLK_SPACE, KeyCode::Space }, { SDLK_RETURN, KeyCode::Enter }, { SDLK_KP_ENTER, KeyCode::Enter },
			{ SDLK_BACKSPACE, KeyCode::Backspace }, { SDLK_TAB, KeyCode::Tab },
			{ SDLK_PAGEUP, KeyCode::PageUp }, { SDLK_PAGEDOWN, KeyCode::PageDown },
			{ SDLK_END, KeyCode::End }, { SDLK_HOME, KeyCode::Home },
			{ SDLK_INSERT, KeyCode::Insert }, { SDLK_DELETE, KeyCode::Delete },

			{ SDLK_KP_PLUS, KeyCode::Add }, { SDLK_KP_MINUS, KeyCode::Subtract },
			{ SDLK_KP_MULTIPLY, KeyCode::Multiply }, { SDLK_KP_DIVIDE, KeyCode::Divide },

			{ SDLK_LEFT, KeyCode::Left }, { SDLK_RIGHT, KeyCode::Right },
			{ SDLK_UP, KeyCode::Up }, { SDLK_DOWN, KeyCode::Down },

			{ SDLK_KP_0, KeyCode::Numpad0 }, { SDLK_KP_1, KeyCode::Numpad1 }, { SDLK_KP_2, KeyCode::Numpad2 },
			{ SDLK_KP_3, KeyCode::Numpad3 }, { SDLK_KP_4, KeyCode::Numpad4 }, { SDLK_KP_5, KeyCode::Numpad5 },
			{ SDLK_KP_6, KeyCode::Numpad6 }, { SDLK_KP_7, KeyCode::Numpad7 }, { SDLK_KP_8, KeyCode::Numpad8 },
			{ SDLK_KP_9, KeyCode::Numpad9 },

			{ SDLK_F1, KeyCode::F1 }, { SDLK_F2, KeyCode::F2 }, { SDLK_F3, KeyCode::F3 }, { SDLK_F4, KeyCode::F4 },
			{ SDLK_F5, KeyCode::F5 }, { SDLK_F6, KeyCode::F6 }, { SDLK_F7, KeyCode::F7 }, { SDLK_F8, KeyCode::F8 },
			{ SDLK_F9, KeyCode::F9 }, { SDLK_F10, KeyCode::F10 }, { SDLK_F11, KeyCode::F11 }, { SDLK_F12, KeyCode::F12 },
			{ SDLK_F13, KeyCode::F13 }, { SDLK_F14, KeyCode::F14 }, { SDLK_F15, KeyCode::F15 },

			{ SDLK_PAUSE, KeyCode::Pause },
		};
	}

	namespace
	{
		// Only SDL's video goes: the SDL2 audio backend (audio/sdl2.h) may still
		// be playing with SDL's audio, which SDL_Quit() would stop too. The last
		// of the two to finish shuts SDL down.
		void quitVideo()
		{
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			if (SDL_WasInit(0) == 0)
			{
				SDL_Quit();
			}
		}
	}

	SDL2Window::SDL2Window(const WindowDesc& windowDesc)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			throw std::runtime_error(std::string("SDL2 could not start: ") + SDL_GetError());
		}

		Uint32 flags = SDL_WINDOW_SHOWN;
		if (windowDesc.fullscreen == "true")
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		scale = static_cast<float>(std::max(1, windowDesc.scale));
		setPictureScale(scale);
		window = SDL_CreateWindow(windowDesc.name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			static_cast<int>(windowDesc.width * scale), static_cast<int>(windowDesc.height * scale), flags);

		if (!window)
		{
			const std::string why = SDL_GetError();
			quitVideo();
			throw std::runtime_error("SDL2 could not make a window: " + why);
		}

		// The engine sets the game's speed (Engine::loop); the window only waits
		// for the screen's refresh, so a picture is never torn.
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		if (!renderer)
		{
			const std::string why = SDL_GetError();
			SDL_DestroyWindow(window);
			window = nullptr;
			quitVideo();
			throw std::runtime_error("SDL2 could not make a renderer: " + why);
		}

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		padsInitialized = (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0);

		isOpenFlag = true;
	}

	SDL2Window::~SDL2Window()
	{
		close();
	}

	bool SDL2Window::isOpen() const
	{
		return isOpenFlag && !closeRequested;
	}

	void SDL2Window::close()
	{
		if (!isOpenFlag)
		{
			return;
		}

		visuals.clear();

		if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
		if (window) { SDL_DestroyWindow(window); window = nullptr; }


		for (SDL_GameController*& pad : pads)
		{
			if (pad) { SDL_GameControllerClose(pad); pad = nullptr; }
		}
		if (padsInitialized) { SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); padsInitialized = false; }

		quitVideo();
		isOpenFlag = false;
	}

	std::pair<int, int> SDL2Window::position() const
	{
		int x = 0;
		int y = 0;
		SDL_GetWindowPosition(window, &x, &y);
		return { x, y };
	}

	void SDL2Window::setPosition(int x, int y)
	{
		SDL_SetWindowPosition(window, x, y);
	}

	void SDL2Window::setTitle(const std::string& title)
	{
		SDL_SetWindowTitle(window, title.c_str());
	}

	Controls SDL2Window::controls()
	{
		Controls controls;
		if (!isOpenFlag || !(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS))
		{
			return controls;
		}

		int x = 0;
		int y = 0;
		const Uint32 mouse = SDL_GetMouseState(&x, &y);
		controls.pointerInside = SDL_GetMouseFocus() == window;
		controls.pointerX = static_cast<float>(x) / scale;
		controls.pointerY = static_cast<float>(y) / scale;
		controls.mouse = { (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0, (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0, (mouse & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0 };

		if (!padsInitialized)
		{
			return controls;
		}

		// The first two game controllers plugged in, opened when first seen
		// and again after one is pulled out.
		int joystick = 0;
		for (std::size_t pad = 0; pad < padCount; ++pad)
		{
			if (pads[pad] && !SDL_GameControllerGetAttached(pads[pad]))
			{
				SDL_GameControllerClose(pads[pad]);
				pads[pad] = nullptr;
			}
			while (!pads[pad] && joystick < SDL_NumJoysticks())
			{
				const int index = joystick++;
				if (!SDL_IsGameController(index)) { continue; }
				const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
				const bool taken = std::any_of(pads.begin(), pads.end(), [&](SDL_GameController* other)
					{ return other && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(other)) == id; });
				if (!taken) { pads[pad] = SDL_GameControllerOpen(index); }
			}
			if (!pads[pad]) { continue; }

			constexpr SDL_GameControllerButton buttons[padButtonCount] = {
				SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
				SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_START,
				SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
			};
			Pad& now = controls.pads[pad];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = SDL_GameControllerGetButton(pads[pad], buttons[button]) != 0;
			}
			now.x = static_cast<float>(SDL_GameControllerGetAxis(pads[pad], SDL_CONTROLLER_AXIS_LEFTX)) / 32767.0f;
			now.y = static_cast<float>(SDL_GameControllerGetAxis(pads[pad], SDL_CONTROLLER_AXIS_LEFTY)) / 32767.0f;
		}

		return controls;
	}

	void SDL2Window::init(std::vector<Object>& objects)
	{
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
		}
	}

	std::vector<std::pair<KeyCode, bool>> SDL2Window::pollEvents()
	{
		std::vector<std::pair<KeyCode, bool>> events;

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				// Not close(): the engine finishes the frame this was read in
				// (clear, draw, display) before it looks at isOpen() again, and
				// that needs the renderer and the textures. They are
				// torn down by close() when the window is destroyed.
				closeRequested = true;
			}
			else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && event.key.repeat == 0)
			{
				const KeyCode code = sdlKeyToKeyCode(event.key.keysym.sym);
				if (code != KeyCode::Unknown)
				{
					events.emplace_back(code, event.type == SDL_KEYDOWN);
				}
			}
		}

		return events;
	}

	void SDL2Window::clear(const std::string& colorName)
	{
		const Color c = colorFromName(colorName);
		// The game's pixels, scale window pixels each.
		SDL_RenderSetScale(renderer, scale, scale);
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		SDL_RenderClear(renderer);
	}

	void SDL2Window::prepare(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.texture || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}
	}

	void SDL2Window::draw(Object& object)
	{
		prepare(object);
		CachedVisual& visual = visuals[object.name];

		if (!visual.texture)
		{
			return;
		}

		// Whole game pixels, so a texture is never sampled between two of
		// its pixels; stretched over the object's size in the game's pixels.
		const Vector2f size = object.pictureSize();
		const SDL_FRect where{ std::floor(object.position.x), std::floor(object.position.y), size.x, size.y };
		SDL_RenderCopyF(renderer, visual.texture, nullptr, &where);
	}

	float SDL2Window::fit() const
	{
		return scale;
	}

	void SDL2Window::display()
	{
		SDL_RenderPresent(renderer);
	}

	KeyCode SDL2Window::sdlKeyToKeyCode(SDL_Keycode key) noexcept
	{
		for (const auto& [candidate, code] : keyTable)
		{
			if (candidate == key) { return code; }
		}

		return KeyCode::Unknown;
	}

	Bitmap SDL2Window::capture()
	{
		Bitmap bitmap;
		if (SDL_GetRendererOutputSize(renderer, &bitmap.width, &bitmap.height) != 0 || bitmap.width <= 0 || bitmap.height <= 0) { return {}; }
		bitmap.rgba.resize(static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height) * 4);
		if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, bitmap.rgba.data(), bitmap.width * 4) != 0) { return {}; }
		return bitmap;
	}

	// The engine's picture of the object, at pictureScale().
	void SDL2Window::build(Object& object, CachedVisual& visual)
	{
		object.visualDirty = false;
		visual.drawnAt = pictureScale();
		if (visual.texture)
		{
			SDL_DestroyTexture(visual.texture);
			visual.texture = nullptr;
		}

		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		if (picture->rgba.empty())
		{
			return;
		}

		// A surface that only looks at the bitmap's pixels (they are copied
		// into the texture below, and the surface is freed before the bitmap).
		SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(const_cast<std::uint8_t*>(picture->rgba.data()),
			picture->width, picture->height, 32, picture->width * 4, SDL_PIXELFORMAT_RGBA32);
		if (!surface)
		{
			std::cout << "error: SDL2: failed to make a surface from the picture of '" << object.name << "': " << SDL_GetError() << std::endl;
			return;
		}

		visual.texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);

		if (visual.texture)
		{
			SDL_SetTextureBlendMode(visual.texture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(visual.texture, object.drawnAtScale() ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
		}
	}
}
