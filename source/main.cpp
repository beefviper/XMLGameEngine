// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"
#include "engine.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <span>

int main(int argc, char* argv[])
{
	
	// TODO: make function to validate filename
	std::string filename{ "games/pong_full.xml" };

	const std::span<char*> args(argv, argc);
	
	if (args.size() > 1)
	{
		filename = args[1];
	}

	// TODO: why nested if-else? just use if, else if, else
	if (!std::filesystem::exists(filename))
	{
		if (std::filesystem::exists("games/" + filename))
		{
			filename = "games/" + filename;
		}
		else
		{
			std::cout << "File not found: " << filename << '\n';
			exit(EXIT_FAILURE);
		}
	}

	xge::Game game{ filename };

	game.printGame();

	xge::Engine engine(game);
	engine.loop();

	return EXIT_SUCCESS;
}
