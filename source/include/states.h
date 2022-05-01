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
	class RawState
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::string> input;

		friend std::ostream& operator<<(std::ostream& o, RawState const& f);

	private:

	};

	class State
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::vector<std::string>> input;

		friend std::ostream& operator<<(std::ostream& o, State const& f);

	private:

	};
}
