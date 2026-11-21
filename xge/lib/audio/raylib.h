// raylib.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio/audio.h"

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// raylib's audio device is apart from its window, so this goes with any
	// window. Each sound is one raylib Sound of the engine's samples.
	class RaylibAudio : public Audio
	{
	public:
		// Throws std::runtime_error if raylib cannot open a sound device.
		RaylibAudio();
		~RaylibAudio() override;

		RaylibAudio(const RaylibAudio&) = delete;
		RaylibAudio& operator=(const RaylibAudio&) = delete;

		void load(const std::vector<SoundDesc>& sounds, Resources& resources) override;
		void play(const std::string& name) override;
		void loop(const std::string& name) override;
		void stop(const std::string& name) override;
		void stopAll() override;
		void update() override;

	private:
		std::unordered_map<std::string, Sound> sounds;

		// A raylib Sound plays once; what loops is a Music stream, which raylib
		// is handed more of each frame (update). Each is made from the sound's
		// samples written as a WAV file in memory, which it reads from as it
		// plays, so the bytes are kept beside it.
		struct Loop
		{
			std::vector<unsigned char> wav;
			Music music{};
		};
		std::unordered_map<std::string, Loop> loops;

		void unloadAll();
	};
}
