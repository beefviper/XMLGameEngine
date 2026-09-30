// test_frogger.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/frogger.xml, played frame by frame by a real
// xge::Game with no window: the verbs it needed (hop, carry, wrap, dec, the
// atmost condition, collision rules with unless=, and inc/dec/reset in a rule
// about another object) and the game they make up.
//
// Window::init() normally measures each object's size once a backend exists;
// a Game built on its own has {0, 0}, so measure() gives every shape the size
// its sprite implies, which is what the backends would measure too.

#include "command_executor.h"
#include "engine.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>

using namespace xge;

namespace
{
	constexpr float kCell = 48.0f;
	constexpr float kInset = 6.0f;
	constexpr float kWindowWidth = 624.0f;

	// The frog's own place on a grid of cells.
	Vector2f cellPosition(float column, float row)
	{
		return { column * kCell + kInset, row * kCell + kInset };
	}

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	// A game in the "playing" state, ready to be stepped a frame at a time.
	struct Table
	{
		Game game{ "games/frogger.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& frog() { return game.getObject("frog"); }
		float lives() { return frog().variable["lives"]; }
		float score() { return frog().variable["score"]; }

		void hop(const char* direction)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "frog", direction } }, true);
			executor.executeInput(Command{ CmdTriggerAction{ "frog", direction } }, false);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Puts the frog in a cell and lets it settle, nothing else moving it.
		void place(float column, float row)
		{
			frog().position = cellPosition(column, row);
		}

		bool atStart()
		{
			return frog().position.x == frog().positionOriginal.x && frog().position.y == frog().positionOriginal.y;
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

TEST_CASE("frogger.xml loads with a frog, five pads and three lives", "[frogger]")
{
	Table table;

	CHECK(table.lives() == 3.0f);
	CHECK(table.score() == 0.0f);
	CHECK(table.frog().position.x == cellPosition(6, 13).x);
	CHECK(table.frog().position.y == cellPosition(6, 13).y);

	int pads = 0;
	int logs = 0;
	int hazards = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "pads") { ++pads; }
		if (object.objClass == "logs") { ++logs; }
		if (object.objClass == "hazard") { ++hazards; }
	}
	CHECK(pads == 5);
	CHECK(logs == 13);
	CHECK(hazards == 13 + 6); // trucks and cars, and the hedges
}

TEST_CASE("frogger.xml loads under every XML backend, checked against the schema", "[frogger]")
{
	// The new attributes (unless, atmost) are in the schema, so the built-in
	// validator the other three backends use has to accept them as well.
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		Game game{ "games/frogger.xml", backend };
		const Object& frog = game.getObject("frog");
		CHECK(frog.variableOriginal.at("lives") == 3.0f);

		// One rule with unless, and the condition with atmost, came through.
		bool unlessSeen = false;
		for (const auto& rule : frog.collisionData.basic)
		{
			if (rule.unlessClass == "logs") { unlessSeen = true; }
		}
		CHECK(unlessSeen);

		game.setCurrentState("playing");
		REQUIRE(game.getCurrentState().conditions.size() == 2);
		CHECK(game.getCurrentState().conditions[0].atMost.has_value());
	}
}

TEST_CASE("a hop moves the frog one cell, once", "[frogger][hop]")
{
	Table table;
	const Vector2f start = table.frog().position;

	table.hop("up");
	table.frames(1);
	CHECK(table.frog().position.x == start.x);
	CHECK(table.frog().position.y == start.y - kCell);

	// Nothing keeps it going.
	table.frames(10);
	CHECK(table.frog().position.y == start.y - kCell);
	CHECK(table.frog().velocity.y == 0.0f);

	// (To the right: a car in the lane would take the frog on the left.)
	table.hop("right");
	table.frames(1);
	CHECK(table.frog().position.x == start.x + kCell);
	CHECK(table.lives() == 3.0f);
}

TEST_CASE("a hop that would leave the window is refused", "[frogger][hop]")
{
	Table table;

	table.place(0, 13);
	table.hop("left");
	table.frames(1);
	CHECK(table.frog().position.x == cellPosition(0, 13).x);

	table.place(12, 13);
	table.hop("right");
	table.frames(1);
	CHECK(table.frog().position.x == cellPosition(12, 13).x);

	table.place(6, 13);
	table.hop("down");
	table.frames(1);
	CHECK(table.frog().position.y == cellPosition(6, 13).y);

	// Refused is not a loss.
	CHECK(table.lives() == 3.0f);
}

TEST_CASE("a held hop key hops once, and a state change never repeats it", "[frogger][hop][engine_input]")
{
	Game game{ "games/frogger.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& frog = game.getObject("frog");
	const float startY = frog.position.y;

	engine.handleKeyPressed(KeyCode::Space); // mainmenu -> playing
	REQUIRE(game.getCurrentState().name == "playing");

	engine.handleKeyPressed(KeyCode::Up);
	game.updateObjects();
	CHECK(frog.position.y == startY - kCell);

	// Still held: no more hops, however long.
	engine.handleKeyPressed(KeyCode::Up); // a repeat event
	for (int i = 0; i < 30; ++i) { game.updateObjects(); }
	CHECK(frog.position.y == startY - kCell);

	// Pause and come back with the key still down: it has to be pressed again.
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "playing");
	for (int i = 0; i < 5; ++i) { game.updateObjects(); }
	CHECK(frog.position.y == startY - kCell);

	// Let go and press again: one more hop.
	engine.handleKeyReleased(KeyCode::Up);
	engine.handleKeyPressed(KeyCode::Up);
	game.updateObjects();
	CHECK(frog.position.y == startY - 2 * kCell);
}

TEST_CASE("a car takes a life and sends the frog back to the start", "[frogger][collision]")
{
	Table table;
	Object& car = table.game.getObject("carrow12.1");

	// The frog is put in the car's lane, right where the car is.
	table.frog().position = { car.position.x, car.position.y };
	table.frames(1);

	CHECK(table.lives() == 2.0f);
	CHECK(table.atStart());
}

TEST_CASE("the frog is safe on the pavement and the grass while the traffic goes by", "[frogger][collision]")
{
	Table table;

	table.frames(1200); // twenty seconds of traffic past the pavement
	CHECK(table.lives() == 3.0f);

	table.place(6, 7);
	table.frames(1200);
	CHECK(table.lives() == 3.0f);
	CHECK(table.frog().position.x == cellPosition(6, 7).x);
}

TEST_CASE("hopping into the river with no log there costs a life", "[frogger][river]")
{
	Table table;

	// Row 6 has logs at 40 (180 long) and 308: 235 is between them.
	table.frog().position = { 235.0f, cellPosition(0, 7).y };
	table.hop("up");
	table.frames(1);

	CHECK(table.lives() == 2.0f);
	CHECK(table.atStart());
}

TEST_CASE("hopping onto a log is safe, and the log carries the frog", "[frogger][river][carry]")
{
	Table table;
	const Object& log = table.game.getObject("logrow6.1"); // 4 cells long, +1 a frame

	table.frog().position = { log.position.x + 60.0f, cellPosition(0, 7).y };
	table.hop("up");
	table.frames(1);

	REQUIRE(table.lives() == 3.0f);
	const float landed = table.frog().position.x;

	table.frames(20);
	CHECK(table.lives() == 3.0f);
	CHECK(table.frog().position.x == landed + 20.0f * log.velocity.x);
	CHECK(table.frog().position.y == cellPosition(0, 6).y);
}

TEST_CASE("a log carrying the frog off the side of the screen costs a life", "[frogger][river][carry]")
{
	Table table;
	const Object& log = table.game.getObject("logrow6.3"); // from 576, heading right

	// Hop up onto its first cells, as far right as a hop can go.
	table.frog().position = { 580.0f, cellPosition(0, 7).y };
	table.hop("up");
	table.frames(1);
	REQUIRE(table.lives() == 3.0f);
	REQUIRE(table.frog().position.x > log.position.x);

	// Carried right until the frog's side crosses the edge of the screen.
	for (int i = 0; i < 40 && table.lives() == 3.0f; ++i) { table.frames(1); }
	CHECK(table.lives() == 2.0f);
	CHECK(table.atStart());
}

TEST_CASE("a hedge costs a life", "[frogger][collision]")
{
	Table table;

	table.place(3, 2); // under the second hedge
	// 150 is where column 3 begins, plus the inset, so it lands right under it.
	table.hop("up");
	table.frames(1);

	CHECK(table.lives() == 2.0f);
	CHECK(table.atStart());
}

TEST_CASE("reaching a pad scores, puts the pad away and sends the frog back", "[frogger][pads]")
{
	Table table;
	Object& pad = table.game.getObject("pads.1");

	table.place(2, 2);
	table.hop("up");
	table.frames(1);

	CHECK(table.score() == 1.0f);
	CHECK(table.lives() == 3.0f);
	CHECK(table.atStart());
	CHECK_FALSE(pad.isVisible);
	CHECK_FALSE(pad.collisionData.enabled);

	// The frog that was under it is still there, now to be seen.
	CHECK(table.game.getObject("homes.1").isVisible);
}

TEST_CASE("all five pads is a win", "[frogger][pads]")
{
	Table table;

	for (int i = 0; i < 5; ++i)
	{
		CHECK(table.game.getCurrentState().name == "playing");
		table.place(static_cast<float>(2 + 2 * i), 2);
		table.hop("up");
		table.frames(1);
	}

	CHECK(table.score() == 5.0f);
	CHECK(table.game.getCurrentState().name == "youwin");
}

TEST_CASE("running out of lives is game over, and space starts again", "[frogger][conditions]")
{
	Table table;

	for (int i = 0; i < 3; ++i)
	{
		CHECK(table.game.getCurrentState().name == "playing");
		table.place(3, 2);
		table.hop("up"); // into a hedge
		table.frames(1);
	}

	CHECK(table.lives() == 0.0f);
	CHECK(table.game.getCurrentState().name == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);
	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.lives() == 3.0f);
	CHECK(table.score() == 0.0f);
}

TEST_CASE("dec takes a life off the bound text as well as the variable", "[frogger][dec]")
{
	Table table;

	table.game.decrementText("frog.lives");
	CHECK(table.lives() == 2.0f);
	CHECK(table.game.getObject("livesvalue").spriteParams.at(1) == "2");

	table.game.incrementText("frog.lives");
	CHECK(table.lives() == 3.0f);
	CHECK(table.game.getObject("livesvalue").spriteParams.at(1) == "3");
}

TEST_CASE("cars, trucks and logs loop round the screen without losing their spacing", "[frogger][wrap]")
{
	Table table;

	struct Lane { const char* prefix; int count; };
	const Lane lanes[] = {
		{ "truckrow8", 2 }, { "carrow9", 2 }, { "carrow10", 3 }, { "carrow11", 3 }, { "carrow12", 3 },
		{ "logrow2", 2 }, { "logrow3", 3 }, { "logrow4", 2 }, { "logrow5", 3 }, { "logrow6", 3 },
	};

	const auto lane = [&](const Lane& l)
	{
		std::vector<Object*> members;
		for (int i = 0; i < l.count; ++i)
		{
			members.push_back(&table.game.getObject(std::string(l.prefix) + "." + std::to_string(i + 1)));
		}
		return members;
	};

	// Positions in a lane's own loop: the distance from the first member,
	// modulo the length of the loop.
	const auto spacing = [](const std::vector<Object*>& members)
	{
		const float loop = kWindowWidth + members[0]->size.x;
		std::vector<float> gaps;
		for (std::size_t i = 1; i < members.size(); ++i)
		{
			gaps.push_back(std::fmod(members[i]->position.x - members[0]->position.x + 2 * loop, loop));
		}
		return gaps;
	};

	std::vector<std::vector<float>> before;
	for (const auto& l : lanes) { before.push_back(spacing(lane(l))); }

	for (int frame = 0; frame < 2000; ++frame)
	{
		table.frames(1);

		for (const auto& l : lanes)
		{
			for (const Object* member : lane(l))
			{
				REQUIRE(member->position.x > -member->size.x - 4.0f);
				REQUIRE(member->position.x < kWindowWidth + 4.0f);
			}
		}
	}

	for (std::size_t i = 0; i < std::size(lanes); ++i)
	{
		const auto after = spacing(lane(lanes[i]));
		REQUIRE(after.size() == before[i].size());
		for (std::size_t g = 0; g < after.size(); ++g)
		{
			INFO(lanes[i].prefix);
			// Everything moves at one speed, so the gaps are the same to within
			// what a float adds up over two thousand frames.
			CHECK(std::abs(after[g] - before[i][g]) < 0.5f);
		}
	}
}

TEST_CASE("an object only wraps once it has gone right off the screen", "[frogger][wrap]")
{
	Table table;
	Object& car = table.game.getObject("carrow9.1"); // 36 wide, +3 a frame

	car.position.x = kWindowWidth - 20.0f; // still partly in view
	table.frames(1);
	CHECK(car.position.x == kWindowWidth - 17.0f);

	car.position.x = kWindowWidth + 1.0f; // gone, still heading right
	table.frames(1);
	CHECK(car.position.x == kWindowWidth + 1.0f - (kWindowWidth + 36.0f) + 3.0f);

	Object& other = table.game.getObject("carrow10.1"); // heading left, -1.5
	other.position.x = -36.0f - 2.0f; // gone off the left
	table.frames(1);
	CHECK(other.position.x == -38.0f + (kWindowWidth + 36.0f) - 1.5f);
}
