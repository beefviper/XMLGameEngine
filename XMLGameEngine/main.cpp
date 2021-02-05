// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include <iostream>
#include <string>

#include "game.h"
#include "engine.h"

int main(void)
{
	std::string filename{ "games/pong.xml" };
	xge::Game game{ filename };

	game.printGame();
	game.setCurrentState(0);

	xge::Engine engine(game);
	engine.loop();

	return EXIT_SUCCESS;
}
