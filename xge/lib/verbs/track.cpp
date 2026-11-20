// track.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// <track>: Follows a control that points (a mouse, a stick).
	static void readTrack(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		if (node.getFirstChild()) { readOnto(node, where + " > <track>", command.own.emplace<WrittenTrack>().onto); }
	}

	static Command makeTrack(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdTrack{ written.path, NameStore::program().of(ownOf<WrittenTrack>(written).onto) };
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

	// <track />: the object's middle goes to `fraction` (0 to 1) of the way
	// across `span` (the window's width, or its height when not
	// `horizontal`), kept wholly inside it. Its speed is left alone.
	static void trackWindow(Object& object, bool horizontal, float fraction, float span)
	{
		const Vector2f size = sizeOf(object);
		const float extent = horizontal ? size.x : size.y;
		const float room = std::max(span - extent, 0.0f);
		const float place = std::clamp(fraction * span - extent * 0.5f, 0.0f, room);
		(horizontal ? object.position.x : object.position.y) = place;
	}

	// <track path="...">: the object's middle `fraction` (0 to 1, held to
	// that) of the way along the path, measured along its legs from its
	// <start>; a <home /> leg goes back to the start.
	static void trackPath(Object& object, const Path& path, float fraction)
	{
		const Vector2f start = path.start.value_or(Vector2f{}); // a tracked path has one (verbs/track.cpp)
		std::vector<Vector2f> corners{ start };
		for (const PathStep& step : path.steps)
		{
			corners.push_back(step.home ? start : corners.back() + step.by);
		}

		float total = 0.0f;
		for (std::size_t i = 1; i < corners.size(); ++i) { total += lengthOf(corners[i] - corners[i - 1]); }

		Vector2f at = corners.front();
		float left = std::clamp(fraction, 0.0f, 1.0f) * total;
		for (std::size_t i = 1; i < corners.size(); ++i)
		{
			const Vector2f leg = corners[i] - corners[i - 1];
			const float length = lengthOf(leg);
			if (left <= length && length > 0.0f)
			{
				at = corners[i - 1] + leg * (left / length);
				break;
			}
			left -= length;
			at = corners[i];
		}

		object.position = at - sizeOf(object) * 0.5f;
	}

	static void pointTrack(const CmdTrack& track, const Doer& doer, bool across, float fraction)
	{
		Game& game = doer.game();
		Object& object = *doer.self;
		if (!track.path.empty())
		{
			if (const auto path = game.getPaths().find(track.path); path != game.getPaths().end()) { trackPath(object, path->second, fraction); }
			return;
		}
		const WindowDesc& window = game.getWindowDesc();
		const float span = across ? window.width : window.height;
		if (game.isHand(object)) { game.trackHand(object, across, fraction * span); }
		else if (object.onCells) { game.cellMaps().trackCells(object, across, fraction * span, track.onto); }
		else { trackWindow(object, across, fraction, span); }
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
		.point = pointAs<pointTrack>,
		.print = printAs<printTrack>,
	};
}
