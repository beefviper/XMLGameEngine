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

#include "audio/audio.h"
#include "cli.h"
#include "picture/font.h"
#include "window/null.h"
#include "window/window.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	template <typename Backend>
	bool contains(const std::vector<Backend>& list, Backend backend)
	{
		return std::find(list.begin(), list.end(), backend) != list.end();
	}
}

TEST_CASE("a test build has every backend", "[backends]")
{
	CHECK(WindowFactory::availableBackends() == std::vector<WindowBackend>{
		WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL, WindowBackend::None });
	CHECK(XmlDocumentFactory::availableBackends() == std::vector<XmlBackend>{
		XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml });
	CHECK(AudioFactory::availableBackends() == std::vector<AudioBackend>{
		AudioBackend::SFML3, AudioBackend::Raylib, AudioBackend::SDL2, AudioBackend::None });
}

TEST_CASE("available() agrees with the list", "[backends]")
{
	for (const WindowBackend backend : { WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL, WindowBackend::None })
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
		ContainsSubstring("is not built into this program") && ContainsSubstring("built with: sfml3, raylib, sdl2, opengl"));
}

TEST_CASE("the usage text lists the libraries that are built, and the defaults", "[backends]")
{
	const std::string usage = usageText();

	CHECK_THAT(usage, ContainsSubstring("window library: sfml3, raylib, sdl2, opengl, none (default sfml3)"));
	CHECK_THAT(usage, ContainsSubstring("XML library: xerces, tinyxml2, pugixml, rapidxml (default xerces)"));
	CHECK_THAT(usage, ContainsSubstring("sound library: sfml3, raylib, sdl2, none (default sfml3)"));
}

TEST_CASE("the null window opens nothing but measures every object as a window would", "[backends]")
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
		return objects.front().size;
	};

	const Vector2f circle = measured([](Picture& picture) { picture.kind = ShapeKind::Circle; picture.radius = 4.5f; });
	CHECK(circle.x == 9.0f);
	CHECK(circle.y == 9.0f);

	const Vector2f box = measured([](Picture& picture) { picture.kind = ShapeKind::Rectangle; picture.size = { 10.0f, 2.5f }; });
	CHECK(box.x == 10.0f);
	CHECK(box.y == 3.0f);

	// As the engine draws it for every window, in the standard font.
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
