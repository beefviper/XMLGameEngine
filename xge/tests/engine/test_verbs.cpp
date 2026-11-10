// test_verbs.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Catch2 tests for the list of the verbs (xge/lib/verbs/): that it and the
// schema name the same commands, that a verb goes nowhere the schema would
// turn it away, that every kind of command a verb makes has one that runs
// it, and that the generated programs are given every verb's file.

#include "verbs/verb.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// A group of commands of the schema: the commands in it, and the groups
	// it takes whole.
	struct Group
	{
		std::set<std::string> commands;
		std::vector<std::string> groups;
	};

	// The schema's groups named ...Commands, by name.
	std::map<std::string, Group> schemaGroups()
	{
		std::map<std::string, Group> found;
		const auto document = XmlDocumentFactory::create();
		REQUIRE(document->load("xgedef.xsd"));
		const auto root = document->getRootElement();
		for (auto group = root->getFirstChild(); group != nullptr; group = group->getNextSibling())
		{
			const std::string name = group->getAttribute("name");
			if (group->getName() != "xs:group" || name.size() < 8 || name.substr(name.size() - 8) != "Commands") { continue; }
			const auto choice = group->getFirstChild();
			REQUIRE(choice);
			REQUIRE(choice->getName() == "xs:choice");
			for (auto each = choice->getFirstChild(); each != nullptr; each = each->getNextSibling())
			{
				if (each->getName() == "xs:element") { found[name].commands.insert(each->getAttribute("name")); }
				else if (each->getName() == "xs:group") { found[name].groups.push_back(each->getAttribute("ref")); }
			}
		}
		return found;
	}

	// Every command a group takes, with those of the groups in it.
	std::set<std::string> commandsOf(const std::map<std::string, Group>& groups, const std::string& name)
	{
		const Group& group = groups.at(name);
		std::set<std::string> all = group.commands;
		for (const std::string& inner : group.groups)
		{
			const std::set<std::string> more = commandsOf(groups, inner);
			all.insert(more.begin(), more.end());
		}
		return all;
	}

	// The group of the schema each place takes its commands from.
	const char* schemaGroupOf(Place place)
	{
		switch (place)
		{
		case Place::Edge:
		case Place::Touch: return "ruleCommands";
		case Place::Input:
		case Place::Condition:
		case Place::StateTimer: return "stateCommands";
		case Place::ObjectTimer:
		case Place::PathStep: return "objectTimeCommands";
		case Place::Action: return "actionCommands";
		case Place::Computer: return "pressCommands";
		case Place::Check: return "checkCommands";
		}
		return "";
	}

	const Place everyPlace[] = { Place::Edge, Place::Touch, Place::Input, Place::Condition, Place::StateTimer,
		Place::ObjectTimer, Place::Action, Place::PathStep, Place::Computer, Place::Check };
}

TEST_CASE("verbs: the list and the schema name the same commands", "[verbs]")
{
	const std::map<std::string, Group> groups = schemaGroups();
	std::set<std::string> inSchema;
	for (const auto& [name, group] : groups) { inSchema.insert(group.commands.begin(), group.commands.end()); }

	std::set<std::string> listed;
	for (const Verb* verb : allVerbs())
	{
		INFO("<" << verb->name << ">");
		CHECK(listed.insert(verb->name).second);
		CHECK(findVerb(verb->name) == verb);
		CHECK(verb->make != nullptr);
		CHECK(verb->forms[0].tag != nullptr);
	}

	for (const std::string& name : listed)
	{
		INFO("<" << name << "> is a verb");
		CHECK(inSchema.count(name) == 1);
	}
	for (const std::string& name : inSchema)
	{
		INFO("<" << name << "> is in a group of commands of the schema");
		CHECK(listed.count(name) == 1);
	}
	CHECK(findVerb("explode") == nullptr);
}

TEST_CASE("verbs: a verb goes only where the schema lets it", "[verbs]")
{
	const std::map<std::string, Group> groups = schemaGroups();
	for (const Verb* verb : allVerbs())
	{
		for (const Form& form : verb->forms)
		{
			if (!form.tag) { continue; }
			CHECK(form.places != 0);
			for (const Place place : everyPlace)
			{
				if (!(form.places & in(place))) { continue; }
				INFO(form.tag << " in the schema's " << schemaGroupOf(place));
				CHECK(commandsOf(groups, schemaGroupOf(place)).count(verb->name) == 1);
			}
		}
	}
}

TEST_CASE("verbs: every command a verb makes has a verb", "[verbs]")
{
	CHECK(verbOf(CmdBounce{}) == findVerb("bounce"));
	CHECK(verbOf(CmdResetObject{ "paddle" }) == findVerb("reset"));
	CHECK(verbOf(CmdHuntHop{}) == findVerb("hunt"));
	CHECK(verbOf(CmdSend{}) == findVerb("send"));
	CHECK(verbOf(CmdWhen{}) == nullptr);

	CHECK(std::string(formOf(CmdResetObject{ "paddle" })->tag) == "<reset object>");
	CHECK(std::string(formOf(CmdDespawn{})->tag) == "<despawn />");
	CHECK(std::string(formOf(CmdDespawn{ "shot" })->tag) == "<despawn object>");
	CHECK((formOf(CmdDespawn{})->places & in(Place::Input)) == 0);
	CHECK((formOf(CmdDespawn{ "shot" })->places & in(Place::Input)) != 0);

	// Only what something else runs has no run of its own.
	for (const Verb* verb : allVerbs())
	{
		const std::string name = verb->name;
		INFO("<" << name << ">");
		const bool runElsewhere = name == "track" || name == "hold" || name == "think" || name == "distribute";
		CHECK((verb->run == nullptr) == runElsewhere);
		for (const Form& form : verb->forms)
		{
			if (form.tag) { CHECK(form.kind < std::variant_size_v<Command>); }
		}
	}
}

TEST_CASE("verbs: the keys that act when let go", "[verbs]")
{
	std::set<std::string> held;
	for (const Verb* verb : allVerbs())
	{
		if (verb->onRelease) { held.insert(verb->name); }
	}
	CHECK(held == std::set<std::string>{ "move", "climb", "accelerate", "turn", "thrust", "swing", "play", "trigger" });
}

TEST_CASE("verbs: a generated program is given every verb's file", "[verbs]")
{
	const auto document = XmlDocumentFactory::create();
	REQUIRE(document->load("generators/windows-cpp/engine.xml"));
	std::set<std::string> files;
	const auto root = document->getRootElement();
	for (auto file = root->getFirstChild(); file != nullptr; file = file->getNextSibling()) { files.insert(file->getText()); }

	CHECK(files.count("lib/verbs/verb.h") == 1);
	CHECK(files.count("lib/verbs/all.cpp") == 1);
	for (const Verb* verb : allVerbs())
	{
		INFO("<" << verb->name << ">");
		CHECK(files.count(std::string("lib/verbs/") + verb->name + ".cpp") == 1);
	}
}
