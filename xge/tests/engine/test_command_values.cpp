// test_command_values.cpp
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026
//
// Every number a command holds is renamed alike: self made the object
// running it, an object's name made its copy's, and a live number in any of
// them makes the command again each time it runs. Each walk over a command
// once listed its values by hand, and the lists disagreed (docs/plan.md CR1).

#include "scratch.h"
#include "engine/engine.h"
#include "game/game.h"
#include "game/object.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace xge;

namespace
{
	// A number of a command: the shared one, or one of a verb's own data,
	// made when it is not there yet.
	using Place = RawValue& (*)(RawCommand&);

	template <RawValue RawCommand::*Part> RawValue& shared(RawCommand& command) { return command.*Part; }

	template <typename Own, std::optional<RawValue> Own::*Part> RawValue& own(RawCommand& command)
	{
		Own* data = std::get_if<Own>(&command.own);
		if (!data) { data = &command.own.template emplace<Own>(); }
		std::optional<RawValue>& value = data->*Part;
		return value ? *value : value.emplace();
	}

	// Every number a command can hold, with the name a message gives it.
	const std::vector<std::pair<const char*, Place>> commandValues{
		{ "amount", shared<&RawCommand::amount> }, { "seconds", shared<&RawCommand::seconds> },
		{ "distance", shared<&RawCommand::distance> }, { "ahead", shared<&RawCommand::ahead> },
		{ "positionX", shared<&RawCommand::positionX> }, { "positionY", shared<&RawCommand::positionY> },
		{ "column", shared<&RawCommand::column> }, { "row", shared<&RawCommand::row> },
		{ "bounce's share", own<RawBounce, &RawBounce::share> },
		{ "hop's distance", own<RawHop, &RawHop::distance> }, { "hop's seconds", own<RawHop, &RawHop::seconds> }, { "hop's arc", own<RawHop, &RawHop::arc> },
		{ "spawn's count", own<RawSpawn, &RawSpawn::count> }, { "spawn's x", own<RawSpawn, &RawSpawn::positionX> }, { "spawn's y", own<RawSpawn, &RawSpawn::positionY> },
		{ "spawn's velocity x", own<RawSpawn, &RawSpawn::velocityX> }, { "spawn's velocity y", own<RawSpawn, &RawSpawn::velocityY> },
		{ "spawn's acceleration x", own<RawSpawn, &RawSpawn::accelerationX> }, { "spawn's acceleration y", own<RawSpawn, &RawSpawn::accelerationY> },
		{ "spawn's column", own<RawSpawn, &RawSpawn::column> }, { "spawn's row", own<RawSpawn, &RawSpawn::row> },
		{ "spawn's below", own<RawSpawn, &RawSpawn::freeBelow> }, { "spawn's member", own<RawSpawn, &RawSpawn::member> },
	};

	RawCommand commandWith(Place place, const std::string& text)
	{
		RawCommand command;
		command.verb = "spawn";
		place(command) = RawValue::expression(text);
		return command;
	}

	// A file that is removed again when the test ends.
	struct ScratchGame
	{
		std::filesystem::path path{ scratchName("descriptions/values_scratch.xml") };

		explicit ScratchGame(const std::string& xml) { std::ofstream(path) << xml; }
		~ScratchGame()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
		ScratchGame(const ScratchGame&) = delete;
		ScratchGame& operator=(const ScratchGame&) = delete;
	};
}

TEST_CASE("self in any of a command's numbers is the object running it", "[values]")
{
	for (const auto& [name, part] : commandValues)
	{
		INFO(name);
		RawCommand command = commandWith(part, "self.h + 1");
		CHECK(nameSelfIn(command, "frog"));
		CHECK(part(command).text == "frog.h + 1");
	}
}

TEST_CASE("an object's name in any of a command's numbers is renamed", "[values]")
{
	for (const auto& [name, part] : commandValues)
	{
		INFO(name);
		std::vector<RawCommand> list{ commandWith(part, "bomb.h * 2") };
		CHECK(renameObjectIn(list, "bomb", "shell"));
		CHECK(part(list.front()).text == "shell.h * 2");
	}
}

TEST_CASE("a copy's own name in any of its commands' numbers is the copy's", "[values]")
{
	for (const auto& [name, part] : commandValues)
	{
		INFO(name);
		RawObject object;
		object.name = "bomb";
		object.action["drop"] = { commandWith(part, "bomb.h") };
		const Renamed renamed = renameOwn(object, "bomb", "bomb.1");
		CHECK(renamed.kind);
		CHECK(part(object.action["drop"].front()).text == "bomb.1.h");
	}
}

TEST_CASE("a live number in a hop's arc is read as the hop is made", "[values]")
{
	const ScratchGame file(R"(<game>
  <window name="t"><width>400</width><height>400</height><background>color.black</background><framerate>60</framerate></window>
  <variables><variable name="unused">0</variable></variables>
  <states>
    <state name="playing">
      <objects>
        <object name="frog">
          <sprite><rectangle><width>10</width><height>10</height><color>color.white</color></rectangle></sprite>
          <position><x>100</x><y>300</y></position>
          <actions><action name="leap"><hop direction="up">10<seconds>1</seconds><arc>frog.h</arc></hop></action></actions>
          <variables><variable name="h">20</variable></variables>
        </object>
      </objects>
      <inputs>
        <input button="space"><inc variable="frog.h">30</inc><trigger object="frog" action="leap" /></input>
      </inputs>
    </state>
  </states>
</game>
)");
	Game game(file.path.string());
	Engine engine(game, std::make_unique<NullWindow>());
	engine.handleKeyPressed(KeyCode::Space);
	engine.input();
	engine.update(kSixtieth);
	const Object& frog = game.getObject("frog");
	REQUIRE(frog.isAirborne());
	CHECK(frog.flight.arc == 50.0f);
}
