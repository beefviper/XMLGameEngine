// audio.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio.h"

#include "audio_sfml.h"
#include "audio_raylib.h"
#include "audio_sdl2.h"

namespace xge
{
	std::unique_ptr<Audio> AudioFactory::create(AudioBackend backend)
	{
		switch (backend)
		{
		case AudioBackend::SFML3:  return std::make_unique<SFMLAudio>();
		case AudioBackend::Raylib: return std::make_unique<RaylibAudio>();
		case AudioBackend::SDL2:   return std::make_unique<SDL2Audio>();
		case AudioBackend::None:   return std::make_unique<NullAudio>();
		}

		return std::make_unique<NullAudio>();
	}
}
