// trace.h
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026
//
// The frame trace, for checking a generated game against the engine: built with
// -DBUILD_TESTING=ON (which defines XGE_TRACE) the game draws into a picture in
// memory instead of a window, plays keys written on the command line, draws its
// random numbers from a seed, and keeps its picture at the frames asked for, as
// the same BMP files xgecli --trace writes:
//
//     game --trace <folder> --frames 10,60 [--keys 5+space,40+d,90-d] [--seed 1]
//
// Frames are counted from 1, and a key written for a frame is pressed (+) or
// released (-) just before it. Without --trace the game plays in a window as
// usual. Built without BUILD_TESTING none of this is compiled in: every
// function here is then the plain thing it stands for.

#pragma once

#include <SFML/Graphics.hpp>

#ifdef XGE_TRACE

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <vector>

#endif

namespace trace
{
#ifndef XGE_TRACE

	inline void start(int, char*[]) {}

	inline bool keyHeld(sf::Keyboard::Key key)
	{
		return sf::Keyboard::isKeyPressed(key);
	}

	inline unsigned seed(unsigned fresh)
	{
		return fresh;
	}

#else

	struct KeyEvent
	{
		int frame;
		sf::Keyboard::Key key;
		bool down;
	};

	struct State
	{
		bool tracing = false;
		std::filesystem::path folder;
		std::vector<int> frames;
		std::vector<KeyEvent> keys;
		unsigned seed = 1;

		int frame = 0;
		std::size_t nextFrame = 0;
		std::size_t nextKey = 0;
		std::set<sf::Keyboard::Key> held;
	};

	inline State& state()
	{
		static State instance;
		return instance;
	}

	[[noreturn]] inline void fail(const std::string& message)
	{
		std::fprintf(stderr, "error: %s\n", message.c_str());
		std::exit(2);
	}

	inline std::vector<std::string> split(const std::string& text, char separator)
	{
		std::vector<std::string> parts;
		std::string part;
		for (const char letter : text)
		{
			if (letter == separator)
			{
				parts.push_back(part);
				part.clear();
			}
			else
			{
				part += letter;
			}
		}
		parts.push_back(part);
		return parts;
	}

	inline int wholeNumber(const std::string& text)
	{
		if (text.empty() || text.size() > 9 || text.find_first_not_of("0123456789") != std::string::npos)
		{
			fail("'" + text + "' is not a whole number");
		}
		return std::stoi(text);
	}

	// A key as <input button="..."> names it: the ones a game is likely to use.
	inline sf::Keyboard::Key keyNamed(const std::string& name)
	{
		if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z')
		{
			return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::A) + (name[0] - 'a'));
		}
		if (name.size() == 4 && name.compare(0, 3, "num") == 0 && name[3] >= '0' && name[3] <= '9')
		{
			return static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num0) + (name[3] - '0'));
		}

		struct Named
		{
			const char* name;
			sf::Keyboard::Key key;
		};
		static const Named names[] = {
			{ "space", sf::Keyboard::Key::Space }, { "enter", sf::Keyboard::Key::Enter },
			{ "escape", sf::Keyboard::Key::Escape }, { "tab", sf::Keyboard::Key::Tab },
			{ "left", sf::Keyboard::Key::Left }, { "right", sf::Keyboard::Key::Right },
			{ "up", sf::Keyboard::Key::Up }, { "down", sf::Keyboard::Key::Down },
			{ "lshift", sf::Keyboard::Key::LShift }, { "lcontrol", sf::Keyboard::Key::LControl },
		};
		for (const Named& entry : names)
		{
			if (name == entry.name) { return entry.key; }
		}

		fail("the key '" + name + "' cannot be pressed by the trace (a-z, num0-num9, space, enter, escape, tab, the arrows, lshift, lcontrol)");
	}

	inline void parseFrames(const std::string& text)
	{
		for (const std::string& part : split(text, ','))
		{
			const int frame = wholeNumber(part);
			if (frame < 1) { fail("frames are counted from 1, not '" + part + "'"); }
			state().frames.push_back(frame);
		}
		std::sort(state().frames.begin(), state().frames.end());
		state().frames.erase(std::unique(state().frames.begin(), state().frames.end()), state().frames.end());
	}

	inline void parseKeys(const std::string& text)
	{
		for (const std::string& part : split(text, ','))
		{
			const std::size_t sign = part.find_first_of("+-");
			if (sign == std::string::npos || sign == 0 || sign + 1 >= part.size())
			{
				fail("'" + part + "' is not a frame, + or -, and a key (like 5+space)");
			}

			const int frame = wholeNumber(part.substr(0, sign));
			if (frame < 1) { fail("frames are counted from 1, not '" + part + "'"); }
			state().keys.push_back({ frame, keyNamed(part.substr(sign + 1)), part[sign] == '+' });
		}
		std::stable_sort(state().keys.begin(), state().keys.end(), [](const KeyEvent& a, const KeyEvent& b) { return a.frame < b.frame; });
	}

	// The sounds the game played, as "frame 0012 wall", for sounds.txt.
	inline std::vector<std::string>& sounds()
	{
		static std::vector<std::string> heard;
		return heard;
	}

	// A sound the game played in the frame now being played: told here and not
	// played, so a trace is silent.
	inline void heard(const std::string& name)
	{
		char frame[16];
		std::snprintf(frame, sizeof frame, "%04d ", state().frame + 1);
		sounds().push_back(std::string("frame ") + frame + name);
	}

	// Writes sounds.txt, one line to a sound heard, the lines of a frame in
	// order of name (xgecli writes the same).
	inline void writeSounds()
	{
		std::filesystem::create_directories(state().folder);
		std::sort(sounds().begin(), sounds().end());

		std::ofstream out(state().folder / "sounds.txt", std::ios::binary);
		for (const std::string& line : sounds())
		{
			out << line << '\n';
		}
		if (!out) { fail("could not write sounds.txt"); }
	}

	// Where one thing is, for state0030.txt: its name and where its top left is.
	struct Place
	{
		std::string name;
		float x;
		float y;
	};

	// Every thing that is shown and in play, in the order of the game file: the
	// game sets this (tracePlaces in main.cpp), and the trace writes it with each
	// frame it keeps, to be compared with the engine's.
	inline std::vector<Place> (*placesOf)() = nullptr;

	inline void writeState(int frame)
	{
		if (placesOf == nullptr)
		{
			return;
		}

		char name[32];
		std::snprintf(name, sizeof name, "state%04d.txt", frame);
		std::ofstream out(state().folder / name, std::ios::binary);
		for (const Place& place : placesOf())
		{
			char line[256];
			std::snprintf(line, sizeof line, "%s %.3f %.3f\n", place.name.c_str(), static_cast<double>(place.x), static_cast<double>(place.y));
			out << line;
		}
	}

	// Reads --trace, --frames, --keys and --seed.
	inline void start(int count, char* arguments[])
	{
		for (int i = 1; i < count; ++i)
		{
			const std::string option = arguments[i];
			const bool hasValue = i + 1 < count;
			if (option == "--trace" && hasValue) { state().folder = arguments[++i]; state().tracing = true; }
			else if (option == "--frames" && hasValue) { parseFrames(arguments[++i]); }
			else if (option == "--keys" && hasValue) { parseKeys(arguments[++i]); }
			else if (option == "--seed" && hasValue) { state().seed = static_cast<unsigned>(wholeNumber(arguments[++i])); }
			else { fail("unknown option or no value for '" + option + "' (--trace <folder> --frames <list> [--keys <list>] [--seed <n>])"); }
		}

		if (state().tracing && state().frames.empty())
		{
			fail("--trace needs --frames, the frames to keep (like --frames 10,60)");
		}
		if (!state().tracing && (!state().frames.empty() || !state().keys.empty()))
		{
			fail("--frames and --keys are for --trace");
		}
	}

	inline bool keyHeld(sf::Keyboard::Key key)
	{
		return state().tracing ? state().held.count(key) > 0 : sf::Keyboard::isKeyPressed(key);
	}

	// The seed to draw random numbers from: the one asked for when tracing,
	// otherwise a fresh one.
	inline unsigned seed(unsigned fresh)
	{
		return state().tracing ? state().seed : fresh;
	}

	inline void put16(std::ofstream& out, unsigned value)
	{
		out.put(static_cast<char>(value & 0xff));
		out.put(static_cast<char>((value >> 8) & 0xff));
	}

	inline void put32(std::ofstream& out, unsigned value)
	{
		put16(out, value & 0xffff);
		put16(out, value >> 16);
	}

	// A 24 bit BMP, the file xgecli --trace writes (lib/source/trace.cpp).
	inline void writeFrame(const sf::Image& image, int frame)
	{
		std::filesystem::create_directories(state().folder);

		char name[32];
		std::snprintf(name, sizeof name, "frame%04d.bmp", frame);
		const std::filesystem::path file = state().folder / name;

		const unsigned width = image.getSize().x;
		const unsigned height = image.getSize().y;
		const unsigned row = (width * 3 + 3) / 4 * 4;

		std::ofstream out(file, std::ios::binary);
		if (!out) { fail("could not write " + file.string()); }

		out.write("BM", 2);
		put32(out, 54 + row * height);
		put32(out, 0);
		put32(out, 54);
		put32(out, 40);
		put32(out, width);
		put32(out, height);
		put16(out, 1);
		put16(out, 24);
		put32(out, 0);
		put32(out, row * height);
		put32(out, 2835);
		put32(out, 2835);
		put32(out, 0);
		put32(out, 0);

		const std::uint8_t* pixels = image.getPixelsPtr();
		std::vector<char> line(row, 0);
		for (unsigned y = height; y-- > 0;)
		{
			for (unsigned x = 0; x < width; ++x)
			{
				const std::uint8_t* from = pixels + (static_cast<std::size_t>(y) * width + x) * 4;
				line[x * 3] = static_cast<char>(from[2]);
				line[x * 3 + 1] = static_cast<char>(from[1]);
				line[x * 3 + 2] = static_cast<char>(from[0]);
			}
			out.write(line.data(), static_cast<std::streamsize>(line.size()));
		}

		if (!out) { fail("could not write " + file.string()); }
	}

	// What the game draws on, in place of an sf::RenderWindow: a picture in
	// memory. Tracing, it plays the keys it was given and stops after the last
	// frame asked for; otherwise it also shows the picture in a window of its
	// own and passes that window's events on, so the game plays as usual.
	class Window : public sf::RenderTexture
	{
	public:
		Window(float width, float height, const std::string& title) :
			sf::RenderTexture(sf::Vector2u(static_cast<unsigned int>(width), static_cast<unsigned int>(height)))
		{
			if (!state().tracing)
			{
				real.emplace(sf::VideoMode(sf::Vector2u(static_cast<unsigned int>(width), static_cast<unsigned int>(height))), title);
			}
		}

		void setFramerateLimit(unsigned int limit)
		{
			if (real) { real->setFramerateLimit(limit); }
		}

		// A key held down is not pressed again and again, as in the engine.
		void setKeyRepeatEnabled(bool enabled)
		{
			if (real) { real->setKeyRepeatEnabled(enabled); }
		}

		bool isOpen() const
		{
			return real ? real->isOpen() : !finished;
		}

		void close()
		{
			if (real) { real->close(); }
			finished = true;
		}

		std::optional<sf::Event> pollEvent()
		{
			if (real) { return real->pollEvent(); }

			State& now = state();
			while (now.nextKey < now.keys.size() && now.keys[now.nextKey].frame == now.frame + 1
				&& now.keys[now.nextKey].down && now.held.count(now.keys[now.nextKey].key) > 0)
			{
				// a key that is down is not pressed again (no key repeat, as in the engine)
				++now.nextKey;
			}
			if (now.nextKey >= now.keys.size() || now.keys[now.nextKey].frame != now.frame + 1)
			{
				return std::nullopt;
			}

			const KeyEvent& event = now.keys[now.nextKey++];
			if (event.down)
			{
				now.held.insert(event.key);
				sf::Event::KeyPressed pressed;
				pressed.code = event.key;
				return sf::Event(pressed);
			}

			now.held.erase(event.key);
			sf::Event::KeyReleased released;
			released.code = event.key;
			return sf::Event(released);
		}

		void display()
		{
			sf::RenderTexture::display();

			if (real)
			{
				real->clear();
				real->draw(sf::Sprite(getTexture()));
				real->display();
				return;
			}

			State& now = state();
			++now.frame;
			if (now.nextFrame < now.frames.size() && now.frames[now.nextFrame] == now.frame)
			{
				writeFrame(getTexture().copyToImage(), now.frame);
				writeState(now.frame);
				++now.nextFrame;
			}
			finished = now.nextFrame >= now.frames.size();
			if (finished) { writeSounds(); }
		}

	private:
		std::optional<sf::RenderWindow> real;
		bool finished = false;
	};

#endif
}
