// test_atlantis.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for games/descriptions/atlantis.xml, played frame by frame by a
// real xge::Game with no window. Gorgon ships cross the sky in four lanes,
// launched by a timer of the wave named "launch" and dropping a lane each
// pass; from the lowest they fire death rays at the city. Three posts fire
// at their own angles. The waves are scenes: the second is the first
// (from=), and the third the second with a "launch" and a "quota" of its own
// in place of the copied ones.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	constexpr float kWindowWidth = 640.0f;
	constexpr int kFramerate = 60;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	// The path of a wave's piece: playing.wave2.lane1.
	std::string of(const std::string& wave, const std::string& piece) { return "playing." + wave + "." + piece; }

	struct City
	{
		Game game{ "descriptions/atlantis.xml" };
		CommandExecutor executor{ game };

		explicit City(const std::string& wave = "wave1")
		{
			measure(game);
			game.setCurrentState(0);
			game.pushState("playing");
			if (wave != "wave1") { game.enterScene("playing." + wave); }
		}

		std::string state() { return game.getCurrentState().name; }
		std::string wave() { return game.currentScene("playing"); }
		float score() { return game.getObject("status").variable["score"]; }
		float downed() { return game.getObject("tally").variable["downed"]; }

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		void fire(const char* post) { executor.executeInput(Command{ CmdTriggerAction{ post, "fire" } }, true); }
		void reveal(const std::string& group, int count = 1) { executor.executeInput(Command{ CmdReveal{ group, count } }, true); }

		// What of a group (a lane, a pool) is in play.
		std::vector<Object*> inPlay(const std::string& group)
		{
			std::vector<Object*> found;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.inGroup(group) && object.isVisible) { found.push_back(&object); }
			}
			return found;
		}

		int buildings()
		{
			int count = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "building" && object.isVisible) { ++count; }
			}
			return count;
		}
	};

	// Just enough of a Window for Engine's constructor.
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};
}

TEST_CASE("atlantis.xml loads a city of six, three posts at their angles, and four lanes of hidden ships in each of three waves", "[atlantis]")
{
	City city;
	CHECK(city.wave() == "playing.wave1");
	CHECK(city.buildings() == 6);

	CHECK(city.game.getObject("leftpost").heading == Approx(45));
	CHECK(city.game.getObject("middlepost").heading == Approx(0));
	CHECK(city.game.getObject("rightpost").heading == Approx(315));

	for (const char* wave : { "wave1", "wave2", "wave3" })
	{
		int ships = 0;
		for (const auto& object : city.game.getCurrentObjects())
		{
			if (!object.inGroup(of(wave, "gorgons"))) { continue; }
			++ships;
			CHECK_FALSE(object.isVisible);
		}
		CHECK(ships == 12);
	}
	CHECK(city.game.getObject(of("wave1", "lane1.1")).velocity.x == Approx(2));
	CHECK(city.game.getObject(of("wave2", "lane1.1")).velocity.x == Approx(2.8));
	CHECK(city.game.getObject(of("wave3", "lane1.1")).velocity.x == Approx(3.5));
}

TEST_CASE("the second wave copies the first's named launch and quota; the third puts its own in their places", "[atlantis][scenes]")
{
	City city;
	const State& playing = city.game.getStates()[1];
	REQUIRE(playing.name == "playing");
	REQUIRE(playing.scenes.size() == 3);

	for (const Scene& scene : playing.scenes)
	{
		CHECK(scene.timers.size() == 1);
		CHECK(scene.conditions.size() == 1);
	}
	// launch: one <reveal>, then two in the third wave
	CHECK(playing.scenes[0].timers[0].commands.size() == 1);
	CHECK(playing.scenes[1].timers[0].commands.size() == 1);
	CHECK(playing.scenes[2].timers[0].commands.size() == 2);
	// quota: on to the next wave (play, three resets, enter), then a win (play, push)
	CHECK(playing.scenes[1].conditions[0].commands.size() == 5);
	CHECK(playing.scenes[2].conditions[0].commands.size() == 2);
	CHECK(playing.scenes[1].ways.at("next") == "playing.wave3");
}

TEST_CASE("the launch timer sends a ship across the top lane, and a ship at the right edge comes back a lane lower", "[atlantis]")
{
	City city;
	city.frames(3 * kFramerate - 10);
	CHECK(city.inPlay(of("wave1", "gorgons")).empty());
	city.frames(20);

	const auto launched = city.inPlay(of("wave1", "lane1"));
	REQUIRE(launched.size() == 1);
	Object& ship = *launched[0];
	CHECK(ship.position.y == Approx(50));
	CHECK(ship.velocity.x == Approx(2));

	ship.position.x = kWindowWidth - ship.size.x - 1.0f;
	city.frames(2);
	CHECK_FALSE(ship.isVisible);

	const auto lower = city.inPlay(of("wave1", "lane2"));
	REQUIRE(lower.size() == 1);
	CHECK(lower[0]->position.y == Approx(105));
	CHECK(lower[0]->position.x < 0.0f);
}

TEST_CASE("in the third wave the launch fills the top lane and the third at once", "[atlantis][scenes]")
{
	City city("wave3");
	city.frames(static_cast<int>(2.2f * kFramerate) + 10);
	CHECK(city.inPlay(of("wave3", "lane1")).size() == 1);
	CHECK(city.inPlay(of("wave3", "lane3")).size() == 1);
	CHECK(city.inPlay(of("wave3", "lane2")).empty());
}

TEST_CASE("the lowest lane goes round and round and fires death rays that take the city a building at a time", "[atlantis]")
{
	City city;
	city.reveal(of("wave1", "lane4"));
	const auto low = city.inPlay(of("wave1", "lane4"));
	REQUIRE(low.size() == 1);
	Object& ship = *low[0];
	CHECK(ship.position.y == Approx(215));

	// Round once off the right edge, still in the lowest lane.
	ship.position.x = kWindowWidth + 1.0f;
	city.frames(2);
	CHECK(ship.isVisible);
	CHECK(ship.position.x < 100.0f);
	CHECK(ship.position.y == Approx(215));

	// A ray from under it, falling.
	int frames = 0;
	while (city.inPlay("playing.rays").empty() && frames < 5 * kFramerate) { city.frames(1); ++frames; }
	const auto rays = city.inPlay("playing.rays");
	REQUIRE(rays.size() == 1);
	CHECK(rays[0]->velocity.y == Approx(5));
	CHECK(rays[0]->position.y >= ship.position.y + ship.size.y - 1.0f);

	// One over a dome takes it.
	Object& dome = city.game.getObject("playing.domes.1");
	rays[0]->position = { dome.position.x + 10.0f, dome.position.y - rays[0]->size.y - 2.0f };
	city.frames(2);
	CHECK_FALSE(dome.isVisible);
	CHECK(city.buildings() == 5);
}

TEST_CASE("each post fires along its own heading, and a hit ship scores and counts", "[atlantis]")
{
	City city;
	city.fire("leftpost");
	city.fire("middlepost");
	city.fire("rightpost");
	const auto left = city.inPlay("playing.leftshells");
	const auto middle = city.inPlay("playing.middleshells");
	const auto right = city.inPlay("playing.rightshells");
	REQUIRE(left.size() == 1);
	REQUIRE(middle.size() == 1);
	REQUIRE(right.size() == 1);
	CHECK(left[0]->velocity.x > 4.0f);
	CHECK(left[0]->velocity.y < -4.0f);
	CHECK(middle[0]->velocity.x == Approx(0).margin(0.01));
	CHECK(middle[0]->velocity.y == Approx(-7));
	CHECK(right[0]->velocity.x < -4.0f);
	CHECK(right[0]->velocity.y < -4.0f);

	// A ship over the middle shell.
	city.reveal(of("wave1", "lane3"));
	Object& ship = *city.inPlay(of("wave1", "lane3"))[0];
	ship.position = { middle[0]->position.x - ship.size.x / 2.0f, middle[0]->position.y - ship.size.y - 4.0f };
	city.frames(2);
	CHECK_FALSE(ship.isVisible);
	CHECK_FALSE(middle[0]->isVisible);
	CHECK(city.score() == 100);
	CHECK(city.downed() == 1);
}

TEST_CASE("the quota moves the game on from wave to wave, with the tally and the rays put back, and the third quota wins", "[atlantis][scenes]")
{
	City city;
	city.game.getObject("tally").variable["downed"] = 12;
	city.frames(1);
	CHECK(city.wave() == "playing.wave2");
	CHECK(city.downed() == 0);

	city.game.getObject("tally").variable["downed"] = 15;
	city.frames(1);
	CHECK(city.wave() == "playing.wave2");
	city.game.getObject("tally").variable["downed"] = 16;
	city.frames(1);
	CHECK(city.wave() == "playing.wave3");
	CHECK(city.state() == "playing");

	city.game.getObject("tally").variable["downed"] = 20;
	city.frames(1);
	CHECK(city.state() == "youwin");
}

TEST_CASE("the city gone is game over, and space plays again with everything back", "[atlantis]")
{
	Game game{ "descriptions/atlantis.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");

	for (auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "building") { object.isVisible = false; }
	}
	engine.step();
	REQUIRE(game.getCurrentState().name == "gameover");
	CHECK(game.isStill(game.getObject("leftpost")));

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "title");
	int buildings = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "building" && object.isVisible) { ++buildings; }
	}
	CHECK(buildings == 6);
}

TEST_CASE("the keys fire the posts, and space pauses over the city drawn still", "[atlantis][engine_input]")
{
	Game game{ "descriptions/atlantis.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");

	engine.handleKeyPressed(KeyCode::A);
	engine.handleKeyReleased(KeyCode::A);
	engine.handleKeyPressed(KeyCode::Up);
	engine.handleKeyReleased(KeyCode::Up);
	engine.handleKeyPressed(KeyCode::D);
	engine.handleKeyReleased(KeyCode::D);
	int shells = 0;
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "shell" && object.isVisible) { ++shells; }
	}
	CHECK(shells == 3);

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "paused");
	CHECK(game.isStill(game.getObject("middlepost")));
	CHECK(game.isStill(game.getObject("playing.domes.1")));
}
