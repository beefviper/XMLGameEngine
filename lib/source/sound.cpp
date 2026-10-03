// sound.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#include "sound.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace xge
{
	std::optional<Waveform> waveformFromName(const std::string& name) noexcept
	{
		if (name == "square")   { return Waveform::Square; }
		if (name == "triangle") { return Waveform::Triangle; }
		if (name == "sawtooth") { return Waveform::Sawtooth; }
		if (name == "sine")     { return Waveform::Sine; }
		if (name == "noise")    { return Waveform::Noise; }
		return std::nullopt;
	}

	std::string waveformName(Waveform wave)
	{
		switch (wave)
		{
		case Waveform::Square:   return "square";
		case Waveform::Triangle: return "triangle";
		case Waveform::Sawtooth: return "sawtooth";
		case Waveform::Sine:     return "sine";
		case Waveform::Noise:    return "noise";
		}
		return "square";
	}

	std::optional<float> pitchFromName(const std::string& name) noexcept
	{
		if (name.empty())
		{
			return std::nullopt;
		}

		// A plain number of hertz.
		if (std::isdigit(static_cast<unsigned char>(name[0])) || name[0] == '.')
		{
			char* end = nullptr;
			const float hz = std::strtof(name.c_str(), &end);
			if (end != name.c_str() + name.size() || !(hz > 0.0f) || !std::isfinite(hz))
			{
				return std::nullopt;
			}
			return hz;
		}

		// A note: a letter, an optional sharp or flat, and an octave. C4 is
		// middle C (MIDI note 60) and A4 is 440 Hz, as on a piano.
		static constexpr int semitoneOf[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
		const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
		if (letter < 'A' || letter > 'G')
		{
			return std::nullopt;
		}

		int semitone = semitoneOf[letter - 'A'];
		std::size_t at = 1;
		if (at < name.size() && name[at] == '#') { ++semitone; ++at; }
		else if (at < name.size() && name[at] == 'b') { --semitone; ++at; }

		// The octave: a whole number, which may be negative (C-1 is MIDI 0).
		const std::string octaveText = name.substr(at);
		if (octaveText.empty() || octaveText.size() > 3)
		{
			return std::nullopt;
		}
		for (std::size_t i = 0; i < octaveText.size(); ++i)
		{
			const bool sign = (i == 0 && octaveText[i] == '-' && octaveText.size() > 1);
			if (!sign && !std::isdigit(static_cast<unsigned char>(octaveText[i])))
			{
				return std::nullopt;
			}
		}

		const int octave = std::atoi(octaveText.c_str());
		const int midi = (octave + 1) * 12 + semitone;
		return static_cast<float>(440.0 * std::pow(2.0, (midi - 69) / 12.0));
	}

	namespace
	{
		// The wave's value, -1 to 1, at `phase` (0 to 1) through one cycle.
		double tone(Waveform wave, double phase)
		{
			constexpr double pi = 3.14159265358979323846;

			switch (wave)
			{
			case Waveform::Square:   return phase < 0.5 ? 1.0 : -1.0;
			case Waveform::Sawtooth: return 2.0 * phase - 1.0;
			case Waveform::Sine:     return std::sin(2.0 * pi * phase);
			case Waveform::Triangle:
				// 0 up to 1, down to -1, back up to 0.
				if (phase < 0.25) { return 4.0 * phase; }
				if (phase < 0.75) { return 2.0 - 4.0 * phase; }
				return 4.0 * phase - 4.0;
			case Waveform::Noise:
				break;
			}
			return 0.0;
		}

		// A small, fixed random number generator (xorshift), so that a noise
		// note sounds the same every time and in every backend.
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

	std::vector<std::int16_t> synthesize(const SoundDesc& sound, unsigned sampleRate)
	{
		std::vector<std::int16_t> samples;
		const double volume = std::clamp(static_cast<double>(sound.volume), 0.0, 1.0);

		std::uint32_t noteNumber = 0;
		for (const SoundNote& note : sound.notes)
		{
			++noteNumber;

			const std::size_t count = static_cast<std::size_t>(std::max(0.0, static_cast<double>(note.seconds)) * sampleRate);

			if (note.rest || count == 0)
			{
				samples.insert(samples.end(), count, 0);
				continue;
			}

			// A couple of milliseconds in and out, so the wave does not start or
			// stop part way up a cycle (a click), but not so long that a short
			// blip loses its edge.
			const std::size_t fade = std::min<std::size_t>(count / 4, static_cast<std::size_t>(sampleRate * 0.002));

			// A slide goes up or down evenly in pitch (by the same number of
			// semitones every moment), which is how a pitch bend sounds right,
			// rather than evenly in hertz.
			const double startHz = note.startHz;
			const double ratio = (note.startHz > 0.0f && note.endHz > 0.0f) ? static_cast<double>(note.endHz) / note.startHz : 1.0;

			NoiseSource noise(0x2545f491u * noteNumber);
			double held = noise.next();

			// The phase is added up a sample at a time rather than worked out
			// from the time, so a slide stays smooth as the pitch changes.
			double phase = 0.0;
			for (std::size_t i = 0; i < count; ++i)
			{
				const double through = static_cast<double>(i) / static_cast<double>(count);
				const double hz = (ratio == 1.0) ? startHz : startHz * std::pow(ratio, through);

				double value = 0.0;
				if (note.wave == Waveform::Noise)
				{
					// A new random level every half cycle, held in between: the
					// pitch sets how coarse the hiss is, as on a sound chip's noise
					// channel (a high pitch is a hiss, a low one a rumble).
					value = held;
				}
				else
				{
					value = tone(note.wave, phase);
				}

				phase += hz / sampleRate;
				if (note.wave == Waveform::Noise && phase >= 0.5)
				{
					held = noise.next();
					phase -= 0.5;
				}
				phase -= std::floor(phase);

				double envelope = 1.0;
				if (fade > 0 && i < fade) { envelope = static_cast<double>(i) / static_cast<double>(fade); }
				else if (fade > 0 && i >= count - fade) { envelope = static_cast<double>(count - 1 - i) / static_cast<double>(fade); }

				samples.push_back(static_cast<std::int16_t>(std::lround(value * envelope * volume * 32767.0)));
			}
		}

		return samples;
	}

	std::ostream& operator<<(std::ostream& o, const SoundDesc& sound)
	{
		o << "sound: name=" << sound.name << ", volume=" << sound.volume << ", notes=";

		for (std::size_t i = 0; i < sound.notes.size(); ++i)
		{
			const SoundNote& note = sound.notes[i];
			o << (i ? ";" : "");

			if (note.rest)
			{
				o << "rest(" << note.seconds << ")";
				continue;
			}

			o << waveformName(note.wave) << "(" << note.startHz;
			if (note.endHz != note.startHz) { o << "->" << note.endHz; }
			o << "Hz, " << note.seconds << ")";
		}

		return o;
	}
}
