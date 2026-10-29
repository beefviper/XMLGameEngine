// test_refused.cpp
// XML Game Engine
// author: beefviper
// date: Oct 18, 2026
//
// Catch2 tests for a press that cannot be done: an action stops at a verb
// that is refused (a <fire> with every shot in flight, a hop in the air),
// so the sound and the look written after it are not given either. A
// collision rule, a key of a state and a condition stop the same way.

#include "played.h"
#include "file_text.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace xge;
using played::Played;

TEST_CASE("spaceinvaders.xml: a press of fire while the bullet is in flight makes no sound", "[refused]")
{
	Played play("descriptions/spaceinvaders.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.frames(1);
	(void)play.game.takeSoundRequests();

	play.tap(KeyCode::Space);
	CHECK(play.get("bullet").collisionsOn);
	CHECK(play.game.takeSoundRequests() == std::vector<std::string>{ "shoot" });

	play.frames(2);
	(void)play.game.takeSoundRequests();
	play.tap(KeyCode::Space);
	CHECK(play.game.takeSoundRequests().empty());
}

TEST_CASE("qbert.xml: a key pressed in the air changes neither Q*bert's look nor makes a sound", "[refused][qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.frames(1);
	(void)play.game.takeSoundRequests();

	play.tap(KeyCode::D);
	CHECK(play.get("qbert").lookName() == "front");
	CHECK(play.game.takeSoundRequests().size() == 1);

	play.frames(4);
	REQUIRE(play.get("qbert").isAirborne());
	(void)play.game.takeSoundRequests();
	play.tap(KeyCode::A);
	CHECK(play.get("qbert").lookName() == "front");
	CHECK(play.game.takeSoundRequests().empty());
}

TEST_CASE("an action runs what comes before a refused verb, and nothing after it", "[refused]")
{
	const played::Scratch scratch(
		played::box("gun", 100, 300,
			"<actions><action name=\"shoot\"><play sound=\"click\" /><fire object=\"shot\" /><play sound=\"bang\" /></action></actions>")
			+ played::box("shot", 100, 200, "<velocity><x>0</x><y>-1</y></velocity>"),
		"<trigger object=\"gun\" action=\"shoot\" />");
	// The scratch game has no sounds; put two in before it is loaded.
	{
		std::ifstream in(scratch.path);
		std::string text = fileText(in);
		const std::string sounds = "<sounds><sound name=\"click\" wave=\"square\"><note pitch=\"C5\">0.05</note></sound>"
			"<sound name=\"bang\" wave=\"square\"><note pitch=\"C4\">0.05</note></sound></sounds>";
		text.insert(text.find("<states>"), sounds);
		std::ofstream(scratch.path) << text;
	}
	Played play(scratch.path);

	play.tap(KeyCode::Space);
	CHECK(play.game.takeSoundRequests() == std::vector<std::string>{ "click", "bang" });
	play.tap(KeyCode::Space);
	CHECK(play.game.takeSoundRequests() == std::vector<std::string>{ "click" });
}

namespace
{
	const std::string kCappedShot = "<object name=\"shot\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite><most>2</most></object>";
}

TEST_CASE("a state's key stops at a refused spawn", "[refused][spawn]")
{
	const played::Scratch file(played::box("ship", 300, 400, "<variables><variable name=\"shots\">0</variable></variables>"),
		"<spawn object=\"shot\" /><inc variable=\"ship.shots\" />", {}, kCappedShot);
	Played play(file.path);

	for (int i = 0; i < 4; ++i) { play.tap(KeyCode::Space); }
	play.frames(1);
	CHECK(play.count("shot") == 2);
	CHECK(play.get("ship").variable.at("shots") == 2.0f);
}

TEST_CASE("an edge rule stops at a refused spawn, and the rules after it still run", "[refused][spawn]")
{
	const played::Scratch file(played::box("ship", 0, 400,
		"<velocity><x>-1</x><y>0</y></velocity>"
		"<collisions><collision edge=\"left\"><spawn object=\"shot\" /><inc variable=\"self.shots\" /></collision>"
		"<collision edge=\"left\"><inc variable=\"self.touches\" /></collision></collisions>"
		"<variables><variable name=\"shots\">0</variable><variable name=\"touches\">0</variable></variables>"),
		"", {}, kCappedShot);
	Played play(file.path);

	play.frames(5);
	CHECK(play.count("shot") == 2);
	CHECK(play.get("ship").variable.at("shots") == 2.0f);
	CHECK(play.get("ship").variable.at("touches") > 2.0f);
}

TEST_CASE("a rule about another object stops at a refused spawn", "[refused][spawn]")
{
	const played::Scratch file(played::box("ship", 300, 400,
		"<collisions><collision object=\"rock\"><spawn object=\"shot\" /><inc variable=\"self.shots\" /></collision>"
		"<collision object=\"rock\"><inc variable=\"self.touches\" /></collision></collisions>"
		"<variables><variable name=\"shots\">0</variable><variable name=\"touches\">0</variable></variables>")
		+ played::box("rock", 312, 400, "<velocity><x>-1</x><y>0</y></velocity><collisions><enabled>true</enabled></collisions>"),
		"", {}, kCappedShot);
	Played play(file.path);

	play.frames(8);
	CHECK(play.count("shot") == 2);
	CHECK(play.get("ship").variable.at("shots") == 2.0f);
	CHECK(play.get("ship").variable.at("touches") > 2.0f);
}

TEST_CASE("a condition stops at a refused spawn", "[refused][spawn]")
{
	const played::Scratch file(played::box("ship", 300, 400, "<variables><variable name=\"shots\">0</variable></variables>"),
		"", "<conditions><condition object=\"ship\" variable=\"shots\"><below>10</below>"
		"<spawn object=\"shot\" /><inc variable=\"ship.shots\" /></condition></conditions>", kCappedShot);
	Played play(file.path);

	play.frames(5);
	CHECK(play.count("shot") == 2);
	CHECK(play.get("ship").variable.at("shots") == 2.0f);
}
