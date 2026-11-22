// test_tree.cpp
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026
//
// A game held as a tree of plain C++ data (xml/tree.h), as a generated
// program holds it, reads the same as its file: the same objects, sprites,
// positions, commands and rules.

#include "game/game.h"
#include "load/expr.h"
#include "load/xml.h"
#include "xml/document.h"
#include "xml/tree.h"

#include <catch2/catch_test_macros.hpp>

#include <deque>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// A tree built from an XmlNode, with the text and arrays it points at.
	class BuiltTree
	{
	public:
		explicit BuiltTree(const XmlNode& node) : root(build(node)) {}

	private:
		// Made before root, which points into them.
		std::deque<std::string> texts;
		std::deque<std::vector<TreeAttribute>> attributeLists;
		std::deque<std::vector<TreeElement>> childLists;

	public:
		const TreeElement root;

	private:

		const char* keep(std::string text) { return texts.emplace_back(std::move(text)).c_str(); }

		TreeElement build(const XmlNode& node)
		{
			std::vector<TreeAttribute>& attributes = attributeLists.emplace_back();
			for (const auto& [name, value] : node.getAttributes()) { attributes.push_back({ keep(name), keep(value) }); }
			std::vector<TreeElement> built;
			for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				built.push_back(build(*child));
			}
			std::vector<TreeElement>& children = childLists.emplace_back(std::move(built));
			return { keep(node.getName()), attributes.data(), attributes.size(), keep(node.getText()), children.data(), children.size(), node.line() };
		}
	};

	struct SameSeed
	{
		SameSeed() { Evaluator::fixRandomSeed(20261119u); }
		~SameSeed() { Evaluator::fixRandomSeed(std::nullopt); }
	};

	std::vector<std::string> printed(Game&& game)
	{
		std::vector<std::string> all;
		for (const auto& object : game.getCurrentObjects())
		{
			std::ostringstream text;
			text << object;
			all.push_back(text.str());
		}
		return all;
	}

	void readsTheSame(const std::string& name)
	{
		const std::string file = "descriptions/" + name + ".xml";
		INFO(file);
		const SameSeed seed;
		const std::vector<std::string> expected = printed(Game{ readGame(file) });

		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(XmlBackend::PugiXml);
		REQUIRE(document->load(file));
		const BuiltTree tree(*document->getRootElement());
		document.reset();
		const std::vector<std::string> fromTree = printed(Game{ readGame(*viewOfTree(tree.root)) });

		REQUIRE(fromTree.size() == expected.size());
		for (std::size_t i = 0; i < expected.size(); ++i) { CHECK(fromTree[i] == expected[i]); }
	}
}

TEST_CASE("a game held as a tree of C++ data reads as its file does", "[.long][tree]")
{
	for (const char* game : { "pong", "pacman", "klondike", "mariobros" }) { readsTheSame(game); }
}

TEST_CASE("a tree's element gives its name, attributes, text, line and children in order", "[tree]")
{
	const TreeAttribute attributes[]{ { "name", "ball" }, { "class", "round" } };
	const TreeElement children[]{
		{ "x", nullptr, 0, "10", nullptr, 0, 3 },
		{ "y", nullptr, 0, "20", nullptr, 0, 4 },
	};
	const TreeElement root{ "position", attributes, 2, "\n  ", children, 2, 2 };

	const std::unique_ptr<XmlNode> node = viewOfTree(root);
	CHECK(node->getName() == "position");
	CHECK(node->getAttribute("class") == "round");
	CHECK(node->getAttribute("missing").empty());
	CHECK(node->getAttributes().size() == 2);
	CHECK(node->line() == 2);
	CHECK(node->getNextSibling() == nullptr);

	const std::unique_ptr<XmlNode> x = node->getFirstChild();
	REQUIRE(x != nullptr);
	CHECK(x->getName() == "x");
	CHECK(x->getText() == "10");
	CHECK(x->getFirstChild() == nullptr);
	const std::unique_ptr<XmlNode> y = x->getNextSibling();
	REQUIRE(y != nullptr);
	CHECK(y->getName() == "y");
	CHECK(y->line() == 4);
	CHECK(y->getNextSibling() == nullptr);
}
