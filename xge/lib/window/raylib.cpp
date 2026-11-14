// raylib.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window/raylib.h"

#include <rlgl.h>

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

		// Not raylib's own WHITE macro: it expands to an unqualified
		// `Color{...}` literal, which inside namespace xge resolves to
		// xge::Color instead of the global raylib ::Color it actually means.
		constexpr ::Color kWhite{ 255, 255, 255, 255 };
	}

	RaylibWindow::RaylibWindow(const WindowDesc& windowDesc)
	{
		// raylib logs every texture it makes; only problems are worth printing.
		SetTraceLogLevel(LOG_WARNING);
		scale = static_cast<float>(std::max(1, windowDesc.scale));
		setPictureScale(scale);
		// The engine sets the game's speed (Engine::loop); the window only waits
		// for the screen's refresh, so a picture is never torn.
		SetConfigFlags(FLAG_VSYNC_HINT);
		InitWindow(static_cast<int>(windowDesc.width * scale), static_cast<int>(windowDesc.height * scale), windowDesc.name.c_str());

		if (!IsWindowReady())
		{
			throw std::runtime_error("raylib could not start");
		}

		if (windowDesc.fullscreen == "true")
		{
			ToggleFullscreen();
		}

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
		for (auto& object : objects)
		{
			build(object, visuals[object.name]);
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

	Controls RaylibWindow::controls()
	{
		Controls controls;
		if (!isOpenFlag || !IsWindowFocused())
		{
			return controls;
		}

		const Vector2 at = GetMousePosition();
		controls.pointerInside = IsCursorOnScreen();
		controls.pointerX = at.x / scale;
		controls.pointerY = at.y / scale;
		controls.mouse = { IsMouseButtonDown(MOUSE_BUTTON_LEFT), IsMouseButtonDown(MOUSE_BUTTON_RIGHT), IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) };

		constexpr int buttons[padButtonCount] = {
			GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP,
			GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_MIDDLE_RIGHT,
			GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
		};
		for (int pad = 0; pad < static_cast<int>(padCount); ++pad)
		{
			if (!IsGamepadAvailable(pad)) { continue; }
			Pad& now = controls.pads[static_cast<std::size_t>(pad)];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = IsGamepadButtonDown(pad, buttons[button]);
			}
			now.x = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_X);
			now.y = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_LEFT_Y);
		}

		return controls;
	}

	void RaylibWindow::clear(const std::string& colorName)
	{
		BeginDrawing();
		ClearBackground(toRaylibColor(colorFromName(colorName)));
	}

	void RaylibWindow::prepare(Object& object)
	{
		CachedVisual& visual = visuals[object.name];

		if (object.visualDirty || !visual.loaded || (object.drawnAtScale() && visual.drawnAt != pictureScale()))
		{
			build(object, visual);
		}
	}

	void RaylibWindow::draw(Object& object)
	{
		prepare(object);
		CachedVisual& visual = visuals[object.name];
		if (!visual.loaded)
		{
			return;
		}

		// Stretched over the object's size in the game's pixels, scale window
		// pixels to each of them (<window><scale>).
		const Vector2f size = object.pictureSize();
		const ::Rectangle source{ 0.0f, 0.0f, static_cast<float>(visual.texture.width), static_cast<float>(visual.texture.height) };
		const ::Rectangle where{ std::floor(object.position.x) * scale, std::floor(object.position.y) * scale, size.x * scale, size.y * scale };
		DrawTexturePro(visual.texture, source, where, ::Vector2{ 0.0f, 0.0f }, 0.0f, kWhite);
	}

	float RaylibWindow::fit() const
	{
		return scale;
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
				std::cout << "raylib: built with custom frame control, so the window swaps and polls for each frame itself" << std::endl;
			}
		}

		if (manualFrameControl)
		{
			SwapScreenBuffer();
			PollInputEvents();
		}
	}


	Bitmap RaylibWindow::capture()
	{
		// raylib keeps what it is asked to draw until the frame ends; it is
		// drawn now, so the screen holds it.
		rlDrawRenderBatchActive();
		Image shown = LoadImageFromScreen();
		if (shown.data == nullptr) { return {}; }
		ImageFormat(&shown, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		Bitmap bitmap;
		bitmap.width = shown.width;
		bitmap.height = shown.height;
		const auto* pixels = static_cast<const std::uint8_t*>(shown.data);
		bitmap.rgba.assign(pixels, pixels + static_cast<std::size_t>(shown.width) * static_cast<std::size_t>(shown.height) * 4);
		UnloadImage(shown);
		return bitmap;
	}

	// The engine's picture of the object, at pictureScale().
	void RaylibWindow::build(Object& object, CachedVisual& visual)
	{
		object.visualDirty = false;
		visual.drawnAt = pictureScale();

		const std::shared_ptr<const Bitmap> picture = object.drawnPicture(pictureScale());
		if (picture->rgba.empty())
		{
			if (visual.loaded) { UnloadTexture(visual.texture); }
			visual.loaded = false;
			return;
		}

		// A picture the same size as the one before (a score that changed
		// digits) goes into the texture it already has.
		if (visual.loaded && visual.texture.width == picture->width && visual.texture.height == picture->height)
		{
			UpdateTexture(visual.texture, picture->rgba.data());
		}
		else
		{
			if (visual.loaded) { UnloadTexture(visual.texture); }

			// raylib only reads the pixels while making the texture, so the
			// bitmap's own memory can be lent to it.
			Image image{};
			image.data = const_cast<std::uint8_t*>(picture->rgba.data());
			image.width = picture->width;
			image.height = picture->height;
			image.mipmaps = 1;
			image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
			visual.texture = LoadTextureFromImage(image);
			visual.loaded = visual.texture.id != 0;
		}

		if (visual.loaded)
		{
			SetTextureFilter(visual.texture, object.drawnAtScale() ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
		}
	}
}
