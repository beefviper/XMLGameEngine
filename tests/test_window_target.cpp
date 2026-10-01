// test_window_target.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for where a Window can draw (window.h): which backends can be
// given a window of a front end's own and which draw to a back buffer, and
// that WindowFactory refuses a target a backend cannot use. No window library
// is started: the refusals come before any backend is built.
//
// Test names must not start with "-" or contain a comma: ctest hands the name to
// Catch2 on its command line, which reads them as an option or a list.

#include "window.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

using namespace xge;

TEST_CASE("SFML3 and SDL2 can draw into a window made for them", "[window_target]")
{
	CHECK(WindowFactory::embedding(WindowBackend::SFML3) == Embedding::NativeWindow);
	CHECK(WindowFactory::embedding(WindowBackend::SDL2) == Embedding::NativeWindow);
}

TEST_CASE("raylib and OpenGL draw to a back buffer", "[window_target]")
{
	CHECK(WindowFactory::embedding(WindowBackend::Raylib) == Embedding::BackBuffer);
	CHECK(WindowFactory::embedding(WindowBackend::OpenGL) == Embedding::BackBuffer);
}

TEST_CASE("a target the backend cannot use is refused before anything starts", "[window_target]")
{
	const WindowDesc desc;
	int standIn = 0;

	WindowTarget native;
	native.kind = WindowTarget::Kind::NativeWindow;
	native.nativeHandle = &standIn;

	WindowTarget buffer;
	buffer.kind = WindowTarget::Kind::BackBuffer;

	CHECK_THROWS_AS(WindowFactory::create(desc, WindowBackend::Raylib, native), std::invalid_argument);
	CHECK_THROWS_AS(WindowFactory::create(desc, WindowBackend::OpenGL, native), std::invalid_argument);
	CHECK_THROWS_AS(WindowFactory::create(desc, WindowBackend::SFML3, buffer), std::invalid_argument);
	CHECK_THROWS_AS(WindowFactory::create(desc, WindowBackend::SDL2, buffer), std::invalid_argument);
}

TEST_CASE("a native window target needs a window", "[window_target]")
{
	const WindowDesc desc;

	WindowTarget native;
	native.kind = WindowTarget::Kind::NativeWindow;

	CHECK_THROWS_AS(WindowFactory::create(desc, WindowBackend::SFML3, native), std::invalid_argument);
}
