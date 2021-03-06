// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <string>
#include <vector>
#include <map>
#include <ostream>

namespace xge
{
	class State
	{
	public:
		void init(const std::string& name, const std::vector<std::string>& show, const std::map<std::string, std::string> &input);

		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::string> input;

		friend std::ostream& operator<<(std::ostream& o, State const& f);

	private:

	};
}
