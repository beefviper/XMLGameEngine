// backends.cpp
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

// Engine's constructor that picks its window and sound libraries by name,
// through the factories (window/window.cpp, audio/audio.cpp). Kept apart from engine/engine.cpp
// so a program built for one library, a generated game on SFML, makes its
// window and audio itself and leaves the factories out.

#include "engine/engine.h"

#include <exception>
#include <iostream>

namespace xge
{
	namespace
	{
		// The audio library asked for, or silence if it will not start (no
		// sound device, say): a game is still worth playing without sound.
		std::unique_ptr<Audio> makeAudioOrSilence(AudioBackend backend)
		{
			try
			{
				return AudioFactory::create(backend);
			}
			catch (const std::exception& error)
			{
				std::cerr << "warning: no sound: " << error.what() << '\n';
				return std::make_unique<NullAudio>();
			}
		}
	}

	Engine::Engine(Game& gameToRun, WindowBackend backend, AudioBackend audioBackend) :
		Engine(gameToRun, WindowFactory::create(gameToRun.getWindowDesc(), backend), makeAudioOrSilence(audioBackend))
	{
	}
}
