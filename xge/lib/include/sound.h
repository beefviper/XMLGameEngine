// sound.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace xge
{
	// The shapes of wave a note can be played with, the voices of an old 8-bit
	// sound chip: square, triangle and sawtooth are its tone channels, noise is
	// its hiss and crash, and sine is a soft extra no such chip had.
	enum class Waveform
	{
		Square,
		Triangle,
		Sawtooth,
		Sine,
		Noise
	};

	// "square", "triangle", ... - the name a game file uses. nullopt for
	// anything else.
	std::optional<Waveform> waveformFromName(const std::string& name) noexcept;
	std::string waveformName(Waveform wave);

	// A pitch as a game file writes it: a note name with an octave, C4 being
	// middle C and A4 440 Hz ("A4", "C#5", "Eb3"), or a plain number of hertz
	// ("440", "97.5"). nullopt for anything else, or for a pitch that is not
	// above 0 Hz.
	std::optional<float> pitchFromName(const std::string& name) noexcept;

	// One step of a sound: a tone of one wave that slides from startHz to
	// endHz (the same for a steady note) over `seconds`, or, for a rest,
	// silence that long.
	struct SoundNote
	{
		bool rest{ false };
		Waveform wave{ Waveform::Square };
		float startHz{ 0.0f };
		float endHz{ 0.0f };
		float seconds{ 0.0f };

		// Which part (voice) of the sound it is in, from 0. A sound's parts
		// play at the same time, each its notes one after another.
		unsigned part{ 0 };
	};

	// A sound from the game file's <sounds>, worked out: its name (what
	// <play sound="..." /> asks for), whether it loops (<loop />: it plays
	// over and over until it is silenced), how loud (0 to 1) and its notes in
	// the order they play, each in its part (the parts play together, as a
	// sound chip's channels do). Nothing about any audio library is in it; an Audio
	// backend (audio.h) is given these and turns each into samples with
	// synthesize() below.
	struct SoundDesc
	{
		std::string name;
		bool loop{ false };
		float volume{ 0.3f };
		std::vector<SoundNote> notes;
	};

	// The rate every sound is made at and played back at: CD quality, which
	// every audio library takes.
	constexpr unsigned kSoundSampleRate = 44100;

	// The longest a single note or rest may be, so that a typo in a length
	// (60 for 0.60) is a load error instead of a minute of noise in memory.
	constexpr float kMaxNoteSeconds = 10.0f;

	// A sound's samples: signed 16 bit, one channel, kSoundSampleRate a second.
	// The same for every backend, which only has to hand them to its library.
	// The parts are made one by one and added together, as long as the
	// longest; a sum louder than the loudest a sample can be is cut off there.
	// Each note fades in and out over a couple of milliseconds so that it
	// starts and stops without a click; a noise note is the same hiss every
	// time it plays (its random numbers are seeded per note).
	std::vector<std::int16_t> synthesize(const SoundDesc& sound, unsigned sampleRate = kSoundSampleRate);

	std::ostream& operator<<(std::ostream& o, const SoundDesc& sound);
}
