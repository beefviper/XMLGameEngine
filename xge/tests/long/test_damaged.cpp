// test_damaged.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// A long test (xgetest-long): every game broken 40 ways, each copy loaded as
// --check loads it and, if it loads, played for 600 frames on the null
// window. A broken game may stop with a message, at load or while it plays;
// it may never crash or hang, so getting to the end is the test. What
// happened is counted and printed. The copies are made by a small reader
// and writer of the games' XML here, so no XML library decides what is
// broken: an element taken out, an attribute taken out, an element doubled,
// an attribute or a text set to nonsense.

#include "cli.h"
#include "engine/engine.h"
#include "file_text.h"
#include "game/game.h"
#include "game/random.h"
#include "load/xml.h"
#include "scratch.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <cctype>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
	// An element as written: its tag, attributes in order, and what is in it
	// (elements, or text kept as written, comments dropped).
	struct Node
	{
		std::string tag;    // empty: text
		std::string text;
		std::vector<std::pair<std::string, std::string>> attributes;
		std::vector<Node> children;
	};

	class Reader
	{
	public:
		explicit Reader(const std::string& text) : s(text) {}

		Node document()
		{
			skipProlog();
			return element();
		}

	private:
		const std::string& s;
		std::size_t at{ 0 };

		void skipProlog()
		{
			while (true)
			{
				at = s.find('<', at);
				if (s.compare(at, 2, "<?") == 0) { at = s.find("?>", at) + 2; }
				else if (s.compare(at, 4, "<!--") == 0) { at = s.find("-->", at) + 3; }
				else { return; }
			}
		}

		std::string name()
		{
			const std::size_t start = at;
			while (at < s.size() && !std::isspace(static_cast<unsigned char>(s[at])) && s[at] != '>' && s[at] != '/' && s[at] != '=') { ++at; }
			return s.substr(start, at - start);
		}

		void spaces()
		{
			while (at < s.size() && std::isspace(static_cast<unsigned char>(s[at]))) { ++at; }
		}

		Node element()
		{
			Node node;
			++at; // '<'
			node.tag = name();
			while (true)
			{
				spaces();
				if (s[at] == '/') { at += 2; return node; }
				if (s[at] == '>') { ++at; break; }
				std::string key = name();
				spaces();
				++at; // '='
				spaces();
				const char quote = s[at++];
				const std::size_t end = s.find(quote, at);
				node.attributes.emplace_back(std::move(key), s.substr(at, end - at));
				at = end + 1;
			}
			while (true)
			{
				const std::size_t next = s.find('<', at);
				if (next > at) { node.children.push_back(Node{ {}, s.substr(at, next - at), {}, {} }); }
				at = next;
				if (s.compare(at, 4, "<!--") == 0) { at = s.find("-->", at) + 3; continue; }
				if (s.compare(at, 2, "</") == 0) { at = s.find('>', at) + 1; return node; }
				node.children.push_back(element());
			}
		}
	};

	void write(const Node& node, std::string& out)
	{
		if (node.tag.empty()) { out += node.text; return; }
		out += "<" + node.tag;
		for (const auto& [key, value] : node.attributes) { out += " " + key + "=\"" + value + "\""; }
		if (node.children.empty()) { out += " />"; return; }
		out += ">";
		for (const Node& child : node.children) { write(child, out); }
		out += "</" + node.tag + ">";
	}

	// Every element below the root, with its parent, as places to break.
	void elementsIn(Node& node, std::vector<std::pair<Node*, std::size_t>>& out)
	{
		for (std::size_t i = 0; i < node.children.size(); ++i)
		{
			if (node.children[i].tag.empty()) { continue; }
			out.emplace_back(&node, i);
			elementsIn(node.children[i], out);
		}
	}

	bool isLeaf(const Node& node)
	{
		return !node.tag.empty() && node.children.size() == 1 && node.children.front().tag.empty();
	}

	// One copy broken one way; what it says it did, for the count.
	std::string damage(Node& root, std::mt19937& pick)
	{
		std::vector<std::pair<Node*, std::size_t>> places;
		elementsIn(root, places);
		const auto any = [&](auto test) -> std::pair<Node*, std::size_t>
		{
			for (int tries = 0; tries < 200; ++tries)
			{
				const auto place = places[pick() % places.size()];
				if (test(place.first->children[place.second])) { return place; }
			}
			return { nullptr, 0 };
		};
		const char* const attributeValues[] = { "nosuch", "self", "0", "a.b.c.d", "" };
		const char* const textValues[] = { "0", "-1", "1000000000", "abc", "", "1 / 0", "nosuch.thing" };

		switch (pick() % 5)
		{
		case 0:
		{
			const auto [parent, i] = places[pick() % places.size()];
			parent->children.erase(parent->children.begin() + static_cast<std::ptrdiff_t>(i));
			return "an element taken out";
		}
		case 1:
		{
			const auto [parent, i] = any([](const Node& node) { return !node.attributes.empty(); });
			if (!parent) { return "nothing"; }
			auto& attributes = parent->children[i].attributes;
			attributes.erase(attributes.begin() + static_cast<std::ptrdiff_t>(pick() % attributes.size()));
			return "an attribute taken out";
		}
		case 2:
		{
			const auto [parent, i] = places[pick() % places.size()];
			const Node copy = parent->children[i];
			parent->children.insert(parent->children.begin() + static_cast<std::ptrdiff_t>(i) + 1, copy);
			return "an element doubled";
		}
		case 3:
		{
			const auto [parent, i] = any([](const Node& node) { return !node.attributes.empty(); });
			if (!parent) { return "nothing"; }
			auto& attributes = parent->children[i].attributes;
			attributes[pick() % attributes.size()].second = attributeValues[pick() % std::size(attributeValues)];
			return "an attribute set to nonsense";
		}
		default:
		{
			const auto [parent, i] = any(isLeaf);
			if (!parent) { return "nothing"; }
			parent->children[i].children.front().text = textValues[pick() % std::size(textValues)];
			return "a text set to nonsense";
		}
		}
	}
}

TEST_CASE("every game broken 40 ways stops with a message or plays on, never crashes", "[long][damaged]")
{
	constexpr int kCopies = 40;
	constexpr int kFrames = 600;
	const std::string scratch = scratchName("descriptions/damaged.xml");

	int copies = 0;
	int refused = 0;
	int stoppedPlaying = 0;
	int played = 0;
	std::mt19937 pick(1);
	for (const std::string& file : xge::gameFilesIn("descriptions"))
	{
		if (file.find("damaged") != std::string::npos) { continue; }
		std::ifstream in(file);
		const Node original = Reader(fileText(in)).document();
		for (int copy = 0; copy < kCopies; ++copy)
		{
			Node broken = original;
			const std::string how = damage(broken, pick);
			std::string text = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			write(broken, text);
			std::ofstream(scratch, std::ios::binary | std::ios::trunc) << text;
			++copies;

			// Xerces (full schema) for half, TinyXML2 (the weaker check) for the rest.
			const xge::XmlBackend xml = copy % 2 == 0 ? xge::XmlBackend::Xerces : xge::XmlBackend::TinyXml2;
			INFO(file << ", copy " << copy << ": " << how);
			std::streambuf* const before = std::cout.rdbuf();
			std::ostringstream unheard;
			std::cout.rdbuf(unheard.rdbuf());
			try
			{
				xge::fixRandomSeed(static_cast<unsigned>(copy + 1));
				xge::Game game{ xge::readGame(scratch, xml) };
				xge::Engine engine(game, std::make_unique<xge::NullWindow>(game.getWindowDesc()), nullptr);
				try
				{
					for (int frame = 0; frame < kFrames; ++frame)
					{
						if (frame % 30 == 0) { engine.handleKeyPressed(xge::KeyCode::Space); engine.handleKeyReleased(xge::KeyCode::Space); }
						engine.step();
					}
					++played;
				}
				catch (const std::exception&)
				{
					++stoppedPlaying;
				}
			}
			catch (const std::exception&)
			{
				++refused;
			}
			std::cout.rdbuf(before);
		}
	}
	std::filesystem::remove(scratch);

	std::printf("damaged copies: %d; refused at load: %d; stopped while playing: %d; played %d frames: %d\n",
		copies, refused, stoppedPlaying, kFrames, played);
	CHECK(copies == refused + stoppedPlaying + played);
}
