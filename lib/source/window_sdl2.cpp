// window_sdl2.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window_sdl2.h"

#include "builtin_font.h"
#include "color.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

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

	SDL2Window::SDL2Window(const WindowDesc& windowDesc)
	{
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cout << "error: failed to initialize SDL2: " << SDL_GetError() << std::endl;
			return;
		}

		ttfInitialized = (TTF_Init() == 0);
		if (!ttfInitialized)
		{
			std::cout << "error: failed to initialize SDL2_ttf: " << TTF_GetError() << std::endl;
		}

		constexpr int imgFlags = IMG_INIT_JPG | IMG_INIT_PNG;
		imgInitialized = (IMG_Init(imgFlags) & imgFlags) == imgFlags;
		if (!imgInitialized)
		{
			std::cout << "error: failed to initialize SDL2_image: " << IMG_GetError() << std::endl;
		}

		Uint32 flags = SDL_WINDOW_SHOWN;
		if (windowDesc.fullscreen == "true")
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		window = SDL_CreateWindow(windowDesc.name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			static_cast<int>(windowDesc.width), static_cast<int>(windowDesc.height), flags);

		if (!window)
		{
			std::cout << "error: failed to create SDL2 window: " << SDL_GetError() << std::endl;
			return;
		}

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		if (!renderer)
		{
			std::cout << "error: failed to create SDL2 renderer: " << SDL_GetError() << std::endl;
			SDL_DestroyWindow(window);
			window = nullptr;
			return;
		}

		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

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

		for (auto& [size, loadedFont] : fontsBySize)
		{
			if (loadedFont) { TTF_CloseFont(loadedFont); }
		}
		fontsBySize.clear();

		if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
		if (window) { SDL_DestroyWindow(window); window = nullptr; }

		if (imgInitialized) { IMG_Quit(); imgInitialized = false; }
		if (ttfInitialized) { TTF_Quit(); ttfInitialized = false; }

		SDL_Quit();
		isOpenFlag = false;
	}

	void SDL2Window::init(std::vector<Object>& objects)
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
				// that needs the renderer, the textures and SDL_ttf. They are
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
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		SDL_RenderClear(renderer);
	}

	void SDL2Window::draw(Object& object)
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

		int w = 0, h = 0;
		SDL_QueryTexture(visual.texture, nullptr, nullptr, &w, &h);

		const SDL_Rect dst{ static_cast<int>(object.position.x), static_cast<int>(object.position.y), w, h };
		SDL_RenderCopyEx(renderer, visual.texture, nullptr, &dst, 0.0, nullptr, visual.flip);
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

	TTF_Font* SDL2Window::getFont(int pointSize)
	{
		auto it = fontsBySize.find(pointSize);
		if (it != fontsBySize.end())
		{
			return it->second;
		}

		TTF_Font* loaded = TTF_OpenFont("assets/tuffy.ttf", pointSize);
		if (!loaded)
		{
			// Said once, not once for every size asked for.
			if (!fontMissing)
			{
				std::cout << "error: failed to load font: assets/tuffy.ttf: " << TTF_GetError() << " - drawing text with the built-in 8x8 font instead" << std::endl;
			}
			fontMissing = true;
		}

		fontsBySize.emplace(pointSize, loaded);
		return loaded;
	}

	void SDL2Window::reloadTargetTexture(CachedVisual& visual, int width, int height)
	{
		if (visual.texture)
		{
			SDL_DestroyTexture(visual.texture);
		}

		visual.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
			width > 0 ? width : 1, height > 0 ? height : 1);
		SDL_SetTextureBlendMode(visual.texture, SDL_BLENDMODE_BLEND);
	}

	void SDL2Window::buildShapeOnly(Object& object, CachedVisual& visual)
	{
		switch (object.shapeKind)
		{
		case ShapeKind::Circle:    buildCircle(object, visual); break;
		case ShapeKind::Rectangle: buildRectangle(object, visual); break;
		case ShapeKind::Text:      buildText(object, visual); break;
		case ShapeKind::Image:     buildImage(object, visual); break;
		case ShapeKind::Line:      buildLines(object, visual); break;
		case ShapeKind::Unknown:   break;
		}
	}

	void SDL2Window::buildCircle(Object& object, CachedVisual& visual)
	{
		const float radiusF = std::stof(object.spriteParams.at(1));
		const int radius = static_cast<int>(radiusF);
		const int diameter = static_cast<int>(std::ceil(radiusF * 2.0f));

		reloadTargetTexture(visual, diameter, diameter);
		visual.flip = SDL_FLIP_NONE;

		SDL_SetRenderTarget(renderer, visual.texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		const Color c = colorFromName(object.spriteParams.at(3));
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y <= radius * radius)
				{
					SDL_RenderDrawPoint(renderer, radius + x, radius + y);
				}
			}
		}

		SDL_SetRenderTarget(renderer, nullptr);
	}

	void SDL2Window::buildRectangle(Object& object, CachedVisual& visual)
	{
		const int width = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(1))));
		const int height = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(2))));

		reloadTargetTexture(visual, width, height);
		visual.flip = SDL_FLIP_NONE;

		SDL_SetRenderTarget(renderer, visual.texture);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);

		const Color c = colorFromName(object.spriteParams.at(3));
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		const SDL_Rect rect{ 0, 0, width, height };
		SDL_RenderFillRect(renderer, &rect);

		SDL_SetRenderTarget(renderer, nullptr);
	}

	void SDL2Window::buildText(Object& object, CachedVisual& visual)
	{
		if (visual.texture)
		{
			SDL_DestroyTexture(visual.texture);
			visual.texture = nullptr;
		}
		visual.flip = SDL_FLIP_NONE;

		const int requestedSize = std::stoi(object.spriteParams.at(2));
		TTF_Font* font = getFont(requestedSize);
		if (!font)
		{
			buildBitmap(object, visual, rasterizeText(object.spriteParams.at(1), requestedSize, colorFromName(object.spriteParams.at(3))));
			return;
		}

		const Color c = colorFromName(object.spriteParams.at(3));
		const SDL_Color sdlColor{ c.r, c.g, c.b, c.a };

		SDL_Surface* surface = TTF_RenderUTF8_Blended(font, object.spriteParams.at(1).c_str(), sdlColor);
		if (!surface)
		{
			std::cout << "error: SDL2_ttf: failed to render text: " << TTF_GetError() << std::endl;
			return;
		}

		visual.texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
	}

	void SDL2Window::buildImage(Object& object, CachedVisual& visual)
	{
		if (visual.texture)
		{
			SDL_DestroyTexture(visual.texture);
			visual.texture = nullptr;
		}

		const std::string& imageFile = object.spriteParams.at(1);
		SDL_Surface* surface = IMG_Load(imageFile.c_str());
		if (!surface)
		{
			std::cout << "error: SDL2_image: failed to load " << imageFile << ": " << IMG_GetError() << '\n';
			exit(EXIT_FAILURE);
		}

		visual.texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);

		visual.flip = SDL_FLIP_NONE;
		if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.horizontal")
		{
			visual.flip = SDL_FLIP_HORIZONTAL;
		}
		else if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.vertical")
		{
			visual.flip = SDL_FLIP_VERTICAL;
		}
	}

	// A sprite of lines was already drawn, pixel by pixel, when the game loaded
	// (see Bitmap); all that is left is to show that picture.
	void SDL2Window::buildLines(Object& object, CachedVisual& visual)
	{
		buildBitmap(object, visual, *object.bitmap);
	}

	// Shows a picture the engine drew itself: a sprite of lines, or text in
	// the built-in font.
	void SDL2Window::buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap)
	{
		if (visual.texture)
		{
			SDL_DestroyTexture(visual.texture);
			visual.texture = nullptr;
		}
		visual.flip = SDL_FLIP_NONE;

		if (bitmap.rgba.empty())
		{
			reloadTargetTexture(visual, 1, 1);
			return;
		}

		// A surface that only looks at the bitmap's pixels (they are copied
		// into the texture below, and the surface is freed before the bitmap).
		SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(const_cast<std::uint8_t*>(bitmap.rgba.data()),
			bitmap.width, bitmap.height, 32, bitmap.width * 4, SDL_PIXELFORMAT_RGBA32);
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
		}
	}

	void SDL2Window::finalizeVisual(Object& object, CachedVisual& visual)
	{
		int w = 0, h = 0;
		if (visual.texture)
		{
			SDL_QueryTexture(visual.texture, nullptr, nullptr, &w, &h);
		}

		object.size.x = static_cast<float>(w);
		object.size.y = static_cast<float>(h);
		object.sizeKnown = true;
		object.visualDirty = false;
	}
}
