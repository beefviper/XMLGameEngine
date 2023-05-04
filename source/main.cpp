// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"
#include "engine.h"

#include <iostream>
#include <string>
#include <filesystem>

int main(int argc, char* argv[])
{
	
	// TODO: make function to validate filename
	std::string filename{ "games/pong_full.xml" };

	if (argc == 2)
	{
		filename = argv[1];
	}

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
