// think.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "engine/keys.h"
#include "load/reading.h"

#include <cmath>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

namespace xge
{
	// <think>: A <computer> looks ahead for its best move: run by Game
	// (game/think.cpp).
	static void readThink(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		const std::string here = where + " > <think>";
		RawThink& think = command.own.emplace<RawThink>();
		think.button = requireAttribute(node, "button", here);
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag == "depth") { think.depth = readValue(*child, here + " > <depth>"); continue; }
			if (tag == "avoid") { think.avoid = true; continue; }
			if (tag != "worth") { fail(here, "unknown <" + tag + ">; expected <depth>, <avoid /> and <worth>s"); }
			const std::string look = requireAttribute(*child, "sprite", here + " > <worth>");
			const std::string text = readText(*child);
			char* end = nullptr;
			const float worth = std::strtof(text.c_str(), &end);
			if (text.empty() || end != text.c_str() + text.size()) { fail(here + " > <worth sprite=\"" + look + "\">", "is \"" + text + "\"; expected a number"); }
			think.worths.emplace_back(look, worth);
		}
	}

	static Command makeThink(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		if (raw.object.empty()) { throw std::runtime_error("<think>: names the object it plays with object= (a <think> on a member of a group that takes <turns> plays that member)"); }
		const RawThink& written = ownOf<RawThink>(raw);
		const KeyCode button = keyCodeFromString(written.button);
		if (button == KeyCode::Unknown)
		{
			throw std::runtime_error("<think button=\"" + written.button + "\">: there is no key named '" + written.button + "'");
		}
		const float depth = valueOr(written.depth, 4.0f, evaluate);
		if (!(depth >= 1.0f && depth <= 9.0f) || depth != std::floor(depth))
		{
			throw std::runtime_error("<think> looks " + formatDisplayNumber(depth) + " moves ahead; expected a whole number from 1 to 9");
		}
		return CmdThink{ raw.objClass, raw.object, button, static_cast<int>(depth), written.worths, written.avoid };
	}

	static void printThink(std::ostream& o, const CmdThink& think)
	{
		o << "think(" << think.objClass << ", " << think.picker << ", " << keyCodeToString(think.button) << ", " << think.depth;
		for (const auto& [look, worth] : think.worths) { o << ", " << look << "=" << worth; }
		o << ")";
	}

	extern const Verb thinkVerb;
	const Verb thinkVerb{
		.name = "think",
		.forms = { { "<think>", places::press, kindOf<CmdThink>() } },
		.read = readThink,
		.make = makeThink,
		.print = printAs<printThink>,
	};
}
