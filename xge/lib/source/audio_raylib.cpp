// audio_raylib.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio_raylib.h"

#include <stdexcept>

namespace xge
{
	RaylibAudio::RaylibAudio()
	{
		SetTraceLogLevel(LOG_WARNING);
		InitAudioDevice();

		if (!IsAudioDeviceReady())
		{
			CloseAudioDevice();
			throw std::runtime_error("raylib could not open a sound device");
		}
	}

	RaylibAudio::~RaylibAudio()
	{
		unloadAll();
		CloseAudioDevice();
	}

	void RaylibAudio::unloadAll()
	{
		for (auto& [name, sound] : sounds)
		{
			StopSound(sound);
			UnloadSound(sound);
		}
		sounds.clear();
	}

	void RaylibAudio::load(const std::vector<SoundDesc>& descs)
	{
		unloadAll();

		for (const SoundDesc& desc : descs)
		{
			std::vector<std::int16_t> samples = synthesize(desc);
			if (samples.empty())
			{
				continue;
			}

			// raylib copies the samples into a sound of its own, so the Wave is
			// only borrowed for the call.
			Wave wave{};
			wave.frameCount = static_cast<unsigned int>(samples.size());
			wave.sampleRate = kSoundSampleRate;
			wave.sampleSize = 16;
			wave.channels = 1;
			wave.data = samples.data();

			sounds[desc.name] = LoadSoundFromWave(wave);
		}
	}

	void RaylibAudio::play(const std::string& name)
	{
		const auto found = sounds.find(name);
		if (found == sounds.end())
		{
			return;
		}

		StopSound(found->second);
		PlaySound(found->second);
	}

	void RaylibAudio::stopAll()
	{
		for (auto& [name, sound] : sounds)
		{
			StopSound(sound);
		}
	}
}
