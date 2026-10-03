// audio_sdl2.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio_sdl2.h"

#include <algorithm>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// Only SDL's audio goes: SDL2Window may still be using SDL's video. The
		// last of the two to finish shuts SDL down.
		void quitAudio()
		{
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			if (SDL_WasInit(0) == 0)
			{
				SDL_Quit();
			}
		}
	}

	SDL2Audio::SDL2Audio()
	{
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		{
			throw std::runtime_error(std::string("SDL2 could not start its audio: ") + SDL_GetError());
		}

		// The engine's own format: SDL converts it for the device if the device
		// wants another (no changes are allowed, so `have` is what was asked).
		SDL_AudioSpec want{};
		want.freq = static_cast<int>(kSoundSampleRate);
		want.format = AUDIO_S16SYS;
		want.channels = 1;
		want.samples = 512; // about 12 ms a call: short enough that a bounce is heard on its frame
		want.callback = &SDL2Audio::fill;
		want.userdata = this;

		SDL_AudioSpec have{};
		device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
		if (device == 0)
		{
			const std::string why = SDL_GetError();
			quitAudio();
			throw std::runtime_error("SDL2 could not open a sound device: " + why);
		}

		SDL_PauseAudioDevice(device, 0);
	}

	SDL2Audio::~SDL2Audio()
	{
		SDL_CloseAudioDevice(device);
		quitAudio();
	}

	void SDL2Audio::load(const std::vector<SoundDesc>& sounds)
	{
		std::unordered_map<std::string, Voice> made;
		for (const SoundDesc& sound : sounds)
		{
			Voice voice;
			voice.samples = synthesize(sound);
			voice.position = voice.samples.size();
			made[sound.name] = std::move(voice);
		}

		SDL_LockAudioDevice(device);
		voices = std::move(made);
		SDL_UnlockAudioDevice(device);
	}

	void SDL2Audio::play(const std::string& name)
	{
		SDL_LockAudioDevice(device);
		const auto found = voices.find(name);
		if (found != voices.end())
		{
			found->second.position = 0;
		}
		SDL_UnlockAudioDevice(device);
	}

	void SDL2Audio::stopAll()
	{
		SDL_LockAudioDevice(device);
		for (auto& [name, voice] : voices)
		{
			voice.position = voice.samples.size();
		}
		SDL_UnlockAudioDevice(device);
	}

	// On SDL's audio thread, with the device locked: adds up every voice that
	// is playing into the device's buffer, held to what 16 bits can say.
	void SDLCALL SDL2Audio::fill(void* userdata, Uint8* stream, int length)
	{
		auto* self = static_cast<SDL2Audio*>(userdata);
		auto* out = reinterpret_cast<std::int16_t*>(stream);
		const std::size_t count = static_cast<std::size_t>(length) / sizeof(std::int16_t);

		for (std::size_t i = 0; i < count; ++i)
		{
			std::int32_t mixed = 0;

			for (auto& [name, voice] : self->voices)
			{
				if (voice.position < voice.samples.size())
				{
					mixed += voice.samples[voice.position++];
				}
			}

			out[i] = static_cast<std::int16_t>(std::clamp<std::int32_t>(mixed, -32768, 32767));
		}
	}
}
