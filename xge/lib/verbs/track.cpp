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
		if (node.getFirstChild()) { readOnto(node, where + " > <track>", command.onto); }
	}

	static Command makeTrack(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdTrack{ raw.path, raw.onto };
	}

	static void checkTrack(const CmdTrack& track, const Checking& checking)
	{
		if (track.path.empty()) { return; }
		const auto path = checking.paths.find(track.path);
		if (path == checking.paths.end())
		{
			std::vector<std::string> names;
			for (const auto& entry : checking.paths) { names.push_back(entry.first); }
			throw std::runtime_error(checking.where + ": <track path=\"" + track.path + "\" /> names no path of the game" + didYouMean(track.path, names));
		}
		if (!path->second.start)
		{
			throw std::runtime_error(checking.where + ": <track path=\"" + track.path + "\" />: the path needs a <start>, where 0 is");
		}
	}

	static void printTrack(std::ostream& o, const CmdTrack& track)
	{
		o << "track" << (track.path.empty() ? "" : "(" + track.path + ")");
		for (const std::string& onto : track.onto) { o << " onto " << onto; }
	}

	extern const Verb trackVerb;
	const Verb trackVerb{
		.name = "track",
		.forms = { { "<track />", places::controls, kindOf<CmdTrack>() } },
		.read = readTrack,
		.make = makeTrack,
		.check = checkAs<checkTrack>,
		.print = printAs<printTrack>,
	};
}
