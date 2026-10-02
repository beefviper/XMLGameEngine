// window_raylib.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window_raylib.h"

#include "builtin_font.h"
#include "color.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// Every {raylib key, KeyCode} pair this backend recognizes. Raylib's
		// own KeyboardKey enum only goes up to F12 (no F13-F15) and has no
		// caps/scroll/num-lock or print-screen equivalents in KeyCode's
		// vocabulary - those are simply never produced by this backend.
		constexpr std::pair<int, KeyCode> keyTable[] = {
			{ KEY_A, KeyCode::A }, { KEY_B, KeyCode::B }, { KEY_C, KeyCode::C }, { KEY_D, KeyCode::D },
			{ KEY_E, KeyCode::E }, { KEY_F, KeyCode::F }, { KEY_G, KeyCode::G }, { KEY_H, KeyCode::H },
			{ KEY_I, KeyCode::I }, { KEY_J, KeyCode::J }, { KEY_K, KeyCode::K }, { KEY_L, KeyCode::L },
			{ KEY_M, KeyCode::M }, { KEY_N, KeyCode::N }, { KEY_O, KeyCode::O }, { KEY_P, KeyCode::P },
			{ KEY_Q, KeyCode::Q }, { KEY_R, KeyCode::R }, { KEY_S, KeyCode::S }, { KEY_T, KeyCode::T },
			{ KEY_U, KeyCode::U }, { KEY_V, KeyCode::V }, { KEY_W, KeyCode::W }, { KEY_X, KeyCode::X },
			{ KEY_Y, KeyCode::Y }, { KEY_Z, KeyCode::Z },

			{ KEY_ZERO, KeyCode::Num0 }, { KEY_ONE, KeyCode::Num1 }, { KEY_TWO, KeyCode::Num2 },
			{ KEY_THREE, KeyCode::Num3 }, { KEY_FOUR, KeyCode::Num4 }, { KEY_FIVE, KeyCode::Num5 },
			{ KEY_SIX, KeyCode::Num6 }, { KEY_SEVEN, KeyCode::Num7 }, { KEY_EIGHT, KeyCode::Num8 },
			{ KEY_NINE, KeyCode::Num9 },

			{ KEY_ESCAPE, KeyCode::Escape },
			{ KEY_LEFT_CONTROL, KeyCode::LControl }, { KEY_LEFT_SHIFT, KeyCode::LShift },
			{ KEY_LEFT_ALT, KeyCode::LAlt }, { KEY_LEFT_SUPER, KeyCode::LSystem },
			{ KEY_RIGHT_CONTROL, KeyCode::RControl }, { KEY_RIGHT_SHIFT, KeyCode::RShift },
			{ KEY_RIGHT_ALT, KeyCode::RAlt }, { KEY_RIGHT_SUPER, KeyCode::RSystem },
			{ KEY_KB_MENU, KeyCode::Menu },

			{ KEY_LEFT_BRACKET, KeyCode::LBracket }, { KEY_RIGHT_BRACKET, KeyCode::RBracket },
			{ KEY_SEMICOLON, KeyCode::Semicolon }, { KEY_COMMA, KeyCode::Comma }, { KEY_PERIOD, KeyCode::Period },
			{ KEY_APOSTROPHE, KeyCode::Apostrophe }, { KEY_SLASH, KeyCode::Slash },
			{ KEY_BACKSLASH, KeyCode::Backslash }, { KEY_GRAVE, KeyCode::Grave },
			{ KEY_EQUAL, KeyCode::Equal }, { KEY_MINUS, KeyCode::Hyphen },

			{ KEY_SPACE, KeyCode::Space }, { KEY_ENTER, KeyCode::Enter }, { KEY_KP_ENTER, KeyCode::Enter },
			{ KEY_BACKSPACE, KeyCode::Backspace }, { KEY_TAB, KeyCode::Tab },
			{ KEY_PAGE_UP, KeyCode::PageUp }, { KEY_PAGE_DOWN, KeyCode::PageDown },
			{ KEY_END, KeyCode::End }, { KEY_HOME, KeyCode::Home },
			{ KEY_INSERT, KeyCode::Insert }, { KEY_DELETE, KeyCode::Delete },

			{ KEY_KP_ADD, KeyCode::Add }, { KEY_KP_SUBTRACT, KeyCode::Subtract },
			{ KEY_KP_MULTIPLY, KeyCode::Multiply }, { KEY_KP_DIVIDE, KeyCode::Divide },

			{ KEY_LEFT, KeyCode::Left }, { KEY_RIGHT, KeyCode::Right },
			{ KEY_UP, KeyCode::Up }, { KEY_DOWN, KeyCode::Down },

			{ KEY_KP_0, KeyCode::Numpad0 }, { KEY_KP_1, KeyCode::Numpad1 }, { KEY_KP_2, KeyCode::Numpad2 },
			{ KEY_KP_3, KeyCode::Numpad3 }, { KEY_KP_4, KeyCode::Numpad4 }, { KEY_KP_5, KeyCode::Numpad5 },
			{ KEY_KP_6, KeyCode::Numpad6 }, { KEY_KP_7, KeyCode::Numpad7 }, { KEY_KP_8, KeyCode::Numpad8 },
			{ KEY_KP_9, KeyCode::Numpad9 },

			{ KEY_F1, KeyCode::F1 }, { KEY_F2, KeyCode::F2 }, { KEY_F3, KeyCode::F3 }, { KEY_F4, KeyCode::F4 },
			{ KEY_F5, KeyCode::F5 }, { KEY_F6, KeyCode::F6 }, { KEY_F7, KeyCode::F7 }, { KEY_F8, KeyCode::F8 },
			{ KEY_F9, KeyCode::F9 }, { KEY_F10, KeyCode::F10 }, { KEY_F11, KeyCode::F11 }, { KEY_F12, KeyCode::F12 },

			{ KEY_PAUSE, KeyCode::Pause },
		};

		::Color toRaylibColor(const xge::Color& c) noexcept
		{
			return ::Color{ c.r, c.g, c.b, c.a };
		}

		constexpr ::Color kTransparent{ 0, 0, 0, 0 };

		// Not raylib's own WHITE macro: it expands to an unqualified
		// `Color{...}` literal, which inside namespace xge resolves to
		// xge::Color instead of the global raylib ::Color it actually means.
		constexpr ::Color kWhite{ 255, 255, 255, 255 };
	}

	RaylibWindow::RaylibWindow(const WindowDesc& windowDesc)
	{
		// raylib logs every texture it makes; only problems are worth printing.
		SetTraceLogLevel(LOG_WARNING);
		InitWindow(static_cast<int>(windowDesc.width), static_cast<int>(windowDesc.height), windowDesc.name.c_str());

		if (!IsWindowReady())
		{
			throw std::runtime_error("raylib could not start");
		}

		if (windowDesc.fullscreen == "true")
		{
			ToggleFullscreen();
		}

		SetTargetFPS(windowDesc.framerate);
		frameTarget = windowDesc.framerate > 0 ? 1.0 / windowDesc.framerate : 0.0;

		// Raylib closes the window on Escape by default. Escape is just a key
		// here (a game can bind it, Lunar Lander pauses with it), as it is on
		// the other backends; closing is the window's own close button.
		SetExitKey(KEY_NULL);

		isOpenFlag = true;
	}

	RaylibWindow::~RaylibWindow()
	{
		close();
	}

	bool RaylibWindow::isOpen() const
	{
		return isOpenFlag && !WindowShouldClose();
	}

	void RaylibWindow::close()
	{
		if (isOpenFlag)
		{
			visuals.clear();

			if (customFont)
			{
				UnloadFont(font);
			}

			CloseWindow();
			isOpenFlag = false;
		}
	}

	std::pair<int, int> RaylibWindow::position() const
	{
		return { static_cast<int>(GetWindowPosition().x), static_cast<int>(GetWindowPosition().y) };
	}

	void RaylibWindow::setPosition(int x, int y)
	{
		SetWindowPosition(x, y);
	}

	void RaylibWindow::setTitle(const std::string& title)
	{
		SetWindowTitle(title.c_str());
	}

	void RaylibWindow::init(std::vector<Object>& objects)
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

	std::vector<std::pair<KeyCode, bool>> RaylibWindow::pollEvents()
	{
		// Raylib has no event queue to drain. It reads the keyboard and the
		// close button itself, once per frame, at the end of EndDrawing() (see
		// display()), so there is nothing to pump here: calling
		// PollInputEvents() as well would read the keyboard a second time
		// each frame, and whatever the first read saw (nearly everything,
		// since the frame is spent waiting inside EndDrawing()) would be
		// reset before it could be reported, losing key presses and window
		// close requests. Instead, compare each key with how it was the last
		// time this was called, which gives the same press/release changes
		// as the other backends' queues.
		std::vector<std::pair<KeyCode, bool>> events;

		for (const auto& [rayKey, code] : keyTable)
		{
			const bool down = IsKeyDown(rayKey);

			if (down != keyWasDown[static_cast<std::size_t>(rayKey)])
			{
				keyWasDown[static_cast<std::size_t>(rayKey)] = down;
				events.emplace_back(code, down);
			}
		}

		return events;
	}

	void RaylibWindow::clear(const std::string& colorName)
	{
		BeginDrawing();
		ClearBackground(toRaylibColor(colorFromName(colorName)));
	}

	void RaylibWindow::draw(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.loaded)
		{
			// Never repeats init()'s grid math - see the matching comment in
			// SFMLWindow::draw() (window_sfml.cpp).
			buildShapeOnly(object, visual);
			finalizeVisual(object, visual);
		}

		const Texture2D& texture = visual.renderTexture.texture;

		// Raylib render textures are stored upside-down relative to the
		// screen; a negative source height flips it back on the way out -
		// this is raylib's own documented convention, not a mistake.
		const ::Rectangle source{ 0.0f, 0.0f, static_cast<float>(texture.width), -static_cast<float>(texture.height) };
		const ::Vector2 position{ object.position.x, object.position.y };

		DrawTextureRec(texture, source, position, kWhite);
	}

	void RaylibWindow::display()
	{
		EndDrawing();

		// A raylib built with SUPPORT_CUSTOM_FRAME_CONTROL (some package
		// builds are) leaves all of this to the program: EndDrawing() then
		// neither swaps the buffers (the window stays blank), nor reads the
		// keyboard and window messages (the window stops responding), nor
		// waits for the frame time (the loop runs flat out). It can be told
		// from the other kind by EndDrawing() not having worked out the time
		// the frame took, which GetFrameTime() would give.
		if (!frameControlChecked)
		{
			frameControlChecked = true;
			manualFrameControl = (GetFrameTime() == 0.0f);

			if (manualFrameControl)
			{
				std::cout << "raylib: built with custom frame control, so the window swaps, polls and waits for each frame itself" << std::endl;
			}
		}

		if (manualFrameControl)
		{
			SwapScreenBuffer();
			PollInputEvents();

			const double elapsed = GetTime() - frameStart;
			if (elapsed < frameTarget)
			{
				WaitTime(frameTarget - elapsed);
			}
			frameStart = GetTime();
		}
	}


	const Font& RaylibWindow::getFont()
	{
		if (!fontLoaded)
		{
			const Font loaded = LoadFontEx("assets/tuffy.ttf", 96, nullptr, 0);
			if (loaded.texture.id != 0)
			{
				font = loaded;
				customFont = true;
			}
			else
			{
				std::cout << "error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead" << std::endl;
				font = GetFontDefault();
				fontMissing = true;
			}
			fontLoaded = true;
		}

		return font;
	}

	void RaylibWindow::reloadTexture(CachedVisual& visual, int width, int height)
	{
		const int wanted = width > 0 ? width : 1;
		const int tall = height > 0 ? height : 1;

		if (visual.loaded)
		{
			// A visual that is rebuilt again at the same size (a score or a
			// fuel readout that changed digits) is drawn over in the texture it
			// already has: making a new one is a texture, a depth buffer and a
			// framebuffer, once per change. Every build clears it first.
			if (visual.renderTexture.texture.width == wanted && visual.renderTexture.texture.height == tall)
			{
				return;
			}

			UnloadRenderTexture(visual.renderTexture);
		}

		visual.renderTexture = LoadRenderTexture(wanted, tall);
		visual.loaded = true;
	}

	void RaylibWindow::buildShapeOnly(Object& object, CachedVisual& visual)
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

	void RaylibWindow::buildCircle(Object& object, CachedVisual& visual)
	{
		const float radius = std::stof(object.spriteParams.at(1));
		const int diameter = static_cast<int>(std::ceil(radius * 2.0f));

		reloadTexture(visual, diameter, diameter);

		BeginTextureMode(visual.renderTexture);
		ClearBackground(kTransparent);
		DrawCircle(static_cast<int>(radius), static_cast<int>(radius), radius, toRaylibColor(colorFromName(object.spriteParams.at(3))));
		EndTextureMode();
	}

	void RaylibWindow::buildRectangle(Object& object, CachedVisual& visual)
	{
		const int width = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(1))));
		const int height = static_cast<int>(std::ceil(std::stof(object.spriteParams.at(2))));

		reloadTexture(visual, width, height);

		BeginTextureMode(visual.renderTexture);
		ClearBackground(kTransparent);
		DrawRectangle(0, 0, width, height, toRaylibColor(colorFromName(object.spriteParams.at(3))));
		EndTextureMode();
	}

	void RaylibWindow::buildText(Object& object, CachedVisual& visual)
	{
		const Font& f = getFont();
		const std::string& text = object.spriteParams.at(1);
		const float fontSize = std::stof(object.spriteParams.at(2));

		if (fontMissing)
		{
			buildBitmap(object, visual, rasterizeText(text, static_cast<int>(std::lround(fontSize)), colorFromName(object.spriteParams.at(3))));
			return;
		}

		const ::Vector2 measured = MeasureTextEx(f, text.c_str(), fontSize, 1.0f);
		const int width = static_cast<int>(std::ceil(measured.x));
		const int height = static_cast<int>(std::ceil(measured.y));

		reloadTexture(visual, width, height);

		BeginTextureMode(visual.renderTexture);
		ClearBackground(kTransparent);
		DrawTextEx(f, text.c_str(), ::Vector2{ 0.0f, 0.0f }, fontSize, 1.0f, toRaylibColor(colorFromName(object.spriteParams.at(3))));
		EndTextureMode();
	}

	void RaylibWindow::buildImage(Object& object, CachedVisual& visual)
	{
		const std::string& imageFile = object.spriteParams.at(1);

		Image image = LoadImage(imageFile.c_str());
		if (image.data == nullptr)
		{
			throw std::runtime_error("raylib could not load the image " + imageFile);
		}

		if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.horizontal")
		{
			ImageFlipHorizontal(&image);
		}
		else if (object.spriteParams.size() > 2 && object.spriteParams.at(2) == "flip.vertical")
		{
			ImageFlipVertical(&image);
		}

		const int width = image.width;
		const int height = image.height;

		reloadTexture(visual, width, height);

		const Texture2D texture = LoadTextureFromImage(image);
		UnloadImage(image);

		BeginTextureMode(visual.renderTexture);
		ClearBackground(kTransparent);
		DrawTexture(texture, 0, 0, kWhite);
		EndTextureMode();

		UnloadTexture(texture);
	}

	// A sprite of lines was already drawn, pixel by pixel, when the game loaded
	// (see Bitmap); all that is left is to show that picture.
	void RaylibWindow::buildLines(Object& object, CachedVisual& visual)
	{
		buildBitmap(object, visual, *object.bitmap);
	}

	// Shows a picture the engine drew itself: a sprite of lines, or text in
	// the built-in font.
	void RaylibWindow::buildBitmap(Object& object, CachedVisual& visual, const Bitmap& bitmap)
	{
		(void)object;

		reloadTexture(visual, bitmap.width, bitmap.height);

		// raylib only reads the pixels while making the texture, so the
		// bitmap's own memory can be lent to it.
		Texture2D texture{};
		if (!bitmap.rgba.empty())
		{
			Image image{};
			image.data = const_cast<std::uint8_t*>(bitmap.rgba.data());
			image.width = bitmap.width;
			image.height = bitmap.height;
			image.mipmaps = 1;
			image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

			texture = LoadTextureFromImage(image);
		}

		BeginTextureMode(visual.renderTexture);
		ClearBackground(kTransparent);

		if (texture.id != 0)
		{
			DrawTexture(texture, 0, 0, kWhite);
		}

		EndTextureMode();

		// Only now: raylib batches what DrawTexture asks for and draws it
		// when the texture mode ends, so the texture has to still exist until
		// then (unloading it before EndTextureMode drew nothing).
		if (texture.id != 0)
		{
			UnloadTexture(texture);
		}
	}

	void RaylibWindow::finalizeVisual(Object& object, CachedVisual& visual)
	{
		object.size.x = static_cast<float>(visual.renderTexture.texture.width);
		object.size.y = static_cast<float>(visual.renderTexture.texture.height);
		object.sizeKnown = true;
		object.visualDirty = false;
	}
}
