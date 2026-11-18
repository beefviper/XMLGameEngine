// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window/window.h"
#include "window/null.h"
#include "picture/color.h"
#include "picture/font.h"
#include "picture/shapes.h"

#include <charconv>
#include <cmath>
#include <stdexcept>

// Each backend is compiled in only when it is built (XGE_WITH_<NAME>, set by
// scripts/cmake/targets.cmake), so its header, which includes its library's, is
// only included then.
#ifdef XGE_WITH_SFML3
#include "window/sfml.h"
#endif
#ifdef XGE_WITH_RAYLIB
#include "window/raylib.h"
#endif
#ifdef XGE_WITH_SDL2
#include "window/sdl2.h"
#endif
#ifdef XGE_WITH_OPENGL
#include "window/opengl.h"
#endif
#ifdef XGE_WITH_VULKAN
#include "window/vulkan.h"
#endif
#ifdef XGE_WITH_DIRECT3D9
#include "window/direct3d9.h"
#endif
#ifdef XGE_WITH_DIRECT3D10
#include "window/direct3d10.h"
#endif
#ifdef XGE_WITH_DIRECT3D11
#include "window/direct3d11.h"
#endif
#ifdef XGE_WITH_DIRECT3D12
#include "window/direct3d12.h"
#endif

namespace xge
{
	namespace
	{
		// A number in a picture's key, exactly.
		std::string keyNumber(float number)
		{
			char text[32];
			const auto [end, failed] = std::to_chars(text, text + sizeof text, number);
			(void)failed;
			return std::string(text, end);
		}
	}

	PictureId Window::pictureOf(const Object& object)
	{
		const PictureId shown = pictureNow(object);
		return shown.get() ? shown : gamePictures->picture("nothing", [] { return Bitmap{}; });
	}

	PictureId Window::pictureNow(const Object& object)
	{
		const Picture& picture = object.drawn.picture;
		const float scale = pictureScale();
		switch (picture.kind)
		{
		case ShapeKind::Line:
			if (object.drawn.bitmap) { return object.drawn.bitmap; }
			break;
		case ShapeKind::Image:
			if (picture.image) { return picture.image; }
			break;
		case ShapeKind::Circle:
			return gamePictures->picture("circle|" + keyNumber(picture.radius) + "|" + picture.color.str() + "|" + keyNumber(scale),
				[&] { return drawCircle(picture.radius, colorFromName(picture.color), scale); });
		case ShapeKind::Rectangle:
			return gamePictures->picture("rectangle|" + keyNumber(picture.size.x) + "," + keyNumber(picture.size.y) + "|" + picture.color.str() + "|" + keyNumber(scale),
				[&] { return drawRectangle(picture.size.x, picture.size.y, colorFromName(picture.color), scale); });
		case ShapeKind::Text:
		{
			const int pixels = std::max(1, static_cast<int>(std::lround(static_cast<float>(picture.textSize) * scale)));
			const std::string from = picture.text + '\n' + picture.font.str() + '\n' + std::to_string(pixels) + '\n' + picture.color.str();
			DrawnText& text = texts[object.id.index];
			if (text.picture && text.drawnFrom == from) { return text.picture; }
			Bitmap drawn = drawText(picture.text, pixels, colorFromName(picture.color), textFontFile(picture.font));
			if (text.picture) { gamePictures->replace(text.picture, std::move(drawn)); }
			else { text.picture = gamePictures->own(std::move(drawn)); }
			text.drawnFrom = from;
			return text.picture;
		}
		case ShapeKind::Unknown:
			break;
		}
		return gamePictures->picture("nothing", [] { return Bitmap{}; });
	}

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
#ifdef XGE_WITH_VULKAN
			{ WindowBackend::Vulkan, "vulkan", true },
#else
			{ WindowBackend::Vulkan, "vulkan", false },
#endif
#ifdef XGE_WITH_DIRECT3D9
			{ WindowBackend::Direct3D9, "direct3d9", true },
#else
			{ WindowBackend::Direct3D9, "direct3d9", false },
#endif
#ifdef XGE_WITH_DIRECT3D10
			{ WindowBackend::Direct3D10, "direct3d10", true },
#else
			{ WindowBackend::Direct3D10, "direct3d10", false },
#endif
#ifdef XGE_WITH_DIRECT3D11
			{ WindowBackend::Direct3D11, "direct3d11", true },
#else
			{ WindowBackend::Direct3D11, "direct3d11", false },
#endif
#ifdef XGE_WITH_DIRECT3D12
			{ WindowBackend::Direct3D12, "direct3d12", true },
#else
			{ WindowBackend::Direct3D12, "direct3d12", false },
#endif
			{ WindowBackend::None, "none", true },
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
		return availableBackends().front(); // None is always there, and last
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
#ifdef XGE_WITH_VULKAN
		if (backend == WindowBackend::Vulkan) { return std::make_unique<VulkanWindow>(windowDesc); }
#endif
#ifdef XGE_WITH_DIRECT3D9
		if (backend == WindowBackend::Direct3D9) { return std::make_unique<Direct3D9Window>(windowDesc); }
#endif
#ifdef XGE_WITH_DIRECT3D10
		if (backend == WindowBackend::Direct3D10) { return std::make_unique<Direct3D10Window>(windowDesc); }
#endif
#ifdef XGE_WITH_DIRECT3D11
		if (backend == WindowBackend::Direct3D11) { return std::make_unique<Direct3D11Window>(windowDesc); }
#endif
#ifdef XGE_WITH_DIRECT3D12
		if (backend == WindowBackend::Direct3D12) { return std::make_unique<Direct3D12Window>(windowDesc); }
#endif
		if (backend == WindowBackend::None) { return std::make_unique<NullWindow>(windowDesc); }

		std::string built;
		for (const WindowBackend each : availableBackends())
		{
			built += (built.empty() ? "" : ", ") + name(each);
		}
		throw std::runtime_error("the " + name(backend) + " window library is not built into this program (built with: "
			+ built + ")");
	}
}
