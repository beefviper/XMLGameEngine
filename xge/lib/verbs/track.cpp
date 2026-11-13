// track.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// <track>: Follows a control that points (a mouse, a stick): run by
	// CommandExecutor::executeAxis.
	static void readTrack(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		if (node.getFirstChild()) { readOnto(node, where + " > <track>", command); }
	}

	static Command makeTrack(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdTrack{ raw.path, raw.onto };
	}

	static void checkTrack(const Command& command, const Checking& checking)
	{
		const CmdTrack& track = std::get<CmdTrack>(command);
		if (track.path.empty()) { return; }
		const auto path = checking.paths.find(track.path);
		if (path == checking.paths.end())
		{
			std::vector<std::string> names;
			for (const auto& entry : checking.paths) { names.push_back(entry.first); }
			throw std::runtime_error(checking.where + ": <track path=\"" + track.path + "\" /> names no path of the game" + didYouMean(track.path, names));
		}
		if (!path->second.hasStart)
		{
			throw std::runtime_error(checking.where + ": <track path=\"" + track.path + "\" />: the path needs a <start>, where 0 is");
		}
	}

	static void printTrack(std::ostream& o, const Command& command)
	{
		const CmdTrack& t = std::get<CmdTrack>(command);
		o << "track" << (t.path.empty() ? "" : "(" + t.path + ")");
		for (const std::string& onto : t.onto) { o << " onto " << onto; }
	}

	static void printTrackRaw(std::ostream& o, const RawCommand& command)
	{
		if (!command.path.empty()) { o << "(" << command.path << ")"; }
	}

	extern const Verb trackVerb;
	const Verb trackVerb{
		.name = "track",
		.forms = { { "<track />", places::controls, kindOf<CmdTrack>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readTrack,
		.make = makeTrack,
		.check = checkTrack,
		.run = nullptr,
		.print = printTrack,
		.printRaw = printTrackRaw,
	};
}
