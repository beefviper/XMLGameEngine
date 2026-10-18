// test_refused.cpp
// XML Game Engine
// author: beefviper
// date: Oct 18, 2026
//
// Catch2 tests for a press that cannot be done: an action stops at a verb
// that is refused (a <fire> with every shot in flight, a hop in the air),
// so the sound and the look written after it are not given either.

#include "played.h"

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
	CHECK(play.get("bullet").collisionData.enabled);
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
	CHECK(play.get("qbert").lookName() == "downright");
	CHECK(play.game.takeSoundRequests().size() == 1);

	play.frames(4);
	REQUIRE(play.get("qbert").isAirborne());
	(void)play.game.takeSoundRequests();
	play.tap(KeyCode::A);
	CHECK(play.get("qbert").lookName() == "downright");
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
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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
