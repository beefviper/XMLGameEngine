// test_command_values.cpp
// XML Game Engine
// author: beefviper
// date: Nov 19, 2026
//
// Every number a command holds is renamed alike: self made the object
// running it, an object's name made its copy's, and a live number in any of
// them makes the command again each time it runs. Each walk over a command
// once listed its values by hand, and the lists disagreed (docs/plan.md CR1);
// now each verb's own data lists its numbers, and this list holds every one.

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
	// A number of a command: one of its verb's own, made when it is not
	// there yet.
	using Place = RawValue& (*)(RawCommand&);

	template <typename Own> Own& ownData(RawCommand& command)
	{
		Own* data = std::get_if<Own>(&command.own);
		return data ? *data : command.own.template emplace<Own>();
	}

	template <typename Own, std::optional<RawValue> Own::*Part> RawValue& own(RawCommand& command)
	{
		std::optional<RawValue>& value = ownData<Own>(command).*Part;
		return value ? *value : value.emplace();
	}

	template <typename Own, RawValue Own::*Part> RawValue& always(RawCommand& command) { return ownData<Own>(command).*Part; }

	// Every number a command can hold, with the name a message gives it.
	const std::vector<std::pair<const char*, Place>> commandValues{
		{ "bounce's share", own<RawBounce, &RawBounce::share> }, { "knock's bounce", own<RawKnock, &RawKnock::bounce> },
		{ "hop's distance", own<RawHop, &RawHop::distance> }, { "hop's seconds", own<RawHop, &RawHop::seconds> }, { "hop's arc", own<RawHop, &RawHop::arc> },
		{ "spawn's count", own<RawSpawn, &RawSpawn::count> }, { "spawn's x", own<RawSpawn, &RawSpawn::positionX> }, { "spawn's y", own<RawSpawn, &RawSpawn::positionY> },
		{ "spawn's velocity x", own<RawSpawn, &RawSpawn::velocityX> }, { "spawn's velocity y", own<RawSpawn, &RawSpawn::velocityY> },
		{ "spawn's acceleration x", own<RawSpawn, &RawSpawn::accelerationX> }, { "spawn's acceleration y", own<RawSpawn, &RawSpawn::accelerationY> },
		{ "spawn's column", own<RawSpawn, &RawSpawn::column> }, { "spawn's row", own<RawSpawn, &RawSpawn::row> },
		{ "spawn's below", own<RawSpawn, &RawSpawn::freeBelow> }, { "spawn's member", own<RawSpawn, &RawSpawn::member> },
		{ "inc's by", own<RawInc, &RawInc::by> }, { "dec's by", own<RawDec, &RawDec::by> }, { "set's value", always<RawSet, &RawSet::value> },
		{ "jump's height", always<RawJump, &RawJump::height> }, { "pass's member", own<RawPass, &RawPass::member> }, { "reveal's count", own<RawReveal, &RawReveal::count> },
		{ "go's speed", always<RawGo, &RawGo::speed> }, { "wander's speed", always<RawWander, &RawWander::speed> }, { "climb's speed", always<RawClimb, &RawClimb::speed> },
		{ "follow's stagger", own<RawFollow, &RawFollow::stagger> }, { "follow's nearest", own<RawFollow, &RawFollow::nearest> },
		{ "move's step", always<RawMove, &RawMove::step> }, { "move's wait", own<RawMove, &RawMove::wait> }, { "turn's rate", own<RawTurn, &RawTurn::rate> },
		{ "thrust's amount", always<RawThrust, &RawThrust::amount> }, { "deflect's widest", always<RawDeflect, &RawDeflect::widest> },
		{ "hole's radius", always<RawHole, &RawHole::radius> }, { "hole's rough", own<RawHole, &RawHole::rough> },
		{ "swing's degrees", always<RawSwing, &RawSwing::degrees> }, { "swing's seconds", always<RawSwing, &RawSwing::seconds> },
		{ "chase's speed", always<RawChase, &RawChase::speed> }, { "chase's near", own<RawChase, &RawChase::near> }, { "aim's lined", own<RawAim, &RawAim::lined> },
		{ "shift's x", own<RawShift, &RawShift::x> }, { "shift's y", own<RawShift, &RawShift::y> },
		{ "hunt's speed", always<RawHunt, &RawHunt::speed> }, { "hunt's ahead", own<RawHunt, &RawHunt::ahead> }, { "hunt's retreat", always<RawHunt, &RawHunt::retreatWithin> },
		{ "send's column", own<RawSend, &RawSend::column> }, { "send's row", own<RawSend, &RawSend::row> }, { "send's count", own<RawSend, &RawSend::count> },
		{ "think's depth", own<RawThink, &RawThink::depth> },
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
	const Object& frog = game.object("frog");
	REQUIRE(frog.isAirborne());
	CHECK(frog.flight.arc == 50.0f);
}
