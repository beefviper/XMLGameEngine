// main.cpp
// XML Game Engine
// author: beefviper
// date: July 20, 2026

#include "core/game.h"
#include "core/engine.h"

#include <iostream>
#include <string>
#include <filesystem>

int main()
{
	std::string xmlFile = "games/pong.xml";

	xge::Game game(xmlFile);

	xge::Engine engine(game);
	engine.run();
}
