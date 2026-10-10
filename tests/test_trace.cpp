// test_trace.cpp
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026
//
// Catch2 tests for the frame trace (lib/source/trace.cpp): the lists of frames
// and keys, the BMP a frame is kept in, comparing two pictures, and playing a
// game from keys and keeping its frames, through a fake window that draws
// nothing and says what the game has done so far as a picture.
//
// Test names must not start with "-" or contain a comma: ctest hands the name to
// Catch2 on its command line, which reads them as an option or a list.

#include "engine.h"
#include "game.h"
#include "trace.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	namespace fs = std::filesystem;

	// A fresh folder in the temporary one, removed again.
	struct TempFolder
	{
		explicit TempFolder(const std::string& name) :
			path(fs::temp_directory_path() / name)
		{
			fs::remove_all(path);
		}

		~TempFolder()
		{
			std::error_code ignored;
			fs::remove_all(path, ignored);
		}

		fs::path path;
	};

	// A window that draws nothing, and whose picture is one pixel as red as the
	// frames displayed so far: a game's frames can be told apart and counted.
	class CountingWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override { ++displayed; }

		Bitmap capture() const override
		{
			Bitmap picture;
			picture.width = 3;
			picture.height = 2;
			picture.rgba.assign(static_cast<std::size_t>(picture.width * picture.height) * 4, 0);
			picture.rgba[0] = static_cast<std::uint8_t>(displayed);
			picture.rgba[3] = 255;
			return picture;
		}

		int displayed = 0;
	};

	Bitmap pictureOf(int width, int height, std::uint8_t red)
	{
		Bitmap picture;
		picture.width = width;
		picture.height = height;
		picture.rgba.assign(static_cast<std::size_t>(width * height) * 4, 255);
		for (std::size_t at = 0; at < picture.rgba.size(); at += 4) { picture.rgba[at] = red; }
		return picture;
	}
}

TEST_CASE("frames are a list of numbers from 1, in order and each once", "[trace]")
{
	CHECK(trace::parseFrames("10") == std::vector<int>{ 10 });
	CHECK(trace::parseFrames("60,10,10,30") == std::vector<int>{ 10, 30, 60 });

	CHECK_THROWS_AS(trace::parseFrames(""), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseFrames("0"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseFrames("10,,20"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseFrames("ten"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseFrames("-5"), std::invalid_argument);
}

TEST_CASE("keys are a frame then + or - then a key's name", "[trace]")
{
	const std::vector<trace::KeyEvent> keys = trace::parseKeys("40-d,5+space,5+left,40+a");

	REQUIRE(keys.size() == 4);
	CHECK(keys[0].frame == 5);
	CHECK(keys[0].key == KeyCode::Space);
	CHECK(keys[0].down);
	CHECK(keys[1].key == KeyCode::Left);
	CHECK(keys[2].frame == 40);
	CHECK(keys[2].key == KeyCode::D);
	CHECK_FALSE(keys[2].down);
	CHECK(keys[3].key == KeyCode::A);
	CHECK(keys[3].down);

	CHECK_THROWS_AS(trace::parseKeys("space"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseKeys("5+"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseKeys("+space"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseKeys("5+spacebar"), std::invalid_argument);
	CHECK_THROWS_AS(trace::parseKeys("0+space"), std::invalid_argument);
}

TEST_CASE("a frame's file is named by its number", "[trace]")
{
	CHECK(trace::frameFile(7) == "frame0007.bmp");
	CHECK(trace::frameFile(1234) == "frame1234.bmp");
}

TEST_CASE("a picture is read back from its BMP as it was written", "[trace]")
{
	TempFolder folder("xge_test_trace_bmp");
	fs::create_directories(folder.path);

	// a width that is not a multiple of 4 pixels is padded in the file
	Bitmap picture = pictureOf(5, 3, 0);
	for (std::size_t at = 0; at < picture.rgba.size(); at += 4)
	{
		picture.rgba[at] = static_cast<std::uint8_t>(at);
		picture.rgba[at + 1] = static_cast<std::uint8_t>(at * 3);
		picture.rgba[at + 2] = static_cast<std::uint8_t>(255 - at);
	}

	trace::writeBitmap(folder.path / "a.bmp", picture);
	const Bitmap back = trace::readBitmap(folder.path / "a.bmp");

	CHECK(back.width == 5);
	CHECK(back.height == 3);
	CHECK(back.rgba == picture.rgba);

	CHECK_THROWS_AS(trace::readBitmap(folder.path / "missing.bmp"), std::runtime_error);
}

TEST_CASE("comparing counts the pixels further apart than the tolerance", "[trace]")
{
	// a block of colour in the middle of one picture, and not in the other
	Bitmap a = pictureOf(6, 6, 10);
	Bitmap b = a;
	for (int y = 1; y <= 3; ++y)
	{
		for (int x = 1; x <= 3; ++x) { b.rgba[static_cast<std::size_t>(y * 6 + x) * 4] = 100; }
	}

	// only the middle of it has no pixel of the colour of a's next to it: the edge
	// of a shape a pixel out of place is not a difference
	const trace::Difference found = trace::compare(a, b, 0);
	CHECK(found.pixels == 1);
	CHECK(found.largest == 90);

	CHECK(trace::compare(a, b, 90).pixels == 0);
	CHECK(trace::compare(a, a, 0).pixels == 0);

	// pictures of another size differ everywhere
	CHECK(trace::compare(a, pictureOf(5, 4, 10), 255).pixels == 36);
}

TEST_CASE("playing a game keeps the frames asked for and compares them", "[trace]")
{
	TempFolder folder("xge_test_trace_run");

	auto play = [&](const trace::Request& request, std::ostringstream& said)
	{
		Game game{ "games/pong.xml" };
		auto window = std::make_unique<CountingWindow>();
		Engine engine(game, std::move(window));
		return trace::run(engine, request, said);
	};

	trace::Request keep;
	keep.frames = trace::parseFrames("3,7");
	keep.write = folder.path;

	std::ostringstream kept;
	CHECK(play(keep, kept) == 0);
	CHECK(kept.str() == "frame 3: kept\nframe 7: kept\n");
	CHECK(fs::exists(folder.path / "frame0003.bmp"));
	CHECK(fs::exists(folder.path / "frame0007.bmp"));
	CHECK_FALSE(fs::exists(folder.path / "frame0004.bmp"));

	// the same game again matches what it kept; the frames in between were drawn
	// too, so a frame's picture is its own number
	trace::Request again;
	again.frames = keep.frames;
	again.against = folder.path;

	std::ostringstream same;
	CHECK(play(again, same) == 0);
	CHECK(same.str() == "frame 3: same\nframe 7: same\n");

	// a picture that is not the one kept is a difference, and is saved to look at
	trace::writeBitmap(folder.path / "frame0007.bmp", pictureOf(3, 2, 200));

	std::ostringstream different;
	CHECK(play(again, different) == 1);
	CHECK(different.str().find("frame 3: same") != std::string::npos);
	CHECK(different.str().find("frame 7: DIFFERENT") != std::string::npos);
	CHECK(fs::exists(folder.path / "frame0007.engine.bmp"));

	// ...unless it is within what is allowed
	trace::Request lenient = again;
	lenient.tolerance = 255;
	std::ostringstream allowed;
	CHECK(play(lenient, allowed) == 0);

	// a frame the other side never kept cannot be compared
	fs::remove(folder.path / "frame0003.bmp");
	std::ostringstream missing;
	CHECK(play(again, missing) == 2);
	CHECK(missing.str().find("frame 3: DIFFERENT") != std::string::npos);
}

TEST_CASE("the sounds heard are listed in order of frame and name", "[trace]")
{
	trace::SoundLog log;
	log.frame = 7;
	log.play("zap");
	log.frame = 3;
	log.play("wall");
	log.play("hit");

	CHECK(log.lines() == std::vector<std::string>{ "frame 0003 hit", "frame 0003 wall", "frame 0007 zap" });
}

TEST_CASE("the sounds of a run are compared with the other program's", "[trace]")
{
	TempFolder folder("xge_test_trace_sounds");
	fs::create_directories(folder.path);
	trace::writeBitmap(folder.path / "frame0001.bmp", pictureOf(3, 2, 1));

	auto compareWith = [&](const std::string& other, int slack)
	{
		{
			std::ofstream file(folder.path / "sounds.txt");
			file << other;
		}

		Game game{ "games/pong.xml" };
		Engine engine(game, std::make_unique<CountingWindow>());

		trace::SoundLog log;
		trace::Request request;
		request.frames = { 1 };
		request.against = folder.path;
		request.sounds = &log;
		request.soundSlack = slack;
		request.tolerance = 255;

		std::ostringstream said;
		const int different = trace::run(engine, request, said);
		return std::make_pair(different, said.str());
	};

	// pong's menu plays nothing in one frame, so the engine heard none
	const auto same = compareWith("", 0);
	CHECK(same.first == 0);
	CHECK(same.second.find("sounds: same") != std::string::npos);

	// a sound only the other program heard is a difference, however much slack
	const auto extra = compareWith("frame 0001 wall\n", 5);
	CHECK(extra.first == 1);
	CHECK(extra.second.find("sounds: DIFFERENT") != std::string::npos);
}
