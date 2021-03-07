// states.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

namespace xge
{
	void State::init(const std::string& name, const std::vector<std::string>& show, const std::map<std::string, std::string>& input)
	{
		this->name = name;
		this->show = show;
		this->input = input;
	}

	std::ostream& operator<<(std::ostream& o, const State& f)
	{
		o << "state: ";
		o << "name=" << f.name << ", show=";
		for (auto& show : f.show)
		{
			o << show << (show != f.show.back() ? ", " : "\n");
		}
		for (auto& input : f.input)
		{
			o << "       action=" << input.first << ", button=" << input.second << '\n';
		}

		return o;
	}
}
