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
	using Place = WrittenValue& (*)(WrittenCommand&);

	template <typename Own> Own& ownData(WrittenCommand& command)
	{
		Own* data = std::get_if<Own>(&command.own);
		return data ? *data : command.own.template emplace<Own>();
	}

	template <typename Own, std::optional<WrittenValue> Own::*Part> WrittenValue& own(WrittenCommand& command)
	{
		std::optional<WrittenValue>& value = ownData<Own>(command).*Part;
		return value ? *value : value.emplace();
	}

	template <typename Own, WrittenValue Own::*Part> WrittenValue& always(WrittenCommand& command) { return ownData<Own>(command).*Part; }

	// Every number a command can hold, with the name a message gives it.
	const std::vector<std::pair<const char*, Place>> commandValues{
		{ "bounce's share", own<WrittenBounce, &WrittenBounce::share> }, { "knock's bounce", own<WrittenKnock, &WrittenKnock::bounce> },
		{ "hop's distance", own<WrittenHop, &WrittenHop::distance> }, { "hop's seconds", own<WrittenHop, &WrittenHop::seconds> }, { "hop's arc", own<WrittenHop, &WrittenHop::arc> },
		{ "spawn's count", own<WrittenSpawn, &WrittenSpawn::count> }, { "spawn's x", own<WrittenSpawn, &WrittenSpawn::positionX> }, { "spawn's y", own<WrittenSpawn, &WrittenSpawn::positionY> },
		{ "spawn's velocity x", own<WrittenSpawn, &WrittenSpawn::velocityX> }, { "spawn's velocity y", own<WrittenSpawn, &WrittenSpawn::velocityY> },
		{ "spawn's acceleration x", own<WrittenSpawn, &WrittenSpawn::accelerationX> }, { "spawn's acceleration y", own<WrittenSpawn, &WrittenSpawn::accelerationY> },
		{ "spawn's column", own<WrittenSpawn, &WrittenSpawn::column> }, { "spawn's row", own<WrittenSpawn, &WrittenSpawn::row> },
		{ "spawn's below", own<WrittenSpawn, &WrittenSpawn::freeBelow> }, { "spawn's member", own<WrittenSpawn, &WrittenSpawn::member> },
		{ "inc's by", own<WrittenInc, &WrittenInc::by> }, { "dec's by", own<WrittenDec, &WrittenDec::by> }, { "set's value", always<WrittenSet, &WrittenSet::value> },
		{ "jump's height", always<WrittenJump, &WrittenJump::height> }, { "pass's member", own<WrittenPass, &WrittenPass::member> }, { "reveal's count", own<WrittenReveal, &WrittenReveal::count> },
		{ "go's speed", always<WrittenGo, &WrittenGo::speed> }, { "wander's speed", always<WrittenWander, &WrittenWander::speed> }, { "climb's speed", always<WrittenClimb, &WrittenClimb::speed> },
		{ "follow's stagger", own<WrittenFollow, &WrittenFollow::stagger> }, { "follow's nearest", own<WrittenFollow, &WrittenFollow::nearest> },
		{ "move's step", always<WrittenMove, &WrittenMove::step> }, { "move's wait", own<WrittenMove, &WrittenMove::wait> }, { "turn's rate", own<WrittenTurn, &WrittenTurn::rate> },
		{ "thrust's amount", always<WrittenThrust, &WrittenThrust::amount> }, { "deflect's widest", always<WrittenDeflect, &WrittenDeflect::widest> },
		{ "hole's radius", always<WrittenHole, &WrittenHole::radius> }, { "hole's rough", own<WrittenHole, &WrittenHole::rough> },
		{ "swing's degrees", always<WrittenSwing, &WrittenSwing::degrees> }, { "swing's seconds", always<WrittenSwing, &WrittenSwing::seconds> },
		{ "chase's speed", always<WrittenChase, &WrittenChase::speed> }, { "chase's near", own<WrittenChase, &WrittenChase::near> }, { "aim's lined", own<WrittenAim, &WrittenAim::lined> },
		{ "shift's x", own<WrittenShift, &WrittenShift::x> }, { "shift's y", own<WrittenShift, &WrittenShift::y> },
		{ "hunt's speed", always<WrittenHunt, &WrittenHunt::speed> }, { "hunt's ahead", own<WrittenHunt, &WrittenHunt::ahead> }, { "hunt's retreat", always<WrittenHunt, &WrittenHunt::retreatWithin> },
		{ "send's column", own<WrittenSend, &WrittenSend::column> }, { "send's row", own<WrittenSend, &WrittenSend::row> }, { "send's count", own<WrittenSend, &WrittenSend::count> },
		{ "think's depth", own<WrittenThink, &WrittenThink::depth> },
	};

	WrittenCommand commandWith(Place place, const std::string& text)
	{
		WrittenCommand command;
		command.verb = "spawn";
		place(command) = WrittenValue::expression(text);
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
		WrittenCommand command = commandWith(part, "self.h + 1");
		CHECK(nameSelfIn(command, "frog"));
		CHECK(part(command).text == "frog.h + 1");
	}
}

TEST_CASE("an object's name in any of a command's numbers is renamed", "[values]")
{
	for (const auto& [name, part] : commandValues)
	{
		INFO(name);
		std::vector<WrittenCommand> list{ commandWith(part, "bomb.h * 2") };
		CHECK(renameObjectIn(list, "bomb", "shell"));
		CHECK(part(list.front()).text == "shell.h * 2");
	}
}

TEST_CASE("a copy's own name in any of its commands' numbers is the copy's", "[values]")
{
	for (const auto& [name, part] : commandValues)
	{
		INFO(name);
		WrittenObject object;
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
