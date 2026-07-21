// game.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "core/game/runtime_object.h"
#include "core/game/runtime_states.h"

#include <string>
#include <map>
#include <vector>

namespace xge
{

class Game
{
public:
	Game(const std::string& game);

	void parse();
	void validate();
	void evaluate();
	void generate();

private:
	std::string game;

	std::map<std::string, float> variables;
	std::vector<State> states;
	std::vector<Object> objects;


};

} // namespace xge
