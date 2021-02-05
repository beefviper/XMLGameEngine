// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "object.h"

void xge::Object::init(std::string name, std::string src, std::map<std::string, std::string> action)
{
	this->name = name;
	this->src = src;
	this->action = action;
}
