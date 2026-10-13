// audio_sfml.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio.h"

#include <SFML/Audio.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// The SFML 3 Audio backend. Nothing outside this file, audio_sfml.cpp and
	// AudioFactory::create (the one place that constructs one) ever names an
	// SFML audio type - see audio.h. Each sound is an sf::SoundBuffer of the
	// engine's samples and one sf::Sound playing it, its one voice.
	class SFMLAudio : public Audio
	{
	public:
		void load(const std::vector<SoundDesc>& sounds) override;
		void play(const std::string& name) override;
		void stopAll() override;

	private:
		// An sf::Sound refers to its buffer for its whole life, so the two are
		// kept together, and the pair is never moved once made.
		struct Voice
		{
			sf::SoundBuffer buffer;
			std::unique_ptr<sf::Sound> sound;
		};

		std::unordered_map<std::string, std::unique_ptr<Voice>> voices;
	};
}
