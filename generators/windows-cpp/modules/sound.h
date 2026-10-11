// sound.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// The sound a generated game uses: 8-bit sounds written as notes (a wave, a
// pitch, a slide, a length) and made into samples when the game starts. The
// samples are made by synth.h, the engine's own synthesizer, copied in as it is,
// so the game sounds as the engine does; all that is here is handing them to
// SFML. Nothing but SFML's audio types and synth.h.
//
//   sound::Sound blip;
//   blip.make("blip", 0.3f, { { sound::Wave::Square, sound::pitch("A4"), 0.0f, 0.05f } });
//   blip.play();

#pragma once

#include "synth.h"

#include <SFML/Audio.hpp>

#include <optional>
#include <string>
#include <vector>

namespace sound
{
	using Wave = xge::Waveform;

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

	// A sound ready to play.
	class Sound
	{
	public:
		Sound() = default;
		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		// Makes the samples; false if there was nothing to make or SFML could
		// not take them.
		// `name` is what the game calls it: the frame trace lists the sounds heard by it.
		bool make(const std::string& name, float volume, const std::vector<Note>& notes);

		// Plays it from the start (again, if it is already playing).
		void play();

	private:
		std::string name;
		sf::SoundBuffer buffer;
		std::optional<sf::Sound> voice;
	};
}
