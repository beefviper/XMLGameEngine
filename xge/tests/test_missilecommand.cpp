// test_missilecommand.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/descriptions/missilecommand.xml, played through a real
// xge::Engine with no window and the keys a player would press: the missiles
// making for the cities (<chase>), the base aimed at the sight (<aim>), a
// counter-missile bursting at the sight (<release>) and the burst taking the
// missiles that fly into it, a city lost, the waves (the scenes of the
// playing state, each with its own share of one pool of missiles), the
// pause drawing the game still, and the end.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
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

	Vector2f middleOf(const Object& object)
	{
		return object.position + object.size * 0.5f;
	}

	struct MissileCommand
	{
		Game game{ "descriptions/missilecommand.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		MissileCommand()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			REQUIRE(wave() == "playing.wave1");
		}

		std::string state() { return game.getCurrentState().name; }
		std::string wave() { return game.currentScene("playing"); }
		// A missile of a wave by its number, one to eight's pool: four of the salvo, two more, two most.
		static std::string missileName(const std::string& scene, int number)
		{
			const std::string part = number <= 4 ? "salvo." + std::to_string(number)
				: number <= 6 ? "more." + std::to_string(number - 4) : "most." + std::to_string(number - 6);
			return "playing." + scene + "." + part;
		}
		Object& missile(const std::string& scene, int number) { return game.getObject(missileName(scene, number)); }
		Object& sight() { return game.getObject("sight"); }
		float status(const char* name) { return game.getObject("status").variable.at(name); }

		// Puts every missile of the wave (a scene: wave1, wave2, wave3) far up
		// out of the way.
		void holdMissiles(const std::string& scene)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.inGroup("playing." + scene + ".missiles"))
				{
					object.position = { 300.0f, -100000.0f };
					object.velocity = { 0.0f, 0.0f };
					object.timers.clear();
				}
			}
		}

		// Out and not left behind in a wave gone by, of a group by its full
		// name (playing.cities).
		int inPlay(const std::string& group)
		{
			int n = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.inGroup(group) && object.isVisible && !game.isAway(object)) { ++n; }
			}
			return n;
		}
	};
}

TEST_CASE("missilecommand.xml loads, and the missiles make for the cities", "[missilecommand]")
{
	MissileCommand play;
	CHECK(play.state() == "title");
	play.start();
	CHECK(play.inPlay("playing.cities") == 6);
	CHECK(play.inPlay("playing.wave1.missiles") == 4);

	play.frames(30);
	for (const auto& object : play.game.getCurrentObjects())
	{
		if (!object.inGroup("playing.wave1.missiles") || !play.game.isShown(object)) { continue; }
		INFO(object.name);
		CHECK(object.position.y < 30.0f * 0.8f);
		CHECK(object.velocity.y > 0.0f);
		CHECK(std::hypot(object.velocity.x, object.velocity.y) == Approx(0.8f));
	}
}

TEST_CASE("a counter-missile flies to the sight and bursts there", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("wave1");

	// Up and to the left.
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::A);
	play.frames(5);
	const Vector2f aimedAt = middleOf(play.sight());
	CHECK(aimedAt.x < 240.0f);

	play.tap(KeyCode::Space);
	play.frames(1);
	REQUIRE(play.inPlay("playing.abms") == 1);
	const Object& abm = play.game.getObject("abms.1");
	CHECK(abm.velocity.x < 0.0f);
	CHECK(abm.velocity.y < 0.0f);
	CHECK(std::hypot(abm.velocity.x, abm.velocity.y) == Approx(7.0f));

	play.frames(60);
	CHECK(play.inPlay("playing.abms") == 0);
	REQUIRE(play.inPlay("playing.bursts") == 1);
	const Vector2f burst = middleOf(play.game.getObject("bursts.1"));
	// It goes off where the counter-missile first touches the sight.
	CHECK(burst.x == Approx(aimedAt.x).margin(12.0f));
	CHECK(burst.y == Approx(aimedAt.y).margin(12.0f));

	// And it is gone again a moment later.
	play.frames(45);
	CHECK(play.inPlay("playing.bursts") == 0);
}

TEST_CASE("a missile that flies into a burst scores and starts again above the sky", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("wave1");

	play.tap(KeyCode::Space);
	play.frames(40);
	REQUIRE(play.inPlay("playing.bursts") == 1);
	const Object& burst = play.game.getObject("bursts.1");

	Object& missile = play.missile("wave1", 1);
	missile.position = { middleOf(burst).x - 2.0f, burst.position.y - 30.0f };
	missile.velocity = { 0.0f, 2.0f };
	play.frames(15);
	CHECK(play.status("score") == 25);
	CHECK(play.status("spent") == 1);
	CHECK(missile.position.y < 0.0f);
}

TEST_CASE("a missile that reaches a city destroys it, and losing them all is the end", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("wave1");

	Object& city = play.game.getObject("cities.1");
	Object& missile = play.missile("wave1", 1);
	missile.position = { middleOf(city).x, city.position.y - 20.0f };
	missile.velocity = { 0.0f, 2.0f };
	play.frames(15);
	CHECK_FALSE(city.isVisible);
	CHECK(play.inPlay("playing.cities") == 5);
	CHECK(play.status("spent") == 1);
	CHECK(play.status("score") == 0);

	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.groupName == "playing.cities") { object.isVisible = false; }
	}
	play.frames(2);
	CHECK(play.state() == "theend");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.inPlay("playing.cities") == 6);
}

TEST_CASE("each wave ends when enough missiles are done with, and the third wins", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.holdMissiles("wave1");

	play.game.getObject("status").variable["spent"] = 12;
	play.frames(2);
	CHECK(play.state() == "playing");
	CHECK(play.wave() == "playing.wave2");
	CHECK(play.status("spent") == 0);
	CHECK(play.inPlay("playing.wave1.missiles") == 0);
	CHECK(play.inPlay("playing.wave2.missiles") == 6);

	play.holdMissiles("wave2");
	play.game.getObject("status").variable["spent"] = 18;
	play.frames(2);
	CHECK(play.wave() == "playing.wave3");
	CHECK(play.inPlay("playing.wave3.missiles") == 8);

	play.holdMissiles("wave3");
	play.game.getObject("status").variable["spent"] = 24;
	play.frames(2);
	CHECK(play.state() == "youwin");
	CHECK(play.game.isStill(play.game.getObject("playing.base")));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.wave() == "playing.wave1");
}

TEST_CASE("each wave's missiles fall at its own speed, in its own color, under its own label", "[missilecommand]")
{
	MissileCommand play;
	const struct { const char* scene; float speed; const char* color; const char* label; } waves[] = {
		{ "wave1", 0.8f, "color.red", "wave 1" },
		{ "wave2", 1.1f, "color.magenta", "wave 2" },
		{ "wave3", 1.4f, "color.green", "wave 3" },
	};
	for (const auto& wave : waves)
	{
		INFO(wave.scene);
		const Object& one = play.missile(wave.scene, 1);
		CHECK(one.velocity.y == Approx(wave.speed));
		CHECK(std::find(one.spriteParams.begin(), one.spriteParams.end(), wave.color) != one.spriteParams.end());
		const Object& label = play.game.getObject(std::string("playing.") + wave.scene + ".label");
		CHECK(std::find(label.spriteParams.begin(), label.spriteParams.end(), wave.label) != label.spriteParams.end());
	}
	CHECK(play.game.tryGetObject(MissileCommand::missileName("wave1", 5)) == nullptr); // the salvo only
	CHECK(play.missile("wave2", 6).isVisible);
	CHECK_FALSE(play.missile("wave2", 7).isVisible);
	CHECK(play.missile("wave3", 8).isVisible);
}

TEST_CASE("paused, the game is drawn still underneath and nothing falls", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	play.frames(10);

	Object& missile = play.missile("wave1", 1);
	const float y = missile.position.y;
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	play.frames(60);
	CHECK(missile.position.y == Approx(y));
	CHECK(play.game.isStill(missile));
	CHECK(play.game.isStill(play.sight()));
	CHECK_FALSE(play.game.isStill(play.missile("wave2", 1)));

	play.tap(KeyCode::Escape);
	play.frames(1);
	CHECK(play.state() == "playing");
	CHECK(missile.position.y > y);
}

TEST_CASE("when its city is gone a missile turns for another", "[missilecommand]")
{
	MissileCommand play;
	play.start();
	const auto chasing = play.missile("wave1", 2).timers;
	play.holdMissiles("wave1");

	// One missile above the first city, heading for it.
	Object& missile = play.missile("wave1", 2);
	const Object& first = play.game.getObject("cities.1");
	missile.timers = chasing;
	missile.position = { middleOf(first).x - 2.0f, 100.0f };
	missile.velocity = { 0.0f, 0.8f };
	play.frames(20);
	CHECK(missile.velocity.x == Approx(0.0f).margin(0.05f));

	play.game.getObject("cities.1").isVisible = false;
	play.frames(20);
	CHECK(missile.velocity.x > 0.1f);
}
