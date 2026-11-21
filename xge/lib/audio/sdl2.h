// sdl2.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio/audio.h"

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// SDL2 only opens a device and asks for samples (no SDL_mixer), so this
	// mixes itself: each sound is a voice, its samples and how far through,
	// and the device's callback adds up the voices playing.
	class SDL2Audio : public Audio
	{
	public:
		// Throws std::runtime_error if SDL cannot open a sound device.
		SDL2Audio();
		~SDL2Audio() override;

		SDL2Audio(const SDL2Audio&) = delete;
		SDL2Audio& operator=(const SDL2Audio&) = delete;

		void load(const std::vector<SoundDesc>& sounds, Resources& resources) override;
		void play(const std::string& name) override;
		void loop(const std::string& name) override;
		void stop(const std::string& name) override;
		void stopAll() override;

	private:
		struct Voice
		{
			std::shared_ptr<const std::vector<std::int16_t>> samples;   // the game's, shared

			// The next sample to play; samples.size() while it is silent.
			std::size_t position{ 0 };

			// Back to the start at the end, instead of falling silent.
			bool looping{ false };
		};

		SDL_AudioDeviceID device{ 0 };

		// Read by the callback on SDL's audio thread: only changed with the
		// device locked.
		std::unordered_map<std::string, Voice> voices;

		static void SDLCALL fill(void* userdata, Uint8* stream, int length);
	};
}
