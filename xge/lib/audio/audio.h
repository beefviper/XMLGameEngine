// audio.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

#include "audio/sound.h"
#include "engine/resources.h"

#include <memory>
#include <string>
#include <vector>

namespace xge
{
	// A sound library, in the engine's terms only; any goes with any window.
	// The engine makes every sound's samples (samplesOf); a backend only hands
	// them on. Each sound has one voice, like a sound chip's channel: played
	// again, it starts over; different sounds play over each other.
	class Audio
	{
	public:
		virtual ~Audio() = default;

		// Every sound of the game made ready, in place of any before.
		virtual void load(const std::vector<SoundDesc>& sounds, Resources& resources) = 0;

		// From the start, even if it is playing; an unknown name does nothing.
		virtual void play(const std::string& name) = 0;

		// Over and over until stopped; one already looping carries on.
		virtual void loop(const std::string& name) = 0;

		virtual void stop(const std::string& name) = 0;

		virtual void stopAll() = 0;

		// Once a frame, for a library handed a loop as it plays (raylib).
		virtual void update() {}
	};

	// Plays nothing: for the tests, "-a none", and when a library cannot start.
	class NullAudio : public Audio
	{
	public:
		void load(const std::vector<SoundDesc>& sounds, Resources& resources) override { (void)sounds; (void)resources; }
		void play(const std::string& name) override { (void)name; }
		void loop(const std::string& name) override { (void)name; }
		void stop(const std::string& name) override { (void)name; }
		void stopAll() override {}
	};

	enum class AudioBackend
	{
		SFML3,
		Raylib,
		SDL2,
		None
	};

	// The one place that knows every backend. A program has the sound
	// libraries of the window backends it is built with; None always.
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
