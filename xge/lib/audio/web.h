// web.h
// XML Game Engine
// author: beefviper
// date: Nov 23, 2026

#pragma once

#include "audio/audio.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// The page's Web Audio, for a game built with Emscripten: each sound's
	// samples a buffer of the page's, played by a voice of its own. A
	// browser lets a page make sound only once someone has pressed a key or
	// clicked on it, so the first press starts it.
	class WebAudio : public Audio
	{
	public:
		WebAudio();

		void load(const std::vector<SoundDesc>& sounds, Resources& resources) override;
		void play(const std::string& name) override;
		void loop(const std::string& name) override;
		void stop(const std::string& name) override;
		void stopAll() override;

	private:
		// Each sound's number on the page, by its name.
		std::unordered_map<std::string, int> sounds;
	};
}
