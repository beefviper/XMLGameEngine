// spelling.h
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026

#pragma once

#include <string>
#include <vector>

namespace xge
{
	// For a load error about a name the game does not have: " (did you mean
	// 'space'?)" when one of `names` is a likely typo of `name` (a letter or
	// two out, or only the case different), and nothing otherwise.
	std::string didYouMean(const std::string& name, const std::vector<std::string>& names);

	// The names as a list for a message: "a, b, c".
	std::string listOf(const std::vector<std::string>& names);
}
