// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window.h"

#include <stdexcept>

// Each backend is compiled in only when it is built (XGE_WITH_<NAME>, set by
// scripts/cmake/targets.cmake), so its header, which includes its library's, is
// only included then.
#ifdef XGE_WITH_SFML3
#include "window_sfml.h"
#endif
#ifdef XGE_WITH_RAYLIB
#include "window_raylib.h"
#endif
#ifdef XGE_WITH_SDL2
#include "window_sdl2.h"
#endif
#ifdef XGE_WITH_OPENGL
#include "window_opengl.h"
#endif

namespace xge
{
	namespace
	{
		struct Entry
		{
			WindowBackend backend;
			const char* name;
			bool built;
		};

		// In WindowBackend's order, which is the order of preference for the default.
		const Entry entries[] = {
#ifdef XGE_WITH_SFML3
			{ WindowBackend::SFML3, "sfml3", true },
#else
			{ WindowBackend::SFML3, "sfml3", false },
#endif
#ifdef XGE_WITH_RAYLIB
			{ WindowBackend::Raylib, "raylib", true },
#else
			{ WindowBackend::Raylib, "raylib", false },
#endif
#ifdef XGE_WITH_SDL2
			{ WindowBackend::SDL2, "sdl2", true },
#else
			{ WindowBackend::SDL2, "sdl2", false },
#endif
#ifdef XGE_WITH_OPENGL
			{ WindowBackend::OpenGL, "opengl", true },
#else
			{ WindowBackend::OpenGL, "opengl", false },
#endif
		};
	}

	std::string WindowFactory::name(WindowBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.name; }
		}
		return "unknown";
	}

	bool WindowFactory::available(WindowBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.built; }
		}
		return false;
	}

	std::vector<WindowBackend> WindowFactory::availableBackends()
	{
		std::vector<WindowBackend> built;
		for (const Entry& entry : entries)
		{
			if (entry.built) { built.push_back(entry.backend); }
		}
		return built;
	}

	WindowBackend WindowFactory::defaultBackend()
	{
		const std::vector<WindowBackend> built = availableBackends();
		return built.empty() ? WindowBackend::SFML3 : built.front();
	}

	std::unique_ptr<Window> WindowFactory::create(const WindowDesc& windowDesc, WindowBackend backend)
	{
#ifdef XGE_WITH_SFML3
		if (backend == WindowBackend::SFML3) { return std::make_unique<SFMLWindow>(windowDesc); }
#endif
#ifdef XGE_WITH_RAYLIB
		if (backend == WindowBackend::Raylib) { return std::make_unique<RaylibWindow>(windowDesc); }
#endif
#ifdef XGE_WITH_SDL2
		if (backend == WindowBackend::SDL2) { return std::make_unique<SDL2Window>(windowDesc); }
#endif
#ifdef XGE_WITH_OPENGL
		if (backend == WindowBackend::OpenGL) { return std::make_unique<OpenGLWindow>(windowDesc); }
#endif

		std::string built;
		for (const WindowBackend each : availableBackends())
		{
			built += (built.empty() ? "" : ", ") + name(each);
		}
		throw std::runtime_error("the " + name(backend) + " window library is not built into this program (built with: "
			+ (built.empty() ? "none" : built) + ")");
	}
}
