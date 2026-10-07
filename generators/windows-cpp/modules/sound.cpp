// sound.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026

#include "sound.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace sound
{
	namespace
	{
		constexpr unsigned sampleRate = 44100;

		// The wave's value, -1 to 1, at `phase` (0 to 1) through one cycle.
		double tone(Wave wave, double phase)
		{
			constexpr double pi = 3.14159265358979323846;

			switch (wave)
			{
			case Wave::Square:   return phase < 0.5 ? 1.0 : -1.0;
			case Wave::Sawtooth: return 2.0 * phase - 1.0;
			case Wave::Sine:     return std::sin(2.0 * pi * phase);
			case Wave::Triangle:
				// 0 up to 1, down to -1, back up to 0.
				if (phase < 0.25) { return 4.0 * phase; }
				if (phase < 0.75) { return 2.0 - 4.0 * phase; }
				return 4.0 * phase - 4.0;
			case Wave::Noise:
				break;
			}
			return 0.0;
		}

		// A small, fixed random number generator (xorshift), so that a noise
		// note sounds the same every time.
		class NoiseSource
		{
		public:
			explicit NoiseSource(std::uint32_t seed) : state(seed ? seed : 0x9e3779b9u) {}

			double next()
			{
				state ^= state << 13;
				state ^= state >> 17;
				state ^= state << 5;
				return static_cast<double>(state) / 2147483648.0 - 1.0;
			}

		private:
			std::uint32_t state;
		};
	}

	float pitch(const std::string& name)
	{
		static constexpr int semitoneOf[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
		if (name.empty()) { return 0.0f; }

		const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
		if (letter < 'A' || letter > 'G') { return 0.0f; }

		int semitone = semitoneOf[letter - 'A'];
		std::size_t at = 1;
		if (at < name.size() && name[at] == '#') { ++semitone; ++at; }
		else if (at < name.size() && name[at] == 'b') { --semitone; ++at; }

		const int octave = std::atoi(name.c_str() + at);
		const int midi = (octave + 1) * 12 + semitone;
		return static_cast<float>(440.0 * std::pow(2.0, (midi - 69) / 12.0));
	}

	Note rest(float seconds)
	{
		return Note{ Wave::Square, 0.0f, 0.0f, seconds };
	}

	std::vector<std::int16_t> synthesize(float volume, const std::vector<Note>& notes)
	{
		std::vector<std::int16_t> samples;
		const double loudness = std::clamp(static_cast<double>(volume), 0.0, 1.0);

		std::uint32_t noteNumber = 0;
		for (const Note& note : notes)
		{
			++noteNumber;

			const std::size_t count = static_cast<std::size_t>(std::max(0.0, static_cast<double>(note.seconds)) * sampleRate);
			if (note.hertz <= 0.0f || count == 0)
			{
				samples.insert(samples.end(), count, 0);
				continue;
			}

			// A couple of milliseconds in and out, so the wave does not start or
			// stop part way up a cycle (a click).
			const std::size_t fade = std::min<std::size_t>(count / 4, static_cast<std::size_t>(sampleRate * 0.002));

			// A slide goes evenly in pitch (the same number of semitones every
			// moment), which is how a pitch bend sounds right.
			const double startHz = static_cast<double>(note.hertz);
			const double ratio = note.slideTo > 0.0f ? static_cast<double>(note.slideTo) / startHz : 1.0;

			NoiseSource noise(0x2545f491u * noteNumber);
			double held = noise.next();

			double phase = 0.0;
			for (std::size_t i = 0; i < count; ++i)
			{
				const double through = static_cast<double>(i) / static_cast<double>(count);
				const double hz = (ratio == 1.0) ? startHz : startHz * std::pow(ratio, through);

				// Noise is a new random level every half cycle, held in between:
				// the pitch sets how coarse the hiss is.
				const double value = note.wave == Wave::Noise ? held : tone(note.wave, phase);

				phase += hz / sampleRate;
				if (note.wave == Wave::Noise && phase >= 0.5)
				{
					held = noise.next();
					phase -= 0.5;
				}
				phase -= std::floor(phase);

				double envelope = 1.0;
				if (fade > 0 && i < fade) { envelope = static_cast<double>(i) / static_cast<double>(fade); }
				else if (fade > 0 && i >= count - fade) { envelope = static_cast<double>(count - 1 - i) / static_cast<double>(fade); }

				samples.push_back(static_cast<std::int16_t>(std::lround(value * envelope * loudness * 32767.0)));
			}
		}

		return samples;
	}

	bool Sound::make(float volume, const std::vector<Note>& notes)
	{
		const std::vector<std::int16_t> samples = synthesize(volume, notes);
		if (samples.empty() || !buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate, { sf::SoundChannel::Mono }))
		{
			return false;
		}

		voice.emplace(buffer);
		return true;
	}

	void Sound::play()
	{
		if (voice) { voice->play(); }
	}
}
