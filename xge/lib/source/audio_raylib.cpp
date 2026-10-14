// audio_raylib.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio_raylib.h"

#include <cstring>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// The samples as a WAV file: a RIFF header, then 16-bit mono PCM.
		std::vector<unsigned char> wavOf(const std::vector<std::int16_t>& samples)
		{
			const std::uint32_t dataBytes = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
			std::vector<unsigned char> wav;
			wav.reserve(44 + dataBytes);

			const auto text = [&](const char* four) { wav.insert(wav.end(), four, four + 4); };
			const auto u32 = [&](std::uint32_t value) { for (int i = 0; i < 4; ++i) { wav.push_back(static_cast<unsigned char>(value >> (8 * i))); } };
			const auto u16 = [&](std::uint16_t value) { wav.push_back(static_cast<unsigned char>(value)); wav.push_back(static_cast<unsigned char>(value >> 8)); };

			text("RIFF"); u32(36 + dataBytes); text("WAVE");
			text("fmt "); u32(16); u16(1); u16(1); u32(kSoundSampleRate); u32(kSoundSampleRate * 2); u16(2); u16(16);
			text("data"); u32(dataBytes);

			const std::size_t at = wav.size();
			wav.resize(at + dataBytes);
			std::memcpy(wav.data() + at, samples.data(), dataBytes);
			return wav;
		}
	}

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

		for (auto& [name, loop] : loops)
		{
			StopMusicStream(loop.music);
			UnloadMusicStream(loop.music);
		}
		loops.clear();
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

			if (desc.loop)
			{
				Loop& loop = loops[desc.name];
				loop.wav = wavOf(samples);
				loop.music = LoadMusicStreamFromMemory(".wav", loop.wav.data(), static_cast<int>(loop.wav.size()));
				loop.music.looping = true;
			}
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

	void RaylibAudio::loop(const std::string& name)
	{
		const auto found = loops.find(name);
		if (found == loops.end() || IsMusicStreamPlaying(found->second.music))
		{
			return;
		}

		PlayMusicStream(found->second.music);
	}

	void RaylibAudio::stop(const std::string& name)
	{
		if (const auto sound = sounds.find(name); sound != sounds.end())
		{
			StopSound(sound->second);
		}
		if (const auto loop = loops.find(name); loop != loops.end())
		{
			StopMusicStream(loop->second.music);
		}
	}

	void RaylibAudio::stopAll()
	{
		for (auto& [name, sound] : sounds)
		{
			StopSound(sound);
		}
		for (auto& [name, loop] : loops)
		{
			StopMusicStream(loop.music);
		}
	}

	void RaylibAudio::update()
	{
		for (auto& [name, loop] : loops)
		{
			UpdateMusicStream(loop.music);
		}
	}
}
