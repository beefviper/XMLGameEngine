// scratch.h
// XML Game Engine
// author: beefviper
// date: Oct 28, 2026
//
// The name of a scratch file or folder a test writes, made this run's own, so
// that test cases run side by side (ctest -j, each case its own run) never
// write over each other's: `name` with a number drawn once per run put in
// before its extension (descriptions/kit_scratch.xml becomes
// descriptions/kit_scratch_48213.xml). The folder it is in stays the same.

#pragma once

#include <filesystem>
#include <random>
#include <string>

inline std::string scratchName(const std::string& name)
{
	static const std::string tag = std::to_string(std::random_device{}() % 1000000u);
	const std::filesystem::path path(name);
	std::filesystem::path made = path.parent_path() / (path.stem().string() + "_" + tag + path.extension().string());
	return made.string();
}
