// test_galaxian.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for <paths> and <follow> (Game::follow, Game::applyPaths) and
// for games/galaxian.xml, the game they were made for: a path flown step by
// step at its speed, home, a start, a stagger, the commands of a step, what
// ends a path, the mistakes a game file can make with them, and Galaxian
// played frame by frame through a real xge::Engine with no window: the fleet
// flying in to its places, a dive that weaves down, wraps and comes home, a
// shot alien scoring, and a new fleet when the last one is down.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

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

	struct Play
	{
		Game game;
		Engine engine;

		explicit Play(const char* file)
			: game(file), engine(game, std::make_unique<FakeWindow>())
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
		return std::abs(object.position.x - object.positionOriginal.x) < 0.01f
			&& std::abs(object.position.y - object.positionOriginal.y) < 0.01f;
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
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		static std::filesystem::path write(const std::string& paths, const std::string& objects, const std::string& stateExtra)
		{
			const std::filesystem::path path = std::filesystem::temp_directory_path() / "xge_paths_scratch.xml";
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
}

// ---------------------------------------------------------------- Galaxian

TEST_CASE("galaxian.xml loads 36 aliens in five rows, a ship and six paths", "[galaxian]")
{
	Play play("games/galaxian.xml");

	CHECK(play.state() == "mainmenu");
	CHECK(play.count([](const Object& object) { return object.isVisible; }) == 36);
	CHECK(play.game.getPaths().size() == 6);
	CHECK(play.object("ship").variable["lives"] == 3.0f);

	// The dive adds up to nothing across, and to the window's height and an
	// alien's down, which is what wrapping at the bottom takes off again.
	float across = 0.0f;
	float down = 0.0f;
	for (const auto& step : play.game.getPaths().at("dive").steps)
	{
		across += step.by.x;
		down += step.by.y;
	}
	CHECK(across == Approx(0.0f));
	CHECK(down == Approx(640.0f + play.object("blues.1").size.y));
}

TEST_CASE("galaxian.xml loads with every XML library, so both schema checkers take <paths>", "[galaxian]")
{
	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		Game game("games/galaxian.xml", backend);
		CHECK(game.getPaths().size() == 6);
		CHECK(game.getPaths().at("dive").steps.back().home);
	}
}

TEST_CASE("the fleet flies in on its paths, one behind another, and settles into formation", "[galaxian]")
{
	Play play("games/galaxian.xml");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");

	// The first frame sends every row off from where its path starts.
	play.frames(1);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 36);
	CHECK(play.object("blues2.10").position.x == Approx(567.0f));
	CHECK(play.object("blues.10").position.x == Approx(-40.0f));
	CHECK(play.object("blues.10").followWait > play.object("blues.2").followWait);

	play.frames(400);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 0);
	CHECK(play.count([](const Object& object) { return isHome(object); }) == 36);
}

TEST_CASE("a diver weaves down, drops bombs, goes off the bottom and comes back to its place from the top", "[galaxian]")
{
	Play play("games/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	Object& diver = play.object("escorts.1");
	REQUIRE(isHome(diver));
	play.game.follow(diver, "dive");

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
	CHECK(bombs > 0);
}

TEST_CASE("a shot alien scores, and when the fleet is down the next one flies in", "[galaxian]")
{
	Play play("games/galaxian.xml");
	play.tap(KeyCode::Space);
	play.frames(400);

	Object& alien = play.object("blues2.5");
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
	play.frames(2);
	CHECK(play.count([](const Object& object) { return object.isVisible; }) == 36);
	CHECK(play.count([](const Object& object) { return object.isFollowing(); }) == 36);
	CHECK(play.object("flagships.1").position.y < 0.0f);
}
