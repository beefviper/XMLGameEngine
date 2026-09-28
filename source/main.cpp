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

	// Game is fully evaluated the moment its constructor returns - every
	// object's position (grid() spacing included), velocity, action, and
	// variable is real - none of it needs a Window/backend to exist first
	// (see game_expr.cpp, command.cpp's measureShapeSize). The one exception
	// is Object::size (the real rendered footprint, used for drawing/
	// collision), which stays {0,0} until Engine's constructor builds a real
	// Window backend to measure it (see Window::init(), window.h) -
	// printGame() knows the difference and prints size as "unknown" instead
	// of a misleading {0,0} if called before Engine exists. For now,
	// printGame() is called after Engine so its dump includes everything.
	xge::Game game{ filename };

	xge::Engine engine(game);

	game.printGame();

	engine.loop();

	return EXIT_SUCCESS;
}
