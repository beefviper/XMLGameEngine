// test_donkeykong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/donkeykong.xml, played through a real xge::Engine
// with no window and the keys a player would press: Jumpman standing on the
// girders (<land />), leaping (<leap>) and climbing (<climb>), the barrels
// Kong throws rolling down to the oil drum, a barrel costing a life, and
// saving Pauline.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	constexpr float kFloor = 660.0f;
	constexpr float kFirst = 560.0f; // the girder above the floor
	constexpr float kPerch = 80.0f;  // Pauline's

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

	struct DonkeyKong
	{
		Game game{ "games/donkeykong.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		DonkeyKong()
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

		// Starts a game, with Kong's first barrel a long way off.
		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			kong().variable["wait"] = 1000.0f;
			frames(5);
		}

		std::string state() { return game.getCurrentState().name; }
		Object& man() { return game.getObject("man"); }
		Object& kong() { return game.getObject("kong"); }
		float feet() { return man().position.y + man().size.y; }
		float variable(const char* name) { return man().variable.at(name); }

		int barrelsInPlay()
		{
			int n = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == "barrels" && object.isVisible) { ++n; }
			}
			return n;
		}

		// Puts Jumpman standing on a girder, `x` from the left.
		void standAt(float x, float girderTop)
		{
			man().position = { x, girderTop - man().size.y };
			man().velocity = { 0.0f, 0.0f };
			frames(2);
		}
	};
}

TEST_CASE("donkeykong.xml loads, and Jumpman stands on the floor", "[donkeykong]")
{
	DonkeyKong play;
	CHECK(play.state() == "mainmenu");
	play.start();

	play.frames(60);
	CHECK(play.feet() == Approx(kFloor));
	CHECK(play.man().grounded);
	CHECK(play.man().velocity.y == Approx(0.0f));
	CHECK(play.variable("lives") == 3);
}

TEST_CASE("Jumpman walks along a girder without sinking, and falls off its open end", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(600.0f, kFirst);

	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(20);
	CHECK(play.man().position.x == Approx(650.0f).margin(1.0f));
	CHECK(play.feet() == Approx(kFirst));

	// The girder ends at 720: past it he drops to the floor, still walking.
	play.frames(60);
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(60);
	CHECK(play.feet() == Approx(kFloor));
	CHECK(play.man().grounded);
}

TEST_CASE("a leap rises about its height and lands, and only from the ground", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, kFloor);

	play.tap(KeyCode::Space);
	float highest = play.feet();
	int airborne = 0;
	for (int i = 0; i < 60; ++i)
	{
		play.frames(1);
		highest = std::min(highest, play.feet());
		if (play.feet() < kFloor) { ++airborne; }
		// A second press in the air does nothing.
		if (i == 5) { play.tap(KeyCode::Space); }
	}
	CHECK(kFloor - highest == Approx(44.0f).margin(4.0f));
	CHECK(airborne > 20);
	CHECK(airborne < 35);
	CHECK(play.feet() == Approx(kFloor));
	CHECK_FALSE(play.man().leaping);
}

TEST_CASE("the way across a leap is fixed at the take-off", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, kFloor);

	// Walking right, he leaps and lets go: he keeps going right in the air...
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(2);
	play.tap(KeyCode::Space);
	play.frames(2);
	play.engine.handleKeyReleased(KeyCode::D);
	const float letGo = play.man().position.x;
	play.frames(10);
	CHECK(play.man().position.x == Approx(letGo + 25.0f).margin(0.5f));

	// ...and turning round does not steer him either, until he lands.
	play.engine.handleKeyPressed(KeyCode::A);
	const float turned = play.man().position.x;
	play.frames(5);
	CHECK(play.man().position.x > turned);
	play.frames(40);
	CHECK(play.feet() == Approx(kFloor));
	const float landed = play.man().position.x;
	play.frames(10);
	CHECK(play.man().position.x == Approx(landed - 25.0f).margin(0.5f));
	play.engine.handleKeyReleased(KeyCode::A);
}

TEST_CASE("a ladder takes him up to the next girder and back down, and nothing else does", "[donkeykong]")
{
	DonkeyKong play;
	play.start();

	// Away from any ladder, up does nothing.
	play.standAt(300.0f, kFloor);
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(30);
	CHECK(play.feet() == Approx(kFloor));
	play.engine.handleKeyReleased(KeyCode::W);

	// At the first ladder (x 560 to 580), up climbs it, lined up with it.
	play.standAt(554.0f, kFloor);
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(10);
	CHECK(play.man().climbing);
	CHECK(play.man().position.x == Approx(560.0f));
	CHECK(play.feet() == Approx(kFloor - 20.0f));

	// Let go halfway: he stays there, held by nothing.
	play.engine.handleKeyReleased(KeyCode::W);
	play.frames(30);
	CHECK(play.feet() == Approx(kFloor - 20.0f));

	// Walking does not take him off it.
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(10);
	CHECK(play.man().position.x == Approx(560.0f));
	play.engine.handleKeyReleased(KeyCode::A);

	// At the top he steps off onto the girder and stands on it.
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(60);
	CHECK_FALSE(play.man().climbing);
	play.engine.handleKeyReleased(KeyCode::W);
	play.frames(10);
	CHECK(play.feet() == Approx(kFirst));
	CHECK(play.man().grounded);

	// And down again, through the girder he stood on.
	play.engine.handleKeyPressed(KeyCode::S);
	play.frames(80);
	play.engine.handleKeyReleased(KeyCode::S);
	play.frames(10);
	CHECK(play.feet() == Approx(kFloor));
	CHECK(play.man().grounded);
}

TEST_CASE("Kong's barrel rolls down every girder to the oil drum", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	// Jumpman out of the way, on Pauline's girder clear of her.
	play.standAt(420.0f, kPerch);
	play.kong().timers.at(0).framesLeft = 1;
	play.frames(2);
	REQUIRE(play.barrelsInPlay() >= 1);

	Object& barrel = play.game.getObject("barrels.1");
	REQUIRE(barrel.isVisible);
	float lowest = 0.0f;
	bool wentLeft = false;
	int frame = 0;
	for (; frame < 60 * 60 && barrel.isVisible; ++frame)
	{
		play.frames(1);
		lowest = std::max(lowest, barrel.position.y + barrel.size.y);
		if (barrel.velocity.x < 0.0f) { wentLeft = true; }
	}
	CHECK_FALSE(barrel.isVisible);
	CHECK(lowest == Approx(kFloor).margin(1.0f));
	CHECK(wentLeft);
	CHECK(barrel.position.x < 40.0f); // at the oil drum
	CHECK(play.variable("lives") == 3);
}

TEST_CASE("a barrel that reaches Jumpman costs a life and clears the girders; leaping over it does not", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, kFloor);

	// A barrel rolling left along the floor, towards him.
	Object& barrel = play.game.getObject("barrels.1");
	barrel.position = { 450.0f, kFloor - barrel.size.y };
	barrel.velocity = { -2.5f, 0.0f };
	barrel.isVisible = true;
	barrel.collisionData.enabled = true;

	// Leap as it comes close: it passes under him.
	while (barrel.position.x > play.man().position.x + play.man().size.x + 8.0f) { play.frames(1); }
	play.tap(KeyCode::Space);
	play.frames(60);
	CHECK(play.variable("lives") == 3);
	CHECK(barrel.position.x < 300.0f);

	// The next one he stands still for.
	barrel.position = { 450.0f, kFloor - barrel.size.y };
	barrel.velocity = { -2.5f, 0.0f };
	play.frames(80);
	CHECK(play.variable("lives") == 2);
	CHECK(play.barrelsInPlay() == 0);
	CHECK(play.man().position.x == Approx(play.man().positionOriginal.x));
}

TEST_CASE("saving Pauline scores, starts the climb again with Kong quicker, and three saves win", "[donkeykong]")
{
	DonkeyKong play;
	play.start();

	for (int save = 1; save <= 3; ++save)
	{
		play.kong().variable["wait"] = 1000.0f + static_cast<float>(save);
		play.standAt(340.0f, kPerch);
		play.engine.handleKeyPressed(KeyCode::A);
		for (int i = 0; i < 30 && play.variable("rescues") < static_cast<float>(save); ++i) { play.frames(1); }
		play.engine.handleKeyReleased(KeyCode::A);
		CHECK(play.variable("rescues") == static_cast<float>(save));
		CHECK(play.variable("score") == 1000.0f * static_cast<float>(save));
		CHECK(play.kong().variable.at("wait") == Approx(1000.0f + static_cast<float>(save) - 0.6f));
		if (save < 3)
		{
			CHECK(play.man().position.x == Approx(play.man().positionOriginal.x));
		}
	}
	play.frames(1);
	CHECK(play.state() == "youwin");

	// Space plays again from the start.
	play.tap(KeyCode::Space);
	CHECK(play.state() == "mainmenu");
	CHECK(play.variable("rescues") == 0);
	CHECK(play.kong().variable.at("wait") == Approx(2.8f));
}
