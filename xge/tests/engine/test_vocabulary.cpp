// test_vocabulary.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Catch2 tests that keep the language honest while it is cut: every word the
// schema has is used by some game (or is listed below with why it stays),
// every word is in the reference docs, and every game lives through a short
// run of random keys. That every command the schema has is a verb, and every
// verb a command, is test_verbs.cpp's.

#include "cli.h"
#include "engine/folder.h"
#include "random_keys.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// Every name an element of the tree under `node` has, and, with `named`,
	// the name= of every element called that instead (the schema's
	// xs:element name="...").
	void collect(const XmlNode& node, std::set<std::string>& out, const std::string& named = {})
	{
		if (named.empty()) { out.insert(node.getName()); }
		else if (node.getName() == named && !node.getAttribute("name").empty()) { out.insert(node.getAttribute("name")); }
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			collect(*child, out, named);
		}
	}

	std::set<std::string> schemaWords()
	{
		auto schema = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(schema->load(findSchema("descriptions/pong.xml", "../xgedef.xsd").string()));
		std::set<std::string> words;
		collect(*schema->getRootElement(), words, "xs:element");
		REQUIRE(words.size() > 100);
		return words;
	}

	// Each word the games use: the games that use it.
	std::map<std::string, std::set<std::string>> gameWords()
	{
		std::map<std::string, std::set<std::string>> used;
		for (const std::string& file : played::shippedGames(gameFilesIn("descriptions")))
		{
			auto document = XmlDocumentFactory::create(XmlBackend::TinyXml2);
			REQUIRE(document->load(file));
			std::set<std::string> words;
			collect(*document->getRootElement(), words);
			const std::string game = std::filesystem::path(file).stem().string();
			for (const std::string& word : words) { used[word].insert(game); }
		}
		return used;
	}

	// Words no game uses, and why each stays for now. The plan's R1 decides
	// each: a game takes it up, or it goes from the schema and this list.
	const std::map<std::string, std::string> kUnused{
		{ "absolute", "a <formula>, beside sign and negate" },
		{ "floor", "a <formula>, beside ceiling and round" },
		{ "framerate", "a window's; 60 when left out" },
		{ "fullscreen", "a window's; false when left out" },
		{ "gap", "a group's, between cells" },
		{ "image", "a picture file as a sprite, such as a still background" },
		{ "margin", "a group's, round its cells" },
		{ "near", "a chase that stops short" },
		{ "negate", "a <formula>, beside sign and absolute" },
		{ "odd", "the pair of <even> in rows and columns" },
		{ "pull", "one object drawn to another" },
		{ "remainder", "a <formula>" },
		{ "rise", "a live number, how fast the altitude changes" },
	};
}

TEST_CASE("every word of the schema is used by some game, or is listed with why it stays", "[vocabulary]")
{
	const std::set<std::string> words = schemaWords();
	const auto used = gameWords();

	for (const std::string& word : words)
	{
		const auto games = used.find(word);
		if (games != used.end())
		{
			CHECK_FALSE(kUnused.count(word)); // listed, and now a game uses it: take it off the list
			continue;
		}
		INFO("<" << word << "> is used by no game: use it in one, take it out of the schema, or list it in kUnused with why it stays");
		CHECK(kUnused.count(word));
	}
	for (const auto& [word, why] : kUnused)
	{
		INFO("kUnused lists <" << word << ">, which the schema does not have");
		CHECK(words.count(word));
	}

	// The words one game alone uses, for R1 to look at.
	std::ostringstream alone;
	int count = 0;
	for (const auto& [word, games] : used)
	{
		if (games.size() == 1 && words.count(word)) { alone << " " << word << " (" << *games.begin() << ")"; ++count; }
	}
	WARN(count << " words one game alone uses:" << alone.str());

	// What a game writes is a word of the schema.
	for (const auto& [word, games] : used)
	{
		INFO("<" << word << "> is in " << *games.begin() << " and not in the schema");
		CHECK(words.count(word));
	}
}

TEST_CASE("every word of the schema is in the reference docs", "[vocabulary]")
{
	// The docs are in the repository the program was built from, beside xge/.
	const std::filesystem::path docs = std::filesystem::path(XGE_ENGINE_FOLDER) / ".." / "docs" / "reference";
	if (!std::filesystem::is_directory(docs)) { SKIP("no docs/reference beside " << XGE_ENGINE_FOLDER); }

	std::string text;
	for (const auto& entry : std::filesystem::directory_iterator(docs))
	{
		if (entry.path().extension() != ".md") { continue; }
		std::ifstream in(entry.path(), std::ios::binary);
		std::ostringstream read;
		read << in.rdbuf();
		text += read.str();
	}
	REQUIRE_FALSE(text.empty());

	for (const std::string& word : schemaWords())
	{
		// As a tag (`<word>`, `<word />`, in a table or an example) or as a name in code.
		const bool named = text.find("<" + word + ">") != std::string::npos || text.find("<" + word + " ") != std::string::npos
			|| text.find("<" + word + "/") != std::string::npos || text.find("`" + word + "`") != std::string::npos;
		INFO("<" << word << "> is in no page of docs/reference");
		CHECK(named);
	}
}

TEST_CASE("every game lives through 1,000 frames of random keys", "[vocabulary][keys]")
{
	for (const std::string& file : played::shippedGames(gameFilesIn("descriptions")))
	{
		played::playRandomKeys(file, 1u, 1000);
	}
}
