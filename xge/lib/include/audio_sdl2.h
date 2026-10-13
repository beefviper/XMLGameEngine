// audio_sdl2.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio.h"

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// The SDL2 Audio backend. Nothing outside this file, audio_sdl2.cpp and
	// AudioFactory::create (the one place that constructs one) ever names an
	// SDL audio type - see audio.h. SDL2 itself only opens a sound device and
	// asks for samples when it wants them (no SDL_mixer is used), so this
	// keeps its own small mixer: each sound is one voice, a list of samples
	// and how far through them it is, and the device's callback adds up every
	// voice that is playing. Uses SDL's audio subsystem only, so it lives
	// alongside SDL2Window (window_sdl2.h), which uses the video one.
	class SDL2Audio : public Audio
	{
	public:
		// Throws std::runtime_error if SDL cannot open a sound device.
		SDL2Audio();
		~SDL2Audio() override;

		SDL2Audio(const SDL2Audio&) = delete;
		SDL2Audio& operator=(const SDL2Audio&) = delete;

		void load(const std::vector<SoundDesc>& sounds) override;
		void play(const std::string& name) override;
		void stopAll() override;

	private:
		struct Voice
		{
			std::vector<std::int16_t> samples;

			// The next sample to play; samples.size() while it is silent.
			std::size_t position{ 0 };
		};

		SDL_AudioDeviceID device{ 0 };

		// Read by the callback on SDL's audio thread: only changed with the
		// device locked.
		std::unordered_map<std::string, Voice> voices;

		static void SDLCALL fill(void* userdata, Uint8* stream, int length);
	};
}
