// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window.h"

#include "window_sfml.h"
#include "window_raylib.h"
#include "window_sdl2.h"
#include "window_opengl.h"

#include <stdexcept>

namespace xge
{
	Embedding WindowFactory::embedding(WindowBackend backend) noexcept
	{
		switch (backend)
		{
		case WindowBackend::SFML3:
		case WindowBackend::SDL2:
			return Embedding::NativeWindow;

		case WindowBackend::Raylib:
		case WindowBackend::OpenGL:
			return Embedding::BackBuffer;
		}

		return Embedding::BackBuffer;
	}

	std::unique_ptr<Window> WindowFactory::create(const WindowDesc& windowDesc, WindowBackend backend, const WindowTarget& target)
	{
		if (target.kind == WindowTarget::Kind::NativeWindow && (embedding(backend) != Embedding::NativeWindow || !target.nativeHandle))
		{
			throw std::invalid_argument("this window library cannot draw into a window it did not make");
		}

		if (target.kind == WindowTarget::Kind::BackBuffer && embedding(backend) != Embedding::BackBuffer)
		{
			throw std::invalid_argument("this window library cannot draw to a back buffer");
		}

		switch (backend)
		{
		case WindowBackend::SFML3:  return std::make_unique<SFMLWindow>(windowDesc, target);
		case WindowBackend::Raylib: return std::make_unique<RaylibWindow>(windowDesc, target);
		case WindowBackend::SDL2:   return std::make_unique<SDL2Window>(windowDesc, target);
		case WindowBackend::OpenGL: return std::make_unique<OpenGLWindow>(windowDesc, target);
		}

		return std::make_unique<SFMLWindow>(windowDesc, target);
	}
}
