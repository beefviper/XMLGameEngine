// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

void xge::State::init(std::string name, std::vector<std::string> show, std::map<std::string, std::string> input)
{
	this->name = name;
	this->show = show;
	this->input = input;
}
