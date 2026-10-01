// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window.h"

#include "window_sfml.h"
#include "window_raylib.h"
#include "window_sdl2.h"

namespace xge
{
	std::unique_ptr<Window> WindowFactory::create(const WindowDesc& windowDesc, WindowBackend backend)
	{
		switch (backend)
		{
		case WindowBackend::SFML3:  return std::make_unique<SFMLWindow>(windowDesc);
		case WindowBackend::Raylib: return std::make_unique<RaylibWindow>(windowDesc);
		case WindowBackend::SDL2:   return std::make_unique<SDL2Window>(windowDesc);
		}

		return std::make_unique<SFMLWindow>(windowDesc);
	}
}
