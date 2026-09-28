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

	// Engine before printGame(): every object's own visual now gets built
	// (and, for a "grid" sprite, its position finalized) by Engine's own
	// constructor once a real Window backend exists - see Window::init() in
	// window.h - so printGame()'s dump of `objects` only shows finalized
	// values if it runs after Engine exists, not before.
	xge::Engine engine(game);

	game.printGame();

	engine.loop();

	return EXIT_SUCCESS;
}
