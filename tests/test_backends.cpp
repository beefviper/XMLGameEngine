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

#include "audio.h"
#include "cli.h"
#include "window.h"
#include "xml_document.h"

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
		WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL });
	CHECK(XmlDocumentFactory::availableBackends() == std::vector<XmlBackend>{
		XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml });
	CHECK(AudioFactory::availableBackends() == std::vector<AudioBackend>{
		AudioBackend::SFML3, AudioBackend::Raylib, AudioBackend::SDL2, AudioBackend::None });
}

TEST_CASE("available() agrees with the list", "[backends]")
{
	for (const WindowBackend backend : { WindowBackend::SFML3, WindowBackend::Raylib, WindowBackend::SDL2, WindowBackend::OpenGL })
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

TEST_CASE("sound is never missing: None is always the last choice", "[backends]")
{
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

	CHECK_THAT(usage, ContainsSubstring("window library: sfml3, raylib, sdl2, opengl (default sfml3)"));
	CHECK_THAT(usage, ContainsSubstring("XML library: xerces, tinyxml2, pugixml, rapidxml (default xerces)"));
	CHECK_THAT(usage, ContainsSubstring("sound library: sfml3, raylib, sdl2, none (default sfml3)"));
}
