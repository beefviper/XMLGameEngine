// audio_sfml.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "audio_sfml.h"

#include <iostream>

namespace xge
{
	void SFMLAudio::load(const std::vector<SoundDesc>& sounds)
	{
		stopAll();
		voices.clear();

		for (const SoundDesc& sound : sounds)
		{
			const std::vector<std::int16_t> samples = synthesize(sound);
			if (samples.empty())
			{
				continue;
			}

			auto voice = std::make_unique<Voice>();
			if (!voice->buffer.loadFromSamples(samples.data(), samples.size(), 1, kSoundSampleRate, { sf::SoundChannel::Mono }))
			{
				std::cerr << "warning: SFML could not load the sound '" << sound.name << "'\n";
				continue;
			}

			voice->sound = std::make_unique<sf::Sound>(voice->buffer);
			voices[sound.name] = std::move(voice);
		}
	}

	void SFMLAudio::play(const std::string& name)
	{
		const auto found = voices.find(name);
		if (found == voices.end())
		{
			return;
		}

		// play() on a sound that is playing starts it again from the beginning.
		found->second->sound->stop();
		found->second->sound->play();
	}

	void SFMLAudio::stopAll()
	{
		for (auto& [name, voice] : voices)
		{
			voice->sound->stop();
		}
	}
}
