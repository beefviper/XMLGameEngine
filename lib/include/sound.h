// sound.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "command.h"

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
	};

	// A sound from the game file's <sounds>, worked out: its name (what
	// <play sound="..." /> asks for), how loud (0 to 1) and its notes in the
	// order they play. Nothing about any audio library is in it; an Audio
	// backend (audio.h) is given these and turns each into samples with
	// synthesize() below.
	struct SoundDesc
	{
		std::string name;
		float volume{ 0.3f };
		std::vector<SoundNote> notes;
	};

	// --- What the XML says, before any of it is evaluated (see game_xml.cpp,
	// and game_expr.cpp, which turns these into SoundDescs).

	// One <note> or <rest> as written: <note pitch="C5" to="C6" wave="noise">0.1</note>.
	// wave, pitch and to are left as the words in the file; length is a value
	// (an expression or a value tag), in seconds.
	struct RawNote
	{
		bool rest{ false };
		std::string wave;   // "" means the sound's own
		std::string pitch;
		std::string to;     // "" means no slide
		RawValue length;
	};

	// One <sound name="..." wave="square"> as written; volume is empty text
	// when the sound gives no <volume>.
	struct RawSound
	{
		std::string name;
		std::string wave;   // "" means square
		RawValue volume;
		std::vector<RawNote> notes;
	};

	// The rate every sound is made at and played back at: CD quality, which
	// every audio library takes.
	constexpr unsigned kSoundSampleRate = 44100;

	// The longest a single note or rest may be, so that a typo in a length
	// (60 for 0.60) is a load error instead of a minute of noise in memory.
	constexpr float kMaxNoteSeconds = 10.0f;

	// A sound's samples: signed 16 bit, one channel, kSoundSampleRate a second.
	// The same for every backend, which only has to hand them to its library.
	// Each note fades in and out over a couple of milliseconds so that it
	// starts and stops without a click; a noise note is the same hiss every
	// time it plays (its random numbers are seeded per note).
	std::vector<std::int16_t> synthesize(const SoundDesc& sound, unsigned sampleRate = kSoundSampleRate);

	std::ostream& operator<<(std::ostream& o, const SoundDesc& sound);
}
