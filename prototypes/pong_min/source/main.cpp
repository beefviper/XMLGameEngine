// main.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong on SFML: the loop every game runs, the keys, a frame, the picture.

#include "pong_min.h"

#include "keycode.h"
#include "window.h"
#include "window_sfml.h"

int main()
{
	pong_min::Pong game;

	xge::SFMLWindow window(game.window);
	window.init(game.objects);

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
	}
}
