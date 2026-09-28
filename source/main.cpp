// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "game.h"
#include "engine.h"

#include <cstdlib>
#include <string>

int main(int argc, char* argv[])
{
	const std::string filename = xge::resolveGameFilename(argc, argv);

	xge::Game game{ filename };

	game.printGame();

	xge::Engine engine(game);
	engine.loop();

	return EXIT_SUCCESS;
}
