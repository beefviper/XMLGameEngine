// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

void xge::State::init(const std::string& name, const std::vector<std::string>& show, const std::map<std::string, std::string> &input)
{
	this->name = name;
	this->show = show;
	this->input = input;
}
