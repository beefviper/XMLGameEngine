// test_donkeykong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/descriptions/donkeykong.xml, played through a real xge::Engine
// with no window and the keys a player would press: Jumpman standing on the
// girders (<land />), which slope (<slope>), leaping (<leap>) and climbing
// (<climb>) ladders of rungs, the barrels Kong throws rolling down to the oil
// drum and now and then down a ladder, the fireballs the oil lets out, a
// barrel costing a life and one jumped scoring, the hammer smashing them, the
// bonus counting down, and saving Pauline.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"

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
	constexpr float kPerch = 85.0f;  // Pauline's, flat

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
		Game game{ "descriptions/donkeykong.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		DonkeyKong()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
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
			kong().timers.at(0).framesLeft = 60000;
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
				if (object.groupName == "playing.barrel" && object.isVisible) { ++n; }
			}
			return n;
		}

		// A barrel, as if Kong had just thrown one, in the game at once.
		Object& barrel()
		{
			const std::string name = game.spawn("barrel")->name;
			game.settleSpawns();
			return game.getObject(name);
		}

		// The top of a girder (floor, first, ... top, perch) at `x` across:
		// the top of its piece there, since each is a little lower or higher
		// than the one before.
		float top(const std::string& girder, float x)
		{
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == "playing." + girder && x >= object.position.x && x < object.position.x + object.size.x)
				{
					return object.position.y;
				}
			}
			FAIL("no piece of " << girder << " at " << x);
			return 0.0f;
		}

		float middle() { return man().position.x + man().size.x / 2.0f; }
		float topUnderMan(const std::string& girder) { return top(girder, middle()); }

		// Puts Jumpman standing on a girder, `x` from the left.
		void standAt(float x, const std::string& girder)
		{
			const float at = top(girder, x + man().size.x / 2.0f);
			man().position = { x, at - man().size.y };
			man().velocity = { 0.0f, 0.0f };
			frames(2);
		}

		// A barrel on a girder at `x`, rolling at `speed`.
		Object& barrelAt(float x, const std::string& girder, float speed)
		{
			Object& one = barrel();
			one.position = { x, top(girder, x + one.size.x / 2.0f) - one.size.y };
			one.velocity = { speed, 0.0f };
			return one;
		}
	};
}

TEST_CASE("donkeykong.xml loads, and Jumpman stands on the floor", "[donkeykong]")
{
	DonkeyKong play;
	CHECK(play.state() == "mainmenu");
	play.start();

	play.frames(60);
	CHECK(play.feet() == Approx(play.topUnderMan("floor")).margin(1.0f));
	CHECK(play.man().grounded);
	CHECK(play.man().velocity.y == Approx(0.0f));
	CHECK(play.variable("lives") == 3);
}

TEST_CASE("Jumpman walks along a girder without sinking, and falls off its open end", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(600.0f, "first");

	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(20);
	CHECK(play.man().position.x == Approx(650.0f).margin(1.0f));
	CHECK(play.feet() == Approx(play.topUnderMan("first")).margin(1.0f));

	// The girder ends at 720: past it he drops to the floor, still walking.
	play.frames(60);
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(60);
	CHECK(play.feet() == Approx(play.topUnderMan("floor")).margin(1.0f));
	CHECK(play.man().grounded);
}

TEST_CASE("a leap rises about its height and lands, and only from the ground", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, "floor");

	const float floor = play.feet();
	play.tap(KeyCode::Space);
	float highest = play.feet();
	int airborne = 0;
	for (int i = 0; i < 60; ++i)
	{
		play.frames(1);
		highest = std::min(highest, play.feet());
		if (play.feet() < floor - 0.5f) { ++airborne; }
		// A second press in the air does nothing.
		if (i == 5) { play.tap(KeyCode::Space); }
	}
	CHECK(floor - highest == Approx(44.0f).margin(4.0f));
	CHECK(airborne > 20);
	CHECK(airborne < 35);
	CHECK(play.feet() == Approx(floor).margin(1.0f));
	CHECK_FALSE(play.man().leaping);
}

TEST_CASE("the way across a leap is fixed at the take-off", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, "floor");

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
	CHECK(play.feet() == Approx(play.topUnderMan("floor")).margin(1.0f));
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
	play.standAt(300.0f, "floor");
	const float floor = play.feet();
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(30);
	CHECK(play.feet() == Approx(floor));
	play.engine.handleKeyReleased(KeyCode::W);

	// At the first ladder (x 510 to 530, eleven rungs), up climbs it, his
	// middle lined up with its middle.
	const float onLadder = 520.0f - play.man().size.x / 2.0f;
	play.standAt(504.0f, "floor");
	const float foot = play.feet();
	REQUIRE(foot == Approx(652.0f));
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(10);
	CHECK(play.man().climbing);
	CHECK(play.man().position.x == Approx(onLadder));
	CHECK(play.feet() == Approx(foot - 20.0f));

	// Let go partway: he stays there, held by nothing.
	play.engine.handleKeyReleased(KeyCode::W);
	play.frames(30);
	CHECK(play.feet() == Approx(foot - 20.0f));

	// Walking does not take him off it.
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(10);
	CHECK(play.man().position.x == Approx(onLadder));
	play.engine.handleKeyReleased(KeyCode::A);

	// At the top he steps off onto the girder and stands on it.
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(60);
	CHECK_FALSE(play.man().climbing);
	play.engine.handleKeyReleased(KeyCode::W);
	play.frames(10);
	CHECK(play.feet() == Approx(564.0f).margin(0.5f));
	CHECK(play.man().grounded);

	// And down again, through the girder he stood on.
	play.engine.handleKeyPressed(KeyCode::S);
	play.frames(80);
	play.engine.handleKeyReleased(KeyCode::S);
	play.frames(10);
	CHECK(play.feet() == Approx(foot).margin(0.5f));
	CHECK(play.man().grounded);
}

TEST_CASE("Kong's barrel rolls down every girder to the oil drum", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	// Jumpman out of the way, on Pauline's girder clear of her.
	play.standAt(420.0f, "perch");
	play.kong().timers.at(0).framesLeft = 1;
	play.frames(2);
	REQUIRE(play.barrelsInPlay() >= 1);

	// Followed by its name while it lasts (it is despawned at the end).
	REQUIRE(play.game.tryGetObject("barrel.1"));
	float lowest = 0.0f;
	float lastX = 0.0f;
	bool wentLeft = false;
	for (int frame = 0; frame < 60 * 60; ++frame)
	{
		play.frames(1);
		const Object* barrel = play.game.tryGetObject("barrel.1");
		if (!barrel) { break; }
		lowest = std::max(lowest, barrel->position.y + barrel->size.y);
		lastX = barrel->position.x;
		if (barrel->velocity.x < 0.0f) { wentLeft = true; }
	}
	CHECK(play.game.tryGetObject("barrel.1") == nullptr);
	CHECK(lowest == Approx(660.0f).margin(1.0f));
	CHECK(wentLeft);
	CHECK(lastX < 40.0f); // at the oil drum
	CHECK(play.variable("lives") == 3);
}

TEST_CASE("a barrel that reaches Jumpman costs a life and clears the girders; leaping over it does not", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, "floor");

	// A barrel rolling left along the floor, towards him.
	Object& barrel = play.barrelAt(450.0f, "floor", -2.5f);

	// Leap as it comes close: it passes under him, and that is 100 when he lands.
	while (barrel.position.x > play.man().position.x + play.man().size.x + 20.0f) { play.frames(1); }
	play.tap(KeyCode::Space);
	play.frames(60);
	CHECK(play.variable("lives") == 3);
	CHECK(barrel.position.x < 300.0f);
	CHECK(play.variable("score") == 100);

	// The next one he stands still for.
	barrel.position = { 450.0f, play.top("floor", 460.0f) - barrel.size.y };
	barrel.velocity = { -2.5f, 0.0f };
	play.frames(80);
	CHECK(play.variable("lives") == 2);
	CHECK(play.barrelsInPlay() == 0);
	CHECK(play.man().position.x == Approx(play.man().positionOriginal.x));
}

TEST_CASE("saving Pauline scores the bonus, starts the climb again with Kong quicker, and three saves win", "[donkeykong]")
{
	DonkeyKong play;
	play.start();

	for (int save = 1; save <= 3; ++save)
	{
		play.kong().variable["wait"] = 1000.0f + static_cast<float>(save);
		play.standAt(340.0f, "perch");
		play.engine.handleKeyPressed(KeyCode::A);
		for (int i = 0; i < 30 && play.variable("rescues") < static_cast<float>(save); ++i) { play.frames(1); }
		play.engine.handleKeyReleased(KeyCode::A);
		CHECK(play.variable("rescues") == static_cast<float>(save));
		CHECK(play.variable("score") == 5000.0f * static_cast<float>(save));
		CHECK(play.variable("bonus") == 5000.0f);
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

TEST_CASE("paused, the girders are drawn still and a rolling barrel stops where it is", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(420.0f, "perch");
	play.kong().timers.at(0).framesLeft = 1;
	play.frames(10);
	REQUIRE(play.barrelsInPlay() >= 1);
	Object& barrel = play.game.getObject("barrel.1");
	const Vector2f barrelAt = barrel.position;

	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	play.frames(60);
	CHECK(barrel.position == barrelAt);
	CHECK(play.game.isStill(barrel));
	CHECK(play.game.isStill(play.man()));
	CHECK(play.game.isStill(play.kong()));
	CHECK(play.game.isStill(play.game.getObject("floor.1.1")));
	CHECK_FALSE(play.game.isShown(barrel));
	CHECK(play.game.isShown(play.game.getObject("paused")));
	CHECK_FALSE(play.game.isStill(play.game.getObject("barrel"))); // the one written, never thrown itself

	play.tap(KeyCode::P);
	REQUIRE(play.state() == "playing");
	play.frames(1);
	CHECK(barrel.position != barrelAt);
}

TEST_CASE("each state owns what it draws, and game over draws the girders still under its message", "[donkeykong]")
{
	DonkeyKong play;
	CHECK(play.man().ownerState == "playing");
	CHECK(play.game.getObject("title").ownerState == "mainmenu");
	CHECK(play.game.getObject("again").ownerState.empty());
	CHECK(play.game.isShown(play.game.getObject("title")));
	CHECK_FALSE(play.game.isShown(play.man()));

	play.start();
	play.man().variable["lives"] = 0;
	play.frames(1);
	REQUIRE(play.state() == "gameover");
	CHECK(play.game.isStill(play.man()));
	CHECK(play.game.isStill(play.game.getObject("score")));
	CHECK(play.game.isShown(play.game.getObject("gameover.message")));
	CHECK(play.game.isShown(play.game.getObject("again")));
	CHECK_FALSE(play.game.isShown(play.game.getObject("youwin.message")));
}

TEST_CASE("with the hammer a barrel is smashed for points, and the hammer wears off", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	Object& hammer = play.game.getObject("hammer");
	// It hangs above the girder: he leaps to take it.
	play.standAt(hammer.position.x, "third");
	play.tap(KeyCode::Space);
	play.frames(40);
	REQUIRE(play.variable("hammer") == 1);
	CHECK_FALSE(hammer.isVisible);
	CHECK(play.man().lookName() == "hammer");

	Object& barrel = play.barrelAt(play.man().position.x + 60.0f, "third", -2.5f);
	play.frames(40);
	CHECK_FALSE(barrel.isVisible);
	CHECK(play.variable("score") == 300);
	CHECK(play.variable("lives") == 3);

	play.frames(60 * 9);
	CHECK(play.variable("hammer") == 0);
	CHECK(play.man().lookName() == "plain");
}

TEST_CASE("the bonus counts down, and running out costs a life", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.frames(2 * 60 + 5);
	CHECK(play.variable("bonus") == 4900);

	play.man().variable["bonus"] = 100.0f;
	play.frames(2 * 60 + 5);
	CHECK(play.variable("lives") == 2);
	CHECK(play.variable("bonus") == Approx(5000.0f).margin(100.0f));
}

TEST_CASE("the girders slope half a pixel a piece, down to their open ends", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	CHECK(play.game.getObject("first.1.1").position.y == Approx(548.0f));
	CHECK(play.game.getObject("first.2.1").position.y == Approx(548.5f));
	CHECK(play.game.getObject("first.45.1").position.y == Approx(570.0f));
	CHECK(play.game.getObject("second.1.1").position.y == Approx(470.0f));
	CHECK(play.game.getObject("second.44.1").position.y == Approx(448.5f));
	CHECK(play.game.getObject("perch.13.1").position.y == Approx(85.0f));

	// Walking up the second girder he climbs with it, a step at a time.
	play.standAt(200.0f, "second");
	const float low = play.feet();
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(64);
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(2);
	CHECK(play.man().grounded);
	CHECK(play.feet() == Approx(play.topUnderMan("second")).margin(1.0f));
	CHECK(play.feet() < low - 4.0f);
}

TEST_CASE("a broken ladder takes him partway and holds him at its top, and back down to the girder", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(254.0f, "floor");
	const float foot = play.feet();
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(5);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.man().climbing);
	CHECK(play.feet() == Approx(foot - 10.0f));

	// Its two rungs end well short of the girder above, with nothing to
	// stand on: held up, he stays at the top of it, as in the arcade.
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(30);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.man().climbing);
	const float held = play.feet();
	CHECK(held == Approx(foot - 16.0f).margin(0.5f));
	play.frames(30);
	CHECK(play.man().climbing);
	CHECK(play.feet() == Approx(held));

	// Down takes him back to the girder, and off.
	play.engine.handleKeyPressed(KeyCode::S);
	play.frames(30);
	play.engine.handleKeyReleased(KeyCode::S);
	play.frames(5);
	CHECK_FALSE(play.man().climbing);
	CHECK(play.feet() == Approx(foot).margin(0.5f));
	CHECK(play.man().grounded);
}

TEST_CASE("an unlucky barrel goes down a ladder and rolls on the other way; a lucky one rolls past", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(420.0f, "perch");

	// On Kong's girder, rolling right towards the ladder at 568.
	Object& unlucky = play.barrelAt(500.0f, "top", 2.5f);
	unlucky.variable["luck"] = 0.0f;
	bool climbed = false;
	for (int i = 0; i < 120 && !climbed; ++i)
	{
		play.frames(1);
		climbed = unlucky.climbing;
	}
	REQUIRE(climbed);
	CHECK(unlucky.lookName() == "down");
	CHECK(unlucky.position.x + unlucky.size.x / 2.0f == Approx(568.0f));
	for (int i = 0; i < 120 && unlucky.climbing; ++i) { play.frames(1); }
	play.frames(5);
	CHECK(unlucky.lookName() == "rolling");
	CHECK(unlucky.velocity.x == Approx(-2.5f));
	CHECK(unlucky.position.y + unlucky.size.y == Approx(play.top("fourth", unlucky.position.x + unlucky.size.x / 2.0f)).margin(1.0f));
	CHECK(unlucky.activeClimb[1] == 0.0f);

	// One with luck stays on its girder past the same ladder.
	Object& lucky = play.barrelAt(500.0f, "top", 2.5f);
	lucky.variable["luck"] = 0.9f;
	bool anyClimb = false;
	for (int i = 0; i < 50; ++i)
	{
		play.frames(1);
		anyClimb = anyClimb || lucky.climbing;
	}
	CHECK_FALSE(anyClimb);
	CHECK(lucky.position.x > 580.0f);
	CHECK(lucky.lookName() == "rolling");
}

TEST_CASE("a barrel in the oil drum lets out a fireball, which costs a life and climbs ladders", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(420.0f, "perch");
	Object& barrel = play.barrelAt(60.0f, "floor", -2.5f);
	barrel.variable["luck"] = 0.9f;
	const std::string barrelName = barrel.name;
	play.frames(30);
	CHECK_FALSE(play.game.tryGetObject(barrelName));
	REQUIRE(play.game.tryGetObject("fireball.1"));
	Object& fire = play.game.getObject("fireball.1");
	CHECK(fire.position.x < 60.0f);

	// One sent at Jumpman on the floor.
	play.standAt(300.0f, "floor");
	fire.variable["luck"] = 0.9f;
	fire.position = { 276.0f, play.feet() - fire.size.y };
	fire.velocity = { 1.0f, 0.0f };
	play.frames(30);
	CHECK(play.variable("lives") == 2);
	CHECK_FALSE(play.game.tryGetObject("fireball.1"));

	// One at the foot of a ladder, with no luck, climbs it to the girder above.
	play.standAt(420.0f, "perch");
	const std::string climberName = play.game.spawn("fireball")->name;
	play.game.settleSpawns();
	Object& climber = play.game.getObject(climberName);
	climber.variable["luck"] = 0.0f;
	climber.position = { 480.0f, play.top("floor", 490.0f) - climber.size.y };
	climber.velocity = { 1.0f, 0.0f };
	bool climbed = false;
	for (int i = 0; i < 300 && !(climbed && !climber.climbing); ++i)
	{
		play.frames(1);
		climbed = climbed || climber.climbing;
	}
	CHECK(climbed);
	play.frames(10);
	CHECK(climber.position.y + climber.size.y == Approx(play.top("first", climber.position.x + climber.size.x / 2.0f)).margin(1.0f));
	CHECK(climber.lookName() == "burning");
	CHECK(climber.velocity.x == Approx(1.0f));
}

TEST_CASE("with the hammer a fireball is smashed for 500", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(300.0f, "floor");
	play.game.setVariable("man", "hammer", 1.0f);
	const std::string fireName = play.game.spawn("fireball")->name;
	play.game.settleSpawns();
	Object& fire = play.game.getObject(fireName);
	fire.variable["luck"] = 0.9f;
	fire.position = { 276.0f, play.feet() - fire.size.y };
	fire.velocity = { 1.0f, 0.0f };
	play.frames(30);
	CHECK_FALSE(fire.isVisible);
	CHECK(play.variable("score") == 500);
	CHECK(play.variable("lives") == 3);
}

TEST_CASE("a barrel that meets Jumpman on a ladder costs a life, coming down it or rolling past its foot", "[donkeykong]")
{
	// A climb moves him by his velocity, so a touch is found as for any
	// move: before, climbing set his place with no velocity, and a barrel
	// on the same ladder, also climbing, was never even looked at.
	{
		DonkeyKong play;
		play.start();

		// Halfway up the first ladder (x 510 to 530).
		play.standAt(504.0f, "floor");
		play.engine.handleKeyPressed(KeyCode::W);
		play.frames(15);
		play.engine.handleKeyReleased(KeyCode::W);
		REQUIRE(play.man().climbing);

		// A barrel at the top of it, unlucky, so it comes down.
		Object& barrel = play.barrelAt(520.0f - 12.0f, "first", 1.0f);
		play.game.setVariable(barrel.name, "luck", 0.0f);
		play.frames(60);
		CHECK(play.variable("lives") == 2);
	}
	{
		DonkeyKong play;
		play.start();

		// Just up the ladder, a barrel rolling along the floor under him.
		play.standAt(504.0f, "floor");
		play.engine.handleKeyPressed(KeyCode::W);
		play.frames(3);
		play.engine.handleKeyReleased(KeyCode::W);
		REQUIRE(play.man().climbing);
		play.barrelAt(600.0f, "floor", -2.5f);
		play.frames(60);
		CHECK(play.variable("lives") == 2);
	}
}

TEST_CASE("a barrel going down a ladder passes the girders on the way and rolls on only at the foot", "[donkeykong]")
{
	DonkeyKong play;
	play.start();
	play.standAt(100.0f, "floor");

	Object& barrel = play.barrelAt(520.0f - 12.0f, "first", 1.0f);
	play.game.setVariable(barrel.name, "luck", 0.0f);
	play.frames(20);
	CHECK(barrel.climbing);
	CHECK(barrel.lookName() == "down");
	CHECK(barrel.velocity.x == 0.0f);

	for (int frame = 0; frame < 120 && barrel.velocity.x == 0.0f; ++frame) { play.frames(1); }
	CHECK_FALSE(barrel.climbing);
	CHECK(barrel.velocity.x < 0.0f);
	CHECK(barrel.position.y + barrel.size.y == Approx(play.top("floor", barrel.position.x + barrel.size.x / 2.0f)).margin(1.0f));
}
