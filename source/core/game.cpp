// game.cpp
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#include "core/game.h"
#include "xml/xml_interface.h"

namespace xge
{

Game::Game(const std::string& game) :
	game(game)
{
	parse();
	validate();
	evaluate();
	generate();
}

void Game::parse()
{

}

void Game::validate()
{

}

void Game::evaluate()
{

}

void Game::generate()
{

}

} // namespace xge
