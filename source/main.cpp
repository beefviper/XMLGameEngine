// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "game.h"
#include "engine.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
	const std::string filename = xge::resolveGameFilename(argc, argv);

	// A game file that is wrong - an unknown tag, a missing <radius>, a value
	// that will not evaluate - is reported by the exception loading it throws,
	// with where in the file it was found.
	try
	{
		// Game is fully evaluated the moment its constructor returns - every
		// object's position (grid spacing included), velocity, action, and
		// variable is real - none of it needs a Window/backend to exist first
		// (see game_expr.cpp, command.cpp's measureShapeSize). The exceptions are
		// Object::size of a text or image (the real rendered footprint, used for
		// drawing/collision), which stays {0,0} until Engine's constructor builds
		// a real Window backend to measure it (see Window::init(), window.h), and
		// any <position> that uses the width or height of such an object (for
		// example title.width), which can only be worked out after that. printGame() knows the
		// difference and prints both as "unknown" instead of a misleading {0,0}
		// if called before Engine exists, so it is called twice: once here to
		// show what is known without a window, and once after Engine.
		xge::Game game{ filename };

		std::cout << "=== before Engine: sizes and size-dependent positions not yet known ===\n\n";
		game.printGame();

		xge::Engine engine(game);

		std::cout << "=== after Engine: sizes measured, positions finished ===\n\n";
		game.printGame();

		engine.loop();
	}
	catch (const std::exception& error)
	{
		std::cout << "Error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
