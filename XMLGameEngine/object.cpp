// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "object.h"

namespace xge
{
	void xge::Object::init(const std::string& name, const std::string& src, const std::map<std::string, std::string>& action)
	{
		this->name = name;
		this->src = src;
		this->action = action;
	}
}
