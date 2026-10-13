// audio.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "sound.h"

#include <memory>
#include <string>
#include <vector>

namespace xge
{
	// What Engine needs from a sound library: take the game's sounds once, and
	// play one by name when the game asks for it - expressed entirely in
	// engine-level terms (SoundDesc, sound.h), so no audio library's types leak
	// into Engine or Game, the same as Window (window.h) does for drawing.
	// Three libraries implement it today - SFML 3, raylib and SDL2 (see
	// audio_sfml.h, audio_raylib.h, audio_sdl2.h) - and NullAudio below plays
	// nothing. The audio library is chosen separately from the window library:
	// any of them goes with any window, the OpenGL one (which has no sound of
	// its own) included.
	//
	// Every sound is made into samples by the engine (synthesize(), sound.h),
	// the same for every backend; a backend only hands those samples to its
	// library. Each sound has one voice, like a channel of an old sound chip:
	// playing a sound that is still playing starts it again from the
	// beginning, and different sounds play over each other.
	class Audio
	{
	public:
		virtual ~Audio() = default;

		// Called once, right after the audio is created (see Engine's
		// constructors and Engine::replaceAudio): makes every sound the game
		// file describes ready to play. Replaces whatever was loaded before.
		virtual void load(const std::vector<SoundDesc>& sounds) = 0;

		// Starts the sound of that name (again, from the beginning, if it is
		// already playing). A name that was not loaded does nothing: a game
		// file is checked for those when it loads (see game_expr.cpp).
		virtual void play(const std::string& name) = 0;

		// Silences everything that is playing.
		virtual void stopAll() = 0;
	};

	// Plays nothing. What the tests use (they must not need a sound card), what
	// "-a none" asks for, and what Engine falls back to when the library asked
	// for cannot start.
	class NullAudio : public Audio
	{
	public:
		void load(const std::vector<SoundDesc>& sounds) override { (void)sounds; }
		void play(const std::string& name) override { (void)name; }
		void stopAll() override {}
	};

	enum class AudioBackend
	{
		SFML3,
		Raylib,
		SDL2,
		None
	};

	// Builds the concrete Audio for the given backend - the one place that knows
	// about all of them, as WindowFactory (window.h) is for windows. Adding
	// another would only need a branch added here and in AudioBackend.
	//
	// As for windows (window.h), a program is built with some of the sound
	// libraries (the ones whose window backend it has, options.cmake), and
	// available() says which. None is always there.
	class AudioFactory
	{
	public:
		// Throws std::runtime_error if the library cannot start (no sound
		// device, say), or was not built into this program (the message says
		// which are).
		static std::unique_ptr<Audio> create(AudioBackend backend = defaultBackend());

		// Whether this program was built with the backend; None always is.
		static bool available(AudioBackend backend);

		// The backends this program was built with, in AudioBackend's order, None
		// last.
		static std::vector<AudioBackend> availableBackends();

		// SFML 3 when it is built, otherwise the first one that is, or None.
		static AudioBackend defaultBackend();

		// The backend's plain lower case name, as -a takes it: "sfml3", "raylib",
		// "sdl2", "none".
		static std::string name(AudioBackend backend);
	};
}
