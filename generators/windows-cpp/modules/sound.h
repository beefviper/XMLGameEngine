// sound.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// The sound a generated game uses, in one place of its own: 8-bit sounds
// written as notes (a wave, a pitch, a slide, a length) and made into samples
// when the game starts, as the engine's synthesizer makes them. Nothing but
// SFML's audio types, so it can be copied into any SFML 3 program.
//
//   sound::Sound blip;
//   blip.make(0.3f, { { sound::Wave::Square, sound::pitch("A4"), 0.0f, 0.05f } });
//   blip.play();

#pragma once

#include <SFML/Audio.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sound
{
	enum class Wave { Square, Triangle, Sawtooth, Sine, Noise };

	// One note: its wave, its pitch in hertz (0 is a rest), the pitch it
	// slides to by the end (0: none), and how many seconds it lasts.
	struct Note
	{
		Wave wave{ Wave::Square };
		float hertz{};
		float slideTo{};
		float seconds{};
	};

	// A pitch by name, as on a piano: a letter, a sharp (#) or flat (b), and an
	// octave (A4 is 440 hertz, C4 middle C). 0 for a name it cannot read.
	float pitch(const std::string& name);

	// A silence that long.
	Note rest(float seconds);

	// The samples of the notes one after another, at `volume` (0 to 1).
	std::vector<std::int16_t> synthesize(float volume, const std::vector<Note>& notes);

	// A sound ready to play.
	class Sound
	{
	public:
		Sound() = default;
		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		// Makes the samples; false if there was nothing to make or SFML could
		// not take them.
		bool make(float volume, const std::vector<Note>& notes);

		// Plays it from the start (again, if it is already playing).
		void play();

	private:
		sf::SoundBuffer buffer;
		std::optional<sf::Sound> voice;
	};
}
