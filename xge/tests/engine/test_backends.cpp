// test_backends.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for which window, sound and XML backends a program was built
// with (options.cmake: SFML 3 and Xerces alone by default, every one when the
// tests are built): what each factory says it can make, the defaults that
// follow it, the plain names, and what asking for a backend that is not built
// says. The test suite is always built with every backend (BUILD_TESTING turns
// them all on), so a backend that is not built is stood in for here by a value
// the enums have no name for; a default build is looked at by hand, with
// `xgecli -w raylib`.
//
// One test, hidden from ordinary runs because it opens real windows, checks
// every window backend against the contract (docs/reference/engine.md "What a
// window must do"): `xgetest "[window]"`, under Xvfb on Linux
// (`xvfb-run -a output/Debug/xgetest "[window]"`).

#include "audio/audio.h"
#include "cli.h"
#include "picture/font.h"
#include "window/null.h"
#include "window/window.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Direct3D is a Windows library: a test build there has both as well.
#ifdef _WIN32
	constexpr bool kWindows = true;
	const std::string kWindowNames = "sfml3, raylib, sdl2, opengl, vulkan, direct3d9, direct3d11, direct3d12";
#else
	constexpr bool kWindows = false;
	const std::string kWindowNames = "sfml3, raylib, sdl2, opengl, vulkan";
#endif

	template <typename Backend>
	bool contains(const std::vector<Backend>& list, Backend backend)
	{
		return std::find(list.begin(), list.end(), backend) != list.end();
	}
}

TEST_CASE("a test build has every backend", "[backends]")
{
	std::vector<WindowBackend> windows{ WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL, WindowBackend::Vulkan };
	if constexpr (kWindows) { windows.insert(windows.end(), { WindowBackend::Direct3D9, WindowBackend::Direct3D11, WindowBackend::Direct3D12 }); }
	windows.push_back(WindowBackend::None);
	CHECK(WindowFactory::availableBackends() == windows);
	CHECK(XmlDocumentFactory::availableBackends() == std::vector<XmlBackend>{
		XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml });
	CHECK(AudioFactory::availableBackends() == std::vector<AudioBackend>{
		AudioBackend::SFML3, AudioBackend::Raylib, AudioBackend::SDL2, AudioBackend::None });
}

TEST_CASE("available() agrees with the list", "[backends]")
{
	for (const WindowBackend backend : { WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL, WindowBackend::Vulkan, WindowBackend::Direct3D9, WindowBackend::Direct3D11, WindowBackend::Direct3D12, WindowBackend::None })
	{
		CHECK(WindowFactory::available(backend) == contains(WindowFactory::availableBackends(), backend));
	}
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		CHECK(XmlDocumentFactory::available(backend) == contains(XmlDocumentFactory::availableBackends(), backend));
	}
	for (const AudioBackend backend : { AudioBackend::SFML3, AudioBackend::Raylib, AudioBackend::SDL2, AudioBackend::None })
	{
		CHECK(AudioFactory::available(backend) == contains(AudioFactory::availableBackends(), backend));
	}

	// A value no enumerator has is never built.
	CHECK_FALSE(WindowFactory::available(static_cast<WindowBackend>(99)));
	CHECK_FALSE(XmlDocumentFactory::available(static_cast<XmlBackend>(99)));
	CHECK_FALSE(AudioFactory::available(static_cast<AudioBackend>(99)));
}

TEST_CASE("sound and a window are never missing: None is always the last choice", "[backends]")
{
	CHECK(WindowFactory::available(WindowBackend::None));
	CHECK(WindowFactory::availableBackends().back() == WindowBackend::None);
	CHECK(WindowFactory::name(WindowBackend::None) == "none");
	CHECK(AudioFactory::available(AudioBackend::None));
	CHECK(AudioFactory::availableBackends().back() == AudioBackend::None);
}

TEST_CASE("the defaults are the first backend built: SFML 3 and Xerces when they are", "[backends]")
{
	CHECK(WindowFactory::defaultBackend() == WindowFactory::availableBackends().front());
	CHECK(XmlDocumentFactory::defaultBackend() == XmlDocumentFactory::availableBackends().front());
	CHECK(AudioFactory::defaultBackend() == AudioFactory::availableBackends().front());

	CHECK(WindowFactory::defaultBackend() == WindowBackend::SFML3);
	CHECK(XmlDocumentFactory::defaultBackend() == XmlBackend::Xerces);
	CHECK(AudioFactory::defaultBackend() == AudioBackend::SFML3);

	// What the command line uses when it is given nothing.
	const CliOptions options = parseCommandLine({});
	CHECK(options.window == WindowFactory::defaultBackend());
	CHECK(options.xml == XmlDocumentFactory::defaultBackend());
	CHECK(options.audio == AudioFactory::defaultBackend());
}

TEST_CASE("each backend has a plain lower case name", "[backends]")
{
	CHECK(WindowFactory::name(WindowBackend::SFML3) == "sfml3");
	CHECK(WindowFactory::name(WindowBackend::Raylib) == "raylib");
	CHECK(WindowFactory::name(WindowBackend::SDL2) == "sdl2");
	CHECK(WindowFactory::name(WindowBackend::OpenGL) == "opengl");
	CHECK(WindowFactory::name(WindowBackend::Vulkan) == "vulkan");
	CHECK(WindowFactory::name(WindowBackend::Direct3D9) == "direct3d9");
	CHECK(WindowFactory::name(WindowBackend::Direct3D11) == "direct3d11");
	CHECK(WindowFactory::name(WindowBackend::Direct3D12) == "direct3d12");
	CHECK(XmlDocumentFactory::name(XmlBackend::Xerces) == "xerces");
	CHECK(XmlDocumentFactory::name(XmlBackend::TinyXml2) == "tinyxml2");
	CHECK(XmlDocumentFactory::name(XmlBackend::PugiXml) == "pugixml");
	CHECK(XmlDocumentFactory::name(XmlBackend::RapidXml) == "rapidxml");
	CHECK(AudioFactory::name(AudioBackend::SFML3) == "sfml3");
	CHECK(AudioFactory::name(AudioBackend::Raylib) == "raylib");
	CHECK(AudioFactory::name(AudioBackend::SDL2) == "sdl2");
	CHECK(AudioFactory::name(AudioBackend::None) == "none");

	// And the command line takes exactly those names.
	CHECK(windowBackendName(WindowBackend::SDL2) == WindowFactory::name(WindowBackend::SDL2));
	CHECK(xmlBackendName(XmlBackend::PugiXml) == XmlDocumentFactory::name(XmlBackend::PugiXml));
	CHECK(audioBackendName(AudioBackend::None) == AudioFactory::name(AudioBackend::None));
}

TEST_CASE("an XML backend that is built makes a document; one that is not says which are built", "[backends]")
{
	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		CHECK(XmlDocumentFactory::create(backend) != nullptr);
	}

	REQUIRE_THROWS_WITH(XmlDocumentFactory::create(static_cast<XmlBackend>(99)),
		ContainsSubstring("is not built into this program") && ContainsSubstring("built with: xerces, tinyxml2, pugixml, rapidxml"));
}

TEST_CASE("a sound backend that is not built says which are built, and None plays nothing", "[backends]")
{
	CHECK(AudioFactory::create(AudioBackend::None) != nullptr);

	REQUIRE_THROWS_WITH(AudioFactory::create(static_cast<AudioBackend>(99)),
		ContainsSubstring("is not built into this program") && ContainsSubstring("built with: sfml3, raylib, sdl2, none"));
}

TEST_CASE("a window backend that is not built says which are built", "[backends]")
{
	REQUIRE_THROWS_WITH(WindowFactory::create(WindowDesc{}, static_cast<WindowBackend>(99)),
		ContainsSubstring("is not built into this program") && ContainsSubstring("built with: " + kWindowNames + ", none"));
}

TEST_CASE("the usage text lists the libraries that are built, and the defaults", "[backends]")
{
	const std::string usage = usageText();

	CHECK_THAT(usage, ContainsSubstring("window library: " + kWindowNames + ", none (default sfml3)"));
	CHECK_THAT(usage, ContainsSubstring("XML library: xerces, tinyxml2, pugixml, rapidxml (default xerces)"));
	CHECK_THAT(usage, ContainsSubstring("sound library: sfml3, raylib, sdl2, none (default sfml3)"));
}

TEST_CASE("a picture's size is the game's own, and the null window opens nothing", "[backends]")
{
	const auto measured = [](auto setUp)
	{
		Object object;
		setUp(object.picture);
		object.visualDirty = true;
		std::vector<Object> objects{ object };
		NullWindow window;
		window.init(objects);
		CHECK_FALSE(objects.front().visualDirty);
		return objects.front().pictureSize();
	};

	// Shapes on whole pixels, rounded up.
	const Vector2f circle = measured([](Picture& picture) { picture.kind = ShapeKind::Circle; picture.radius = 4.5f; });
	CHECK(circle.x == 9.0f);
	CHECK(circle.y == 9.0f);

	const Vector2f box = measured([](Picture& picture) { picture.kind = ShapeKind::Rectangle; picture.size = { 10.0f, 2.5f }; });
	CHECK(box.x == 10.0f);
	CHECK(box.y == 3.0f);

	// A text as the engine draws it for every window, in the standard font.
	const Vector2f text = measured([](Picture& picture) { picture.kind = ShapeKind::Text; picture.text = "SCORE\n12"; picture.textSize = 32; });
	const Bitmap drawn = drawText("SCORE\n12", 32, Color{}, textFontFile(""));
	CHECK(text.x == static_cast<float>(drawn.width));
	CHECK(text.y == static_cast<float>(drawn.height));
	CHECK(drawn.width > 0);

	// An image file by its pixels (the tests run in games/).
	const Vector2f image = measured([](Picture& picture) { picture.kind = ShapeKind::Image; picture.file = "assets/paddle.jpg"; });
	CHECK(image.x == 30.0f);
	CHECK(image.y == 150.0f);

	NullWindow window;
	CHECK(window.isOpen());
	CHECK(window.pollEvents().empty());
	window.close();
	CHECK_FALSE(window.isOpen());
}

namespace
{
	// The key A pressed (or let go) in the window with this title, as a
	// person would: XTest through xdotool on X, SendInput on Windows. False
	// where neither is to hand, and the key is not tried.
	bool sendKeyA(const std::string& title, bool down)
	{
#ifdef _WIN32
		HWND found = FindWindowA(nullptr, title.c_str());
		if (!found) { return false; }
		SetForegroundWindow(found);
		INPUT input{};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = 'A';
		// SDL2 reads a key by its scan code, not its virtual key: give both.
		input.ki.wScan = static_cast<WORD>(MapVirtualKeyW('A', MAPVK_VK_TO_VSC));
		input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
		return SendInput(1, &input, sizeof(INPUT)) == 1;
#else
		if (std::system("command -v xdotool > /dev/null 2>&1") != 0) { return false; }
		const std::string command = "xdotool search --sync --name '^" + title + "$' windowfocus --sync " + (down ? "keydown" : "keyup") + " a > /dev/null 2>&1";
		return std::system(command.c_str()) == 0;
#endif
	}

	bool haveScreen()
	{
#ifdef _WIN32
		return true;
#else
		return std::getenv("DISPLAY") != nullptr || std::getenv("WAYLAND_DISPLAY") != nullptr;
#endif
	}

	struct Rgb
	{
		int r;
		int g;
		int b;
	};

	Rgb pixelAt(const Bitmap& shot, int x, int y)
	{
		const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(shot.width) + static_cast<std::size_t>(x)) * 4;
		return { shot.rgba[at], shot.rgba[at + 1], shot.rgba[at + 2] };
	}

	bool looksLike(const Rgb& got, const Rgb& want)
	{
		UNSCOPED_INFO("the pixel is " << got.r << ", " << got.g << ", " << got.b);
		return std::abs(got.r - want.r) <= 8 && std::abs(got.g - want.g) <= 8 && std::abs(got.b - want.b) <= 8;
	}

	// A window keeps each picture by its object's name.
	Object pictureAt(const std::string& name, float x, float y, void (*setUp)(Object&))
	{
		Object object;
		object.name = name;
		object.position = { x, y };
		object.isDrawn = true;
		setUp(object);
		object.size = object.pictureSize();
		object.visualDirty = true;
		return object;
	}

	// Draws one frame and looks at what came of it: polls, clears, draws
	// every object, and keeps what it was given back from the window.
	std::vector<std::pair<KeyCode, bool>> frame(Window& window, std::vector<Object>& objects, Bitmap* shot = nullptr)
	{
		std::vector<std::pair<KeyCode, bool>> keys = window.pollEvents();
		window.clear("color.black");
		for (Object& object : objects) { window.draw(object); }
		if (shot) { *shot = window.capture(); }
		window.display();
		return keys;
	}

	bool sawKey(const std::vector<std::pair<KeyCode, bool>>& keys, bool down)
	{
		return std::find(keys.begin(), keys.end(), std::make_pair(KeyCode::A, down)) != keys.end();
	}
}

TEST_CASE("every window backend keeps the window contract", "[.][window]")
{
	if (!haveScreen()) { SKIP("no screen to open a window on (run under xvfb-run)"); }

	for (const WindowBackend backend : WindowFactory::availableBackends())
	{
		if (backend == WindowBackend::None) { continue; }
		const std::string name = WindowFactory::name(backend);
		DYNAMIC_SECTION(name)
		{
			// Rule 1: it opens at the game's size, here 160 by 120 (wider than the
			// narrowest window Windows allows with a title bar).
			WindowDesc desc;
			desc.name = "xge window test " + name;
			desc.width = 160.0f;
			desc.height = 120.0f;
			desc.scale = 1;
			desc.background = "color.black";
			desc.fullscreen = "false";
			desc.framerate = 60;
			std::unique_ptr<Window> window = WindowFactory::create(desc, backend);
			REQUIRE(window->isOpen());
			CHECK(window->fit() == 1.0f);
			CHECK(window->pictureScale() == 1.0f);

			// Rules 3 to 5: three known pictures, a bitmap, a rectangle and a
			// circle, each where its position says.
			std::vector<Object> objects{
				pictureAt("bitmap", 4.0f, 4.0f, [](Object& o)
				{
					o.picture.kind = ShapeKind::Line;
					auto red = std::make_shared<Bitmap>();
					red->width = 8;
					red->height = 8;
					for (int i = 0; i < 64; ++i) { red->rgba.insert(red->rgba.end(), { 255, 0, 0, 255 }); }
					o.bitmap = red;
					o.picture.size = { 8.0f, 8.0f };
				}),
				pictureAt("rectangle", 30.0f, 4.0f, [](Object& o) { o.picture.kind = ShapeKind::Rectangle; o.picture.size = { 10.0f, 6.0f }; o.picture.color = "color.green"; }),
				pictureAt("circle", 20.0f, 28.0f, [](Object& o) { o.picture.kind = ShapeKind::Circle; o.picture.radius = 6.0f; o.picture.color = "color.blue"; }),
			};
			window->init(objects);
			for (const Object& object : objects) { CHECK_FALSE(object.visualDirty); }

			Bitmap shot;
			for (int i = 0; i < 5; ++i) { frame(*window, objects, i == 4 ? &shot : nullptr); }
			if (shot.rgba.empty())
			{
				WARN(name << " cannot read its pixels back; the pictures were drawn but not looked at");
			}
			else
			{
				CHECK(shot.width == 160);
				CHECK(shot.height == 120);
				REQUIRE(shot.rgba.size() == 160u * 120u * 4u);
				CHECK(looksLike(pixelAt(shot, 7, 7), { 255, 0, 0 }));      // the bitmap
				CHECK(looksLike(pixelAt(shot, 35, 6), { 0, 255, 0 }));     // the rectangle
				CHECK(looksLike(pixelAt(shot, 26, 34), { 0, 0, 255 }));    // the circle's middle
				CHECK(looksLike(pixelAt(shot, 2, 2), { 0, 0, 0 }));        // the background
				CHECK(looksLike(pixelAt(shot, 150, 110), { 0, 0, 0 }));
				CHECK(looksLike(pixelAt(shot, 20, 28), { 0, 0, 0 }));      // outside the circle's round
			}

			// Rule 6: a key pressed in the window is reported down, then up.
			if (!sendKeyA(desc.name, true))
			{
				WARN("no way to press a key here (xdotool on X, SendInput on Windows); keys not tried");
			}
			else
			{
				bool pressed = false;
				for (int i = 0; i < 120 && !pressed; ++i) { pressed = sawKey(frame(*window, objects), true); }
				CHECK(pressed);
				REQUIRE(sendKeyA(desc.name, false));
				bool released = false;
				for (int i = 0; i < 120 && !released; ++i) { released = sawKey(frame(*window, objects), false); }
				CHECK(released);
			}

			window->close();
			CHECK_FALSE(window->isOpen());
		}
	}
}
