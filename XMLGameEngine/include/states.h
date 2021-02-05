// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <string>
#include <vector>
#include <map>

namespace xge
{
	class State
	{
	public:
		void init(std::string name, std::vector<std::string> show, std::map<std::string, std::string> input);

		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::string> input;

	private:

	};
}
