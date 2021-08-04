// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"
#include "engine.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	std::string filename{ "games/breakout.xml" };

	if (argc == 2)
	{
		filename = argv[1];
	}

	xge::Game game{ filename };
	
	game.printGame();
	game.setCurrentState(0);

	xge::Engine engine(game);
	engine.loop();

	return EXIT_SUCCESS;
}
