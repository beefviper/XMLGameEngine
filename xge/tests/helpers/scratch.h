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
// Every scratch name holds "_scratch" (added when missing), which is how a
// test listing the games (played::shippedGames) leaves out files another
// case is writing and deleting at the same moment.

#pragma once

#include <filesystem>
#include <random>
#include <string>

inline std::string scratchName(const std::string& name)
{
	static const std::string tag = std::to_string(std::random_device{}() % 1000000u);
	const std::filesystem::path path(name);
	std::string stem = path.stem().string();
	if (stem.find("_scratch") == std::string::npos) { stem += "_scratch"; }
	std::filesystem::path made = path.parent_path() / (stem + "_" + tag + path.extension().string());
	return made.string();
}
