// audio.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio.h"

#include <stdexcept>

// Each backend is compiled in only when it is built (XGE_WITH_<NAME>, set by
// scripts/cmake/targets.cmake): a sound library comes with its window library.
#ifdef XGE_WITH_SFML3
#include "audio_sfml.h"
#endif
#ifdef XGE_WITH_RAYLIB
#include "audio_raylib.h"
#endif
#ifdef XGE_WITH_SDL2
#include "audio_sdl2.h"
#endif

namespace xge
{
	namespace
	{
		struct Entry
		{
			AudioBackend backend;
			const char* name;
			bool built;
		};

		// In AudioBackend's order, which is the order of preference for the default.
		const Entry entries[] = {
#ifdef XGE_WITH_SFML3
			{ AudioBackend::SFML3, "sfml3", true },
#else
			{ AudioBackend::SFML3, "sfml3", false },
#endif
#ifdef XGE_WITH_RAYLIB
			{ AudioBackend::Raylib, "raylib", true },
#else
			{ AudioBackend::Raylib, "raylib", false },
#endif
#ifdef XGE_WITH_SDL2
			{ AudioBackend::SDL2, "sdl2", true },
#else
			{ AudioBackend::SDL2, "sdl2", false },
#endif
			{ AudioBackend::None, "none", true },
		};
	}

	std::string AudioFactory::name(AudioBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.name; }
		}
		return "unknown";
	}

	bool AudioFactory::available(AudioBackend backend)
	{
		for (const Entry& entry : entries)
		{
			if (entry.backend == backend) { return entry.built; }
		}
		return false;
	}

	std::vector<AudioBackend> AudioFactory::availableBackends()
	{
		std::vector<AudioBackend> built;
		for (const Entry& entry : entries)
		{
			if (entry.built) { built.push_back(entry.backend); }
		}
		return built;
	}

	AudioBackend AudioFactory::defaultBackend()
	{
		return availableBackends().front(); // None is always there, and last
	}

	std::unique_ptr<Audio> AudioFactory::create(AudioBackend backend)
	{
#ifdef XGE_WITH_SFML3
		if (backend == AudioBackend::SFML3) { return std::make_unique<SFMLAudio>(); }
#endif
#ifdef XGE_WITH_RAYLIB
		if (backend == AudioBackend::Raylib) { return std::make_unique<RaylibAudio>(); }
#endif
#ifdef XGE_WITH_SDL2
		if (backend == AudioBackend::SDL2) { return std::make_unique<SDL2Audio>(); }
#endif
		if (backend == AudioBackend::None) { return std::make_unique<NullAudio>(); }

		std::string built;
		for (const AudioBackend each : availableBackends())
		{
			built += (built.empty() ? "" : ", ") + name(each);
		}
		throw std::runtime_error("the " + name(backend) + " sound library is not built into this program (built with: " + built + ")");
	}
}
