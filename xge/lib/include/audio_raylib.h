// audio_raylib.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio.h"

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// The raylib Audio backend. Nothing outside this file, audio_raylib.cpp and
	// AudioFactory::create (the one place that constructs one) ever names a
	// raylib audio type - see audio.h. raylib's audio device is its own,
	// separate from its window (InitAudioDevice), so this works with any
	// window backend, raylib's included. Each sound is one raylib Sound made
	// from the engine's samples.
	class RaylibAudio : public Audio
	{
	public:
		// Throws std::runtime_error if raylib cannot open a sound device.
		RaylibAudio();
		~RaylibAudio() override;

		RaylibAudio(const RaylibAudio&) = delete;
		RaylibAudio& operator=(const RaylibAudio&) = delete;

		void load(const std::vector<SoundDesc>& sounds) override;
		void play(const std::string& name) override;
		void stopAll() override;

	private:
		std::unordered_map<std::string, Sound> sounds;

		void unloadAll();
	};
}
