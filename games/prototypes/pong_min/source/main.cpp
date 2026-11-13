// main.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong on SFML: the keys, a frame at 60 a second, the picture.

#include "pong_min.h"

#include "engine/keys.h"
#include "window/sfml.h"
#include "window/window.h"

#include <chrono>
#include <thread>

int main()
{
	pong_min::Pong game;

	xge::SFMLWindow window(game.window);
	window.init(game.objects);

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
		xge::drawFrame(window, game.objects, game.window.background, [&](const xge::Object& object) { return game.inPlay(object); });

		next += period;
		std::this_thread::sleep_until(next);
	}
}
