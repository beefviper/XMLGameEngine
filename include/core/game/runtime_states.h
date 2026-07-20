// model_states.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include <string>
#include <vector>
#include <map>

namespace xge
{
	struct State
	{
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::string> input;
	};
}
