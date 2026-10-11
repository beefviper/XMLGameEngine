// trace.h
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026

#pragma once

#include "audio.h"
#include "bitmap.h"
#include "engine.h"
#include "keycode.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace xge::trace
{
	// The frame trace: a game played with no window and no player, from keys
	// written down, with its random numbers started from a seed, and the
	// picture kept at the frames asked for. A program generated from the game
	// (windows-cpp, built with BUILD_TESTING) takes the same --frames, --keys and
	// --seed and writes the same files, so the two can be put side by side:
	// scripts/shell/windows/games-test-all.bat does, game by game.
	//
	// Frames are counted from 1: frame 1 is the first step and draw after the
	// game starts. A key written for frame N is pressed (or released) just
	// before frame N's step.

	struct KeyEvent
	{
		int frame{};
		KeyCode key{};
		bool down{};
	};

	// "10,60,120": frames to keep, each 1 or more. Returned in order, each once.
	// Throws std::invalid_argument saying what is wrong.
	std::vector<int> parseFrames(const std::string& text);

	// "5+space,7-space,40+d": + presses a key and - releases it, before the
	// frame in front; the key is named as <input button="..."> does. Returned in
	// frame order, those of one frame in the order written. Throws
	// std::invalid_argument saying what is wrong.
	std::vector<KeyEvent> parseKeys(const std::string& text);

	// The file a frame is kept in: frame0030.bmp. A 24 bit BMP, which any
	// picture viewer opens and which is simple enough to read back here.
	std::string frameFile(int frame);

	// Throw std::runtime_error when the file cannot be written or read, or is
	// not a BMP written by writeBitmap.
	void writeBitmap(const std::filesystem::path& file, const Bitmap& picture);
	Bitmap readBitmap(const std::filesystem::path& file);

	struct Difference
	{
		// Pixels where a colour (red, green or blue) is further apart than the
		// tolerance, and not the colour of a pixel next to it in the other picture
		// either, and the furthest any colour is apart. A picture of another size differs
		// in every pixel.
		std::size_t pixels{};
		int largest{};
	};

	Difference compare(const Bitmap& a, const Bitmap& b, int tolerance);

	// The audio of a trace: plays nothing and writes down each sound the game asks
	// for, as "frame 0012 wall", with the frame that run() has reached.
	class SoundLog : public Audio
	{
	public:
		void load(const std::vector<SoundDesc>&) override {}
		void play(const std::string& name) override;
		void stopAll() override {}

		// What was heard, in order of frame and then name.
		std::vector<std::string> lines() const;

		int frame = 0;

	private:
		std::vector<std::string> heard;
	};

	// Where one object is, as the trace keeps it: the engine's and a generated
	// program's are written and compared by name and place.
	struct Place
	{
		std::string name;
		float x{};
		float y{};
	};

	// The shown objects in play, in the order of the game file, with where each is.
	std::vector<Place> placesOf(Game& game);

	// The file the places of a frame are kept in: state0030.txt, a line to an
	// object (its name, x and y).
	std::string stateFile(int frame);

	void writePlaces(const std::filesystem::path& file, const std::vector<Place>& places);
	std::vector<Place> readPlaces(const std::filesystem::path& file);

	// What is different between two lists of places, in words: how many objects
	// differ by more than `within` pixels, and the first of them (by the engine's
	// name for it); empty when there is no difference.
	std::string comparePlaces(const std::vector<Place>& engine, const std::vector<Place>& program, float within);

	struct Request
	{
		std::vector<int> frames;
		std::vector<KeyEvent> keys;

		// Where each frame is written (empty: not written), and a folder of frames
		// a generated program wrote to compare each with (empty: not compared).
		std::filesystem::path write;
		std::filesystem::path against;

		// How far apart a colour may be and still count as the same: text edges
		// are not drawn exactly the same by a picture kept and a text drawn.
		int tolerance = 0;

		// How many pixels may be further apart than that and the frame still
		// count as the same: a shape's edge is drawn a pixel differently when it
		// is a little differently placed.
		std::size_t allow = 0;

		// The log of the audio the engine was made with, if the sounds are to be
		// kept (as sounds.txt beside the frames) and compared (with the one beside
		// the other's).
		SoundLog* sounds = nullptr;

		// How many frames apart the same sound may be heard by the two and still
		// count as the same: a touch found a frame earlier or later is not heard.
		int soundSlack = 0;
	};

	// Plays the engine (whose window must be offscreen) up to the last frame
	// asked for, saying on `out` what each frame did. A frame that differs from
	// the one it is compared with is also written, as <frame>.engine.bmp beside
	// the other, so the two can be looked at. Returns how many frames did not
	// match (or could not be compared); 0 is a pass.
	int run(Engine& engine, const Request& request, std::ostream& out);
}
