// main.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Pong on SFML: the keys, a frame at 60 a second, the texts measured and
// placed, the sounds, the picture.

#include "pong.h"

#include "audio/sfml.h"
#include "engine/keys.h"
#include "window/sfml.h"
#include "window/window.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>

int main()
{
	pong::Pong game;
	game.keepIn("pong-kept.txt");

	xge::SFMLWindow window(game.window);
	window.init(game.objects);
	game.place();

	// No sound device is no reason not to play.
	std::unique_ptr<xge::Audio> audio;
	try
	{
		audio = std::make_unique<xge::SFMLAudio>();
		audio->load(game.sounds);
	}
	catch (const std::exception& error)
	{
		std::cerr << "warning: no sound: " << error.what() << '\n';
	}

	using Clock = std::chrono::steady_clock;
	const auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / game.window.framerate));
	auto next = Clock::now();

	xge::HeldKeys keys;
	while (window.isOpen())
	{
		for (const auto& [key, pressed] : window.pollEvents())
		{
			if (!pressed) { keys.release(key); game.key(key, false); }
			else if (keys.press(key)) { game.key(key, true); }
		}

		game.frame();

		// a score that changed is measured again, and put back against the net
		for (xge::Object& object : game.objects) { window.prepare(object); }
		game.place();

		for (const std::string& sound : game.takeSounds())
		{
			if (audio) { audio->play(sound); }
		}
		if (audio) { audio->update(); }

		xge::drawFrame(window, game.objects, game.window.background,
			[&](const xge::Object& object) { return game.still(object); },
			[&](const xge::Object& object) { return game.inPlay(object); });

		next += period;
		std::this_thread::sleep_until(next);
	}
}
