// states.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

namespace xge
{
	std::ostream& operator<<(std::ostream& o, const RawState& f)
	{
		o << "state: ";
		o << "name=" << f.name << ", show=";
		for (auto& show : f.show)
		{
			o << show << (show != f.show.back() ? ", " : "\n");
		}
		for (auto& input : f.input)
		{
			o << "       button=" << input.first << ", action=" << input.second << '\n';
		}

		return o;
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
			o << "       button=" << input.first << ", action=" << input.second.at(0) << '\n';
		}

		return o;
	}
}
