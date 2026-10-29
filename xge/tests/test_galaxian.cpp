// test_galaxian.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for <paths> and <follow> (Game::follow, Game::applyPaths) and
// for games/descriptions/galaxian.xml, the game they were made for: a path flown step by
// step at its speed, home, a start, a stagger, the commands of a step, what
// ends a path, the mistakes a game file can make with them, and Galaxian
// played frame by frame through a real xge::Engine with no window: the fleet
// flying in to its places (none drawn there first), a dive that weaves down,
// wraps and comes home, a shot alien scoring, a new fleet when the last one
// is down (the fleet is the playing state's one scene, put back with
// <reset scene>), the pause
// drawing the game still, and the table of the best five: initials asked
// for, ranked, and kept in a file, two players taking turns, and the demo
// the title plays when left alone.

#include "scratch.h"
#include "engine/engine.h"
#include "game/game.h"
#include "load/xml.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Keys pressed at the window (as a player's would be) go in `typed`.
	class FakeWindow : public Window
	{
	public:
		explicit FakeWindow(std::vector<std::pair<KeyCode, bool>>& keys) : typed(keys) {}
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return std::exchange(typed, {}); }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}

	private:
		std::vector<std::pair<KeyCode, bool>>& typed;
	};

	struct Play
	{
		std::vector<std::pair<KeyCode, bool>> typed;
		Game game;
		Engine engine;

		explicit Play(const char* file) : Play(readGame(file)) {}

		explicit Play(GameData data)
			: game(std::move(data)), engine(game, std::make_unique<FakeWindow>(typed))
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

		std::string state() { return game.getCurrentState().name; }
		Object& object(const char* name) { return game.getObject(name); }

		int count(bool (*test)(const Object&))
		{
			int n = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "alien" && test(object)) { ++n; }
			}
			return n;
		}
	};

	bool isHome(const Object& object)
	{
		return std::abs(object.position.x - object.positionOriginal.x - object.shifted.x) < 0.01f
			&& std::abs(object.position.y - object.positionOriginal.y - object.shifted.y) < 0.01f;
	}

	// A file in the temporary folder, removed afterwards.
	struct ScratchFile
	{
		std::filesystem::path path;
		~ScratchFile() { std::error_code ignored; std::filesystem::remove(path, ignored); }
	};

	// A game of boxes on one screen, with the paths given, loaded the way
	// Engine starts one: first state shown, every shape measured.
	struct Loaded
	{
		ScratchFile file;
		Game game;

		Loaded(const std::string& paths, const std::string& objects, const std::string& stateExtra = "") :
			file{ write(paths, objects, stateExtra) },
			game(file.path.string())
		{
			game.setCurrentState(0);
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
			}
		}

		static std::filesystem::path write(const std::string& paths, const std::string& objects, const std::string& stateExtra)
		{
			const std::filesystem::path path = std::filesystem::temp_directory_path() / scratchName("xge_paths_scratch.xml");
			std::ofstream out(path);
			out << "<game><window name=\"paths\"><width>200</width><height>100</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>10</framerate></window><variables><variable name=\"unused\">0</variable></variables>"
				<< paths << "<objects>" << objects << "</objects><states><state name=\"playing\"><shows><show object=\"a\" /><show object=\"b\" /></shows>"
				"<inputs></inputs>" << stateExtra << "</state></states></game>";
			return path;
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}
	};

	std::string box(const std::string& name, int x, int y, const std::string& extra = "")
	{
		return "<object name=\"" + name + "\"><sprite><rectangle><width>4</width><height>4</height><color>color.white</color></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions>" + extra + "</object>";
	}

	const std::string kSquare = "<paths><path name=\"square\"><speed>2</speed>"
		"<step><x>10</x><y>0</y></step><step><x>0</x><y>10</y></step><home /></path></paths>";
}

// ---------------------------------------------------------------- the verb

TEST_CASE("a path is flown a step at a time at its speed, and home takes the object back to its place", "[paths]")
{
	Loaded loaded(kSquare, box("a", 50, 50) + box("b", 0, 0));
	Object& a = loaded.game.getObject("a");
	loaded.game.follow(a, "square");
	CHECK(a.isFollowing());

	loaded.frames(5);
	CHECK(a.position.x == Approx(60.0f));
	CHECK(a.position.y == Approx(50.0f));

	loaded.frames(5);
	CHECK(a.position.x == Approx(60.0f));
	CHECK(a.position.y == Approx(60.0f));

	// Home is the diagonal back, at the same speed: about 7.07 frames.
	loaded.frames(7);
	CHECK(a.isFollowing());
	loaded.frames(2);
	CHECK_FALSE(a.isFollowing());
	CHECK(isHome(a));
	CHECK(a.velocity.x == 0.0f);
	CHECK(a.velocity.y == 0.0f);
}

TEST_CASE("a path with a start puts the object there first, and a stagger sends a group one behind another", "[paths]")
{
	const std::string paths = "<paths><path name=\"in\"><speed>5</speed><start><x>-20</x><y>10</y></start>"
		"<step><x>20</x><y>0</y></step><home /></path></paths>";
	const std::string group = "<group name=\"b\"><sprite><rectangle><width>4</width><height>4</height><color>color.white</color></rectangle></sprite>"
		"<position><y>40</y></position><velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions>"
		"<member><position><x>100</x></position></member><member><position><x>120</x></position></member><member><position><x>140</x></position></member></group>";
	Loaded loaded(paths, box("a", 0, 0) + group);

	// At a framerate of 10, half a second apart is 5 frames.
	loaded.game.follow("b", "in", 0.5f);
	Object& first = loaded.game.getObject("b.1");
	Object& second = loaded.game.getObject("b.2");
	Object& third = loaded.game.getObject("b.3");
	CHECK(first.position.x == Approx(-20.0f));
	CHECK(third.position.x == Approx(-20.0f));
	CHECK(second.followWait == 5);
	CHECK(third.followWait == 10);

	loaded.frames(4);
	CHECK(first.position.x == Approx(0.0f));
	CHECK(second.position.x == Approx(-20.0f));

	loaded.frames(100);
	CHECK(isHome(first));
	CHECK(isHome(second));
	CHECK(isHome(third));
	CHECK_FALSE(third.isFollowing());
}

TEST_CASE("a step runs its commands as it sets off, and an object on a path finishes it before another", "[paths]")
{
	const std::string paths = "<paths><path name=\"count\"><speed>1</speed>"
		"<step><x>3</x><y>0</y><inc variable=\"a.n\" /></step><step><x>-3</x><y>0</y><inc variable=\"a.n\">10</inc></step></path></paths>";
	Loaded loaded(paths, box("a", 50, 50, "<variables><variable name=\"n\">0</variable></variables>") + box("b", 0, 0));
	Object& a = loaded.game.getObject("a");

	loaded.game.follow(a, "count");
	loaded.frames(1);
	CHECK(a.variable["n"] == 1.0f);

	// A second <follow> while it is on its way does nothing.
	loaded.game.follow(a, "count");
	loaded.frames(3);
	CHECK(a.variable["n"] == 11.0f);
	loaded.frames(3);
	CHECK(a.variable["n"] == 11.0f);
	CHECK_FALSE(a.isFollowing());
	CHECK(isHome(a));
}

TEST_CASE("a reset ends a path, and an object out of play is not moved along it", "[paths]")
{
	Loaded loaded(kSquare, box("a", 50, 50) + box("b", 0, 0));
	Object& a = loaded.game.getObject("a");

	loaded.game.follow(a, "square");
	loaded.frames(3);
	loaded.game.resetObject("a");
	CHECK_FALSE(a.isFollowing());
	CHECK(isHome(a));

	loaded.game.follow(a, "square");
	loaded.frames(3);
	a.isVisible = false;
	loaded.frames(3);
	CHECK(a.isFollowing());
	CHECK(a.position.x == Approx(56.0f));
}

TEST_CASE("a <follow> in a timer sends its own object, and in a state needs object=", "[paths]")
{
	Loaded loaded(kSquare, box("a", 50, 50, "<timers><timer><after>0.1</after><follow path=\"square\" /></timer></timers>") + box("b", 0, 0));
	loaded.frames(1);
	CHECK(loaded.game.getObject("a").isFollowing());
	CHECK_FALSE(loaded.game.getObject("b").isFollowing());

	Loaded fromState(kSquare, box("a", 50, 50) + box("b", 0, 0),
		"<timers><timer><after>0.1</after><follow path=\"square\" object=\"b\" /></timer></timers>");
	fromState.frames(1);
	CHECK(fromState.game.getObject("b").isFollowing());
	CHECK_FALSE(fromState.game.getObject("a").isFollowing());
}

TEST_CASE("<nearest> in a <follow> sends only that many, the nearest to the one doing it", "[paths]")
{
	const std::string group = "<group name=\"b\"><sprite><rectangle><width>4</width><height>4</height><color>color.white</color></rectangle></sprite>"
		"<position><y>40</y></position><velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions>"
		"<member><position><x>100</x></position></member><member><position><x>120</x></position></member><member><position><x>140</x></position></member></group>";
	Loaded loaded(kSquare, box("a", 135, 0,
		"<timers><timer><after>0.1</after><follow path=\"square\" object=\"b\"><nearest>2</nearest></follow></timer></timers>") + group);
	loaded.frames(1);
	CHECK_FALSE(loaded.game.getObject("b.1").isFollowing());
	CHECK(loaded.game.getObject("b.2").isFollowing());
	CHECK(loaded.game.getObject("b.3").isFollowing());
	CHECK_FALSE(loaded.game.getObject("a").isFollowing());
}

TEST_CASE("<shift> moves objects and the places they go home to; one on a path only has its place moved", "[paths]")
{
	Loaded loaded(kSquare, box("a", 50, 50) + box("b", 0, 0,
		"<timers><timer><every>0.1</every><shift object=\"a\"><x>1</x></shift></timer></timers>"));
	Object& a = loaded.game.getObject("a");
	loaded.frames(3);
	CHECK(a.position.x == Approx(53.0f));
	CHECK(isHome(a));

	// Sent round the square, it is not pushed along by the shift, but
	// comes home to where the shift has taken its place meanwhile.
	loaded.game.follow(a, "square");
	loaded.frames(5);
	CHECK(a.position.x == Approx(63.0f));
	CHECK(a.shifted.x == Approx(8.0f));
	loaded.frames(40);
	CHECK_FALSE(a.isFollowing());
	CHECK(isHome(a));

	// A reset puts it back where the file says.
	loaded.game.resetObject("a");
	CHECK(a.position.x == Approx(50.0f));
	CHECK(a.shifted.x == Approx(0.0f));
}

TEST_CASE("a path step that names something live is worked out as it starts, with self the one following", "[paths]")
{
	const std::string paths = "<paths><path name=\"aim\"><speed>2</speed>"
		"<step><x>b.x - self.x</x><y>0</y></step><home /></path></paths>";
	Loaded loaded(paths, box("a", 50, 50) + box("b", 10, 0));
	Object& a = loaded.game.getObject("a");
	Object& b = loaded.game.getObject("b");
	b.position.x = 30.0f;
	loaded.game.follow(a, "aim");
	loaded.frames(1);

	// Moving b once the step has started changes nothing.
	b.position.x = 0.0f;
	loaded.frames(9);
	CHECK(a.position.x == Approx(30.0f));
	loaded.frames(20);
	CHECK(isHome(a));
}

TEST_CASE("a step's commands can name the one flying it as self, whoever sent it", "[paths]")
{
	const std::string paths = "<paths><path name=\"count\"><speed>1</speed>"
		"<step><x>3</x><y>0</y><inc variable=\"self.n\">self.x</inc></step></path></paths>";
	const std::string group = "<group name=\"b\"><sprite><rectangle><width>4</width><height>4</height><color>color.white</color></rectangle></sprite>"
		"<position><y>40</y></position><velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions>"
		"<variables><variable name=\"n\">0</variable></variables>"
		"<member><position><x>100</x></position></member><member><position><x>120</x></position></member></group>";
	Loaded loaded(paths, box("a", 0, 0, "<timers><timer><after>0.1</after><follow path=\"count\" object=\"b\" /></timer></timers>") + group);
	loaded.frames(2);
	CHECK(loaded.game.getObject("b.1").variable["n"] == 100.0f);
	CHECK(loaded.game.getObject("b.2").variable["n"] == 120.0f);

	// One that cannot be sent along it says nothing of it; one that can and
	// has no such variable stops the load, naming it.
	CHECK_THROWS_WITH(Loaded(paths, box("a", 0, 0, "<timers><timer><after>0.1</after><follow path=\"count\" /></timer></timers>") + group),
		ContainsSubstring("flown by 'a'"));
}

TEST_CASE("mistakes with paths are turned away when the game loads, saying where", "[paths][errors]")
{
	const auto fails = [](const std::string& paths, const std::string& objects, const std::string& stateExtra, const std::string& expected)
	{
		INFO(paths << objects << stateExtra);
		CHECK_THROWS_WITH(Loaded(paths, objects, stateExtra), ContainsSubstring(expected));
	};

	const std::string both = box("a", 0, 0) + box("b", 0, 0);
	fails(kSquare, box("a", 0, 0, "<timers><timer><after>1</after><follow path=\"circle\" /></timer></timers>") + box("b", 0, 0), "",
		"<follow path=\"circle\" /> names no path");
	fails(kSquare, both, "<timers><timer><after>1</after><follow path=\"square\" /></timer></timers>", "needs object=\"...\" here");
	fails(kSquare, both, "<timers><timer><after>1</after><follow path=\"square\" object=\"c\" /></timer></timers>", "<follow> names 'c'");
	fails("<paths><path name=\"p\"><speed>0</speed><home /></path></paths>", both, "", "the <speed> is not above 0");
	fails("<paths><path name=\"p\"><speed>1</speed></path></paths>", both, "", "has no <step> or <home />");
	fails("<paths><path name=\"p\"><speed>1</speed><home /></path><path name=\"p\"><speed>1</speed><home /></path></paths>", both, "",
		"already a path of that name");
	fails("<paths><path name=\"p\"><speed>1</speed><wait /></path></paths>", both, "", "unknown <wait>");
	fails(kSquare, both, "<timers><timer><after>1</after><shift object=\"a\" /></timer></timers>", "<shift>");
	fails(kSquare, both, "<timers><timer><after>1</after><shift object=\"c\"><x>1</x></shift></timer></timers>", "'c'");
	fails(kSquare, box("a", 0, 0, "<timers><timer><after>1</after><follow path=\"square\"><nearest>1</nearest></follow></timer></timers>") + box("b", 0, 0), "",
		"<nearest>");
	fails(kSquare, both, "<timers><timer><after>1</after><follow path=\"square\" object=\"b\"><nearest>0</nearest></follow></timer></timers>", "<nearest>");
}

// ---------------------------------------------------------------- Galaxian

TEST_CASE("galaxian.xml loads 46 aliens in six rows, a ship and six paths", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");

	CHECK(play.state() == "mainmenu");
	CHECK(play.count([](const Object& object) { return object.isVisible; }) == 46);
	CHECK(play.game.getPaths().size() == 6);
	CHECK(play.object("ship").variable["lives"] == 3.0f);

	// The dive swings out 48 to the left, then turns toward the ship in two
	// steps worked out as they start; down it adds up to the window's height
	// and an alien's, which is what wrapping at the bottom takes off again.
	float across = 0.0f;
	float down = 0.0f;
	int aimed = 0;
	for (const auto& step : play.game.getPaths().at("dive").steps)
	{
		if (step.liveX) { ++aimed; }
		else { across += step.by.x; }
		down += step.by.y;
	}
	CHECK(across == Approx(-48.0f));
	CHECK(aimed == 2);
	CHECK(down == Approx(640.0f + play.object("blues1.1.1").size.y));
}

TEST_CASE("galaxian.xml loads with every XML library, so both schema checkers take <paths>", "[galaxian]")
{
	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		Game game(readGame("descriptions/galaxian.xml", backend));
		CHECK(game.getPaths().size() == 6);
		CHECK(game.getPaths().at("dive").steps.back().home);
	}
}

TEST_CASE("the fleet flies in on its paths, one behind another, and settles into formation", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

	// The first frame sends all six rows off from where their paths start;
	// the last row of blues waits there behind the first, a line of twenty.
	play.frames(1);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 46);
	CHECK(play.object("blues2.10.1").position.x == Approx(567.0f));
	CHECK(play.object("blues1.10.1").position.x == Approx(-40.0f));
	CHECK(play.object("blues3.1.1").position.x == Approx(-40.0f));
	CHECK(play.object("blues1.10.1").followWait > play.object("blues1.2.1").followWait);
	CHECK(play.object("blues3.1.1").followWait > play.object("blues1.10.1").followWait);

	play.frames(400);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 0);
	CHECK(play.count([](const Object& object) { return isHome(object); }) == 46);
}

TEST_CASE("a diver weaves down, drops bombs, goes off the bottom and comes back to its place from the top", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	// The ship, far to the right, is out of play so nothing hits it; the
	// dive still steers at it.
	Object& ship = play.object("ship");
	ship.position.x = 700.0f;
	ship.isVisible = false;

	Object& diver = play.object("escorts.1.1");
	REQUIRE(isHome(diver));
	play.game.follow(diver, "dive");

	float closest = 1000.0f;
	float lowest = diver.position.y;
	float leftmost = diver.position.x;
	float rightmost = diver.position.x;
	bool cameFromTop = false;
	int bombs = 0;
	int frames = 0;
	while (diver.isFollowing() && frames < 1000)
	{
		play.frames(1);
		++frames;
		lowest = std::max(lowest, diver.position.y);
		if (diver.position.y > 400.0f && diver.position.y < ship.position.y) { closest = std::min(closest, std::abs(diver.position.x - ship.position.x)); }
		leftmost = std::min(leftmost, diver.position.x);
		rightmost = std::max(rightmost, diver.position.x);
		if (diver.position.y < 0.0f) { cameFromTop = true; }
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.objClass == "bomb" && object.isVisible) { ++bombs; break; }
		}
	}

	CHECK_FALSE(diver.isFollowing());
	CHECK(diver.isVisible);
	CHECK(isHome(diver));
	CHECK(lowest > 600.0f);
	CHECK(cameFromTop);
	CHECK(rightmost - leftmost > 60.0f);
	CHECK(closest < 1.0f);
	CHECK(bombs > 0);
}

TEST_CASE("the formation sways from side to side, and a diver comes home to where its place has gone", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	Object& alien = play.object("blues1.1.1");
	const float before = alien.position.x;
	float leftmost = before;
	float rightmost = before;
	for (int frame = 0; frame < 300; ++frame)
	{
		play.frames(1);
		leftmost = std::min(leftmost, alien.position.x);
		rightmost = std::max(rightmost, alien.position.x);
	}
	CHECK(rightmost - leftmost > 20.0f);
	CHECK(rightmost - leftmost < 60.0f);
	CHECK(play.count([](const Object& object) { return object.isFollowing() || isHome(object); }) == play.count([](const Object& object) { return object.isVisible; }));
}

TEST_CASE("a flagship dives with the two escorts nearest it, and a diver shot pays more", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	Object& flagship = play.object("flagships.1.1");
	play.game.follow("playing.fleet.escorts", "dive", 0.0f, 2, &flagship);
	int following = 0;
	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.isFollowing() && object.name.rfind("playing.fleet.escorts.", 0) == 0) { ++following; }
	}
	CHECK(following == 2);
	CHECK(play.object("escorts.1.1").isFollowing());
	CHECK_FALSE(play.object("escorts.6.1").isFollowing());

	// In the formation an escort pays 50; diving, even taken along, 100,
	// from the dive's first step until it wraps.
	Object& escort = play.object("escorts.1.1");
	CHECK(play.object("escorts.6.1").variable["pays"] == 50.0f);
	play.frames(1);
	CHECK(escort.variable["pays"] == 100.0f);
	Object& shot = play.object("shot");
	shot.isVisible = true;
	shot.collisionData.enabled = true;
	// Put on it (the purples are just below, in the way of one fired).
	shot.position = { escort.position.x + escort.size.x / 2.0f, escort.position.y + escort.size.y / 2.0f };
	shot.velocity = {};
	const float before = play.object("ship").variable["score"];
	play.frames(2);
	CHECK(play.object("ship").variable["score"] - before == 100.0f);
}

TEST_CASE("a shot alien scores, and when the fleet is down the next one flies in", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	Object& alien = play.object("blues3.5.1");
	Object& ship = play.object("ship");
	ship.position.x = alien.position.x + alien.size.x / 2.0f - ship.size.x / 2.0f;
	play.tap(KeyCode::Space);
	play.frames(60);
	CHECK_FALSE(alien.isVisible);
	CHECK(ship.variable["score"] == 30.0f);

	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "alien") { object.isVisible = false; object.followPath.clear(); }
	}
	play.frames(1);
	CHECK(play.count([](const Object& object) { return object.isVisible; }) == 46);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 46);
	CHECK(play.object("flagships.1.1").position.y < 0.0f);
	// the ship and its score are the state's, not the fleet's: kept
	CHECK(ship.variable["score"] == 30.0f);
	CHECK(ship.variable["lives"] == 3.0f);
}

TEST_CASE("the fleet is the playing state's scene, and the ship and its shot are the state's own", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	CHECK(play.game.currentScene("playing") == "playing.fleet");
	CHECK(play.object("blues2.5.1").name == "playing.fleet.blues2.5.1");
	CHECK(play.object("ship").name == "playing.ship");
	CHECK(play.object("shot").name == "playing.shot");
	CHECK(play.object("title").name == "mainmenu.title");
	CHECK_FALSE(play.game.isShown(play.object("ship")));
	play.tap(KeyCode::Space);
	CHECK(play.game.isShown(play.object("ship")));
	CHECK(play.game.isShown(play.object("flagships.1.1")));
}

TEST_CASE("paused, the fleet is drawn still underneath and stops where it is", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(30);

	Object& alien = play.object("blues1.1.1");
	REQUIRE(alien.isFollowing());
	const Vector2f where = alien.position;
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	play.frames(60);
	CHECK(alien.position.x == Approx(where.x));
	CHECK(alien.position.y == Approx(where.y));
	CHECK(play.game.isStill(alien));
	CHECK(play.game.isStill(play.object("ship")));

	play.tap(KeyCode::P);
	play.frames(1);
	CHECK(play.state() == "playing");
	CHECK((alien.position.x != Approx(where.x) || alien.position.y != Approx(where.y)));
}

TEST_CASE("the best score follows the score up, and a new game keeps it", "[galaxian][keeps]")
{
	Play play("descriptions/galaxian.xml");
	Object& best = play.object("best");
	CHECK(best.kept);
	CHECK(play.game.isShown(best)); // on the title
	play.tap(KeyCode::Space);
	CHECK(play.game.isShown(best));

	Object& ship = play.object("ship");
	ship.variable["score"] = 240.0f;
	play.frames(1);
	CHECK(best.variable.at("score") == 240.0f);
	CHECK(best.picture.text == "240");

	// Game over, then Space: the score starts again, the best does not.
	ship.variable["lives"] = 0.0f;
	play.frames(1);
	REQUIRE(play.state() == "gameover");
	CHECK(play.game.isStill(best));
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "mainmenu");
	CHECK(ship.variable.at("score") == 0.0f);
	CHECK(best.variable.at("score") == 240.0f);
	CHECK(best.picture.text == "240");

	// A lower score leaves it alone; a higher one moves it on.
	play.tap(KeyCode::Space);
	ship.variable["score"] = 100.0f;
	play.frames(1);
	CHECK(best.variable.at("score") == 240.0f);
	ship.variable["score"] = 300.0f;
	play.frames(1);
	CHECK(best.variable.at("score") == 300.0f);
}

TEST_CASE("a game above the fifth best asks for initials and takes its place in the table", "[galaxian][scores]")
{
	Play play("descriptions/galaxian.xml");
	Object& first = play.object("table.1");
	CHECK(first.kept);
	CHECK(play.game.isShown(first)); // on the title
	CHECK(first.picture.text == "AAA 0");

	const auto playTo = [&](float score)
	{
		play.tap(KeyCode::Space);
		REQUIRE(play.state() == "playing");
		play.object("ship").variable["score"] = score;
		play.object("ship").variable["lives"] = 0.0f;
		play.frames(2); // game over, then above the fifth
	};

	playTo(240.0f);
	REQUIRE(play.state() == "initial1");
	CHECK(play.object("entry").variable.at("score") == 240.0f);
	CHECK(play.object("entryscore").picture.text == "240");

	// Up and down go round the letters; Space goes on to the next.
	play.tap(KeyCode::Up);
	play.tap(KeyCode::Up);
	CHECK(play.object("letter1").picture.text == "C");
	play.tap(KeyCode::S);
	play.tap(KeyCode::S);
	play.tap(KeyCode::S);
	CHECK(play.object("letter1").picture.text == "Z");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "initial2");
	play.tap(KeyCode::W);
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "initial3");
	play.tap(KeyCode::Space);

	// Ranked, and a new game: the title shows it at the top.
	REQUIRE(play.state() == "mainmenu");
	CHECK(first.picture.text == "ZBA 240");
	CHECK(play.object("table.2").picture.text == "AAA 0");
	CHECK(play.object("letter1").picture.text == "A"); // the entry starts again

	// A lower one goes below it; a higher one above both, the rest moving down.
	playTo(100.0f);
	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	playTo(300.0f);
	play.tap(KeyCode::Down);
	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "mainmenu");
	CHECK(first.picture.text == "ZAA 300");
	CHECK(play.object("table.2").picture.text == "ZBA 240");
	CHECK(play.object("table.3").picture.text == "AAA 100");

	// Not above the fifth: no initials.
	for (int i = 4; i <= 5; ++i)
	{
		play.object(("table." + std::to_string(i)).c_str()).variable["score"] = 50.0f;
	}
	playTo(50.0f);
	CHECK(play.state() == "gameover");
}

TEST_CASE("what Galaxian keeps lasts in a file from one run to the next", "[galaxian][scores][keeps]")
{
	const std::filesystem::path file = scratchName("saves/galaxian_test.xml");
	std::filesystem::remove(file);
	{
		Play play("descriptions/galaxian.xml");
		play.engine.keepIn(file.string());
		CHECK_FALSE(std::filesystem::exists(file)); // nothing to read yet
		play.object("table.1").variable["score"] = 500.0f;
		play.object("table.1").variable["first"] = 6.0f;
		play.object("best").variable["score"] = 500.0f;
		play.game.resetAll();
		play.frames(1); // a new game writes it
		REQUIRE(std::filesystem::exists(file));
	}
	{
		Play play("descriptions/galaxian.xml");
		CHECK(play.object("table.1").variable.at("score") == 0.0f);
		play.engine.keepIn(file.string());
		CHECK(play.object("table.1").variable.at("score") == 500.0f);
		CHECK(play.object("table.1").picture.text == "GAA 500");
		CHECK(play.object("best").picture.text == "500");
		CHECK(play.object("table.2").variable.at("score") == 0.0f);
	}
	std::filesystem::remove(file);
	std::error_code notEmpty; // other games' saves stay
	std::filesystem::remove(file.parent_path(), notEmpty);
}

TEST_CASE("two players take turns, each with a game of their own", "[galaxian][turns]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Num2);
	REQUIRE(play.state() == "player1");
	CHECK(play.game.playerCount() == 2);
	CHECK(play.game.currentPlayer() == 1);
	CHECK(play.game.isStill(play.object("ship"))); // their game, under the screen

	// Two seconds, and player 1 plays.
	play.frames(121);
	REQUIRE(play.state() == "playing");
	Object& ship = play.object("ship");
	ship.variable["score"] = 120.0f;
	ship.position.x = 40.0f;

	// A ship lost: player 2's turn, with a game of their own from the start.
	play.game.decrementText("ship.lives");
	play.game.incrementText("ship.lost");
	play.frames(1);
	REQUIRE(play.state() == "player2");
	CHECK(play.game.currentPlayer() == 2);
	CHECK(ship.variable.at("score") == 0.0f);
	CHECK(ship.variable.at("lives") == 3.0f);
	CHECK(play.object("score").picture.text == "0");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	ship.variable["score"] = 30.0f;

	// Player 2 loses one: player 1's game comes back as they left it.
	play.game.decrementText("ship.lives");
	play.game.incrementText("ship.lost");
	play.frames(1);
	REQUIRE(play.state() == "player1");
	CHECK(ship.variable.at("score") == 120.0f);
	CHECK(ship.variable.at("lives") == 2.0f);
	CHECK(ship.variable.at("lost") == 0.0f);
	CHECK(ship.position.x == Approx(40.0f));
	CHECK(play.object("score").picture.text == "120");

	// Player 1 runs out (not above the table's fifth, so no initials): Space
	// goes on to player 2, who plays on alone.
	for (int i = 1; i <= 5; ++i)
	{
		play.object(("table." + std::to_string(i)).c_str()).variable["score"] = 1000.0f;
	}
	play.tap(KeyCode::Space);
	ship.variable["lives"] = 0.0f;
	play.frames(1);
	REQUIRE(play.state() == "gameover");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "player2");
	CHECK(ship.variable.at("score") == 30.0f);
	play.tap(KeyCode::Space);
	play.game.incrementText("ship.lost");
	play.frames(1);
	CHECK(play.state() == "playing"); // no one to pass to
	CHECK(play.game.currentPlayer() == 2);

	// Player 2 out too: a new game.
	ship.variable["lives"] = 0.0f;
	play.frames(1);
	REQUIRE(play.state() == "gameover");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "mainmenu");
	CHECK(play.game.playerCount() == 0);
}

TEST_CASE("one player: <pass /> does nothing and <out /> lets what follows run", "[galaxian][turns]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	Object& ship = play.object("ship");
	ship.variable["score"] = 10.0f;
	play.game.incrementText("ship.lost");
	play.frames(1);
	CHECK(play.state() == "playing");
	CHECK(ship.variable.at("score") == 10.0f);
	CHECK(ship.variable.at("lost") == 0.0f);
	for (int i = 1; i <= 5; ++i)
	{
		play.object(("table." + std::to_string(i)).c_str()).variable["score"] = 1000.0f;
	}
	ship.variable["lives"] = 0.0f;
	play.frames(1);
	REQUIRE(play.state() == "gameover");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "mainmenu");
}

TEST_CASE("left on the title, Galaxian plays its demo until a key is pressed", "[galaxian][demo]")
{
	Play play("descriptions/galaxian.xml");
	REQUIRE(play.game.getDemo().recorded);
	play.object("best").variable["score"] = 70.0f;

	play.frames(599);
	CHECK_FALSE(play.engine.isPlayingDemo());
	play.frames(2); // ten seconds on the title
	REQUIRE(play.engine.isPlayingDemo());
	CHECK(play.state() == "mainmenu"); // a new game, from the start
	CHECK(play.object("best").variable.at("score") == 70.0f); // kept

	// The recording presses Space, and then plays.
	play.frames(40);
	CHECK(play.state() == "playing");
	const float startX = play.object("ship").position.x;
	play.frames(230);
	CHECK(play.object("ship").position.x != Approx(startX)); // it moved the ship

	// A key pressed: the demo ends, without the key doing anything, and
	// the game is as it was before it.
	play.object("best").variable["score"] = 900.0f; // as if the demo had scored
	play.typed = { { KeyCode::Num2, true }, { KeyCode::Num2, false } };
	play.frames(1);
	CHECK_FALSE(play.engine.isPlayingDemo());
	CHECK(play.state() == "mainmenu");
	CHECK(play.game.playerCount() == 0); // the 2 did not start a game
	CHECK(play.object("best").variable.at("score") == 70.0f);
	CHECK(play.object("best").picture.text == "70");

	// Ten more seconds on the title, and it plays again.
	play.frames(601);
	CHECK(play.engine.isPlayingDemo());
}

TEST_CASE("the demo plays to its end and the title comes back", "[galaxian][demo]")
{
	Play play("descriptions/galaxian.xml");
	play.game.requestDemo();
	play.frames(1);
	REQUIRE(play.engine.isPlayingDemo());
	play.frames(static_cast<int>(play.game.getDemo().frames) - 1);
	CHECK(play.engine.isPlayingDemo());
	play.frames(1);
	CHECK_FALSE(play.engine.isPlayingDemo());
	CHECK(play.state() == "mainmenu");
	CHECK(play.object("ship").variable.at("score") == 0.0f);
}

TEST_CASE("a game recorded plays back the same as it was played", "[galaxian][demo]")
{
	Play played("descriptions/galaxian.xml");
	played.engine.record();
	played.tap(KeyCode::Space);
	played.frames(150);
	played.engine.handleKeyPressed(KeyCode::Left);
	played.frames(40);
	played.engine.handleKeyReleased(KeyCode::Left);
	for (int i = 0; i < 6; ++i)
	{
		played.tap(KeyCode::Space);
		played.frames(30);
	}
	played.engine.handleKeyPressed(KeyCode::Right);
	played.frames(98);

	// Where everything is, a frame before the end (the last frame of a
	// demo ends it).
	struct Seen { std::string name; float x; float y; bool visible; };
	std::vector<Seen> seen;
	for (const auto& object : played.game.getCurrentObjects())
	{
		seen.push_back({ object.name, object.position.x, object.position.y, object.isVisible });
	}
	const std::string state = played.state();
	const float score = played.object("ship").variable.at("score");
	played.frames(1);

	const RawDemo demo = played.engine.recorded();
	CHECK(demo.recorded);
	CHECK(demo.frames == 469u);
	REQUIRE(demo.keys.size() == 17);
	CHECK(demo.keys[0].button == "space");
	CHECK(demo.keys[0].pressed);
	CHECK(demo.keys[0].frame == 0u);
	CHECK(demo.keys[2].button == "left");
	CHECK(demo.keys[3].frame == 190u);
	CHECK(demo.keys.back().button == "right");

	// Played back, frame for frame.
	GameData data = readGame("descriptions/galaxian.xml");
	data.demo = demo;
	Play replay(std::move(data));
	replay.game.requestDemo();
	replay.frames(1);
	REQUIRE(replay.engine.isPlayingDemo());
	replay.frames(static_cast<int>(demo.frames) - 1);
	REQUIRE(replay.engine.isPlayingDemo());

	CHECK(replay.state() == state);
	CHECK(replay.object("ship").variable.at("score") == score);
	int same = 0;
	for (const Seen& was : seen)
	{
		const Object& now = replay.object(was.name.c_str());
		if (std::abs(now.position.x - was.x) < 0.01f && std::abs(now.position.y - was.y) < 0.01f && now.isVisible == was.visible) { ++same; }
		else { UNSCOPED_INFO(was.name << " was at " << was.x << "," << was.y << " and is at " << now.position.x << "," << now.position.y); }
	}
	CHECK(same == static_cast<int>(seen.size()));
	CHECK(seen.size() > 36);
}

TEST_CASE("a key held when the turn passes does not keep moving that player's ship when their turn comes back", "[galaxian][turns]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Num2);
	play.frames(121);
	REQUIRE(play.state() == "playing");
	Object& ship = play.object("ship");

	// Player 1 dies moving left, the key still held.
	play.engine.handleKeyPressed(KeyCode::Left);
	play.frames(5);
	REQUIRE(ship.velocity.x != 0.0f);
	play.game.decrementText("ship.lives");
	play.game.incrementText("ship.lost");
	play.frames(1);
	REQUIRE(play.state() == "player2");
	play.engine.handleKeyReleased(KeyCode::Left);
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	CHECK(ship.velocity.x == 0.0f);

	// Player 2 dies standing still: player 1's ship comes back still.
	play.game.decrementText("ship.lives");
	play.game.incrementText("ship.lost");
	play.frames(1);
	REQUIRE(play.state() == "player1");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	const float x = ship.position.x;
	play.frames(10);
	CHECK(ship.velocity.x == 0.0f);
	CHECK(ship.position.x == Approx(x));
}

TEST_CASE("no alien is drawn in its place before it has flown in, at the start or in a new fleet", "[galaxian]")
{
	Play play("descriptions/galaxian.xml");
	play.tap(KeyCode::Space);
	for (int fleet = 0; fleet < 2; ++fleet)
	{
		std::set<std::string> flown;
		int early = 0;
		for (int frame = 0; frame < 400; ++frame)
		{
			play.frames(1);
			for (auto& object : play.game.getCurrentObjects())
			{
				if (object.objClass != "alien" || !play.game.isShown(object)) { continue; }
				if (object.isFollowing()) { flown.insert(object.name); }
				else if (!flown.count(object.name)) { ++early; }
			}
		}
		CHECK(early == 0);
		CHECK(flown.size() == 46);

		// The fleet shot down: the condition puts the next one back.
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.objClass == "alien") { object.isVisible = false; object.followPath.clear(); }
		}
	}
}
