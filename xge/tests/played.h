// played.h
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// What the tests of one game's worth of play share: a game played with no
// window, frame by frame, and a game of one state written out from a few
// objects to be loaded and played.

#pragma once

#include "scratch.h"
#include "engine/engine.h"
#include "game/game.h"
#include "play/collision.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace played
{
	class FakeWindow : public xge::Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<xge::Object>&) override {}
		std::vector<std::pair<xge::KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(xge::Object&) override {}
		void display() override {}
	};

	// A game played with no window, each shape the size its sprite implies.
	struct Played
	{
		xge::Game game;
		xge::Engine engine;

		explicit Played(const std::string& file) : game(file), engine(game, std::make_unique<FakeWindow>())
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = xge::measureShapeSize(object.picture);
			}
		}

		void tap(xge::KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		std::string state() { return game.getCurrentState().name; }
		xge::Object& get(const std::string& name) { return game.getObject(name); }

		// Whether an object of that name is in the game (a spawned copy,
		// shot.1, is there from the end of the frame it was spawned in).
		bool has(const std::string& name)
		{
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.exists && calls(object.name, name)) { return true; }
			}
			return false;
		}

		// Whether a full name (playing.charge.1) is that name, or ends with
		// it after a dot (charge.1).
		static bool calls(const std::string& full, const std::string& name)
		{
			return full == name || (full.size() > name.size() && full.compare(full.size() - name.size(), name.size(), name) == 0
				&& full[full.size() - name.size() - 1] == '.');
		}

		// How many of that name or group are in the game: what was spawned
		// of it, and is still there. The name may be the end of a path
		// (charge for playing.charge).
		int count(const std::string& name)
		{
			const auto is = [&](const std::string& full) { return calls(full, name); };
			int found = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (!object.exists) { continue; }
				bool named = is(object.name) || is(object.groupName);
				for (const std::string& group : object.outerGroups) { named = named || is(group); }
				if (named) { ++found; }
			}
			return found;
		}
	};

	// A game of one state, "playing", with these objects, these commands on
	// the space bar and whatever else the state is given, and these pieces in
	// its <kit>, written out to be loaded and played, and taken away again.
	class Scratch
	{
	public:
		Scratch(const std::string& objects, const std::string& onSpace, const std::string& more = "", const std::string& kit = "")
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"scratch\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				<< (kit.empty() ? std::string{} : "<kit>" + kit + "</kit>") <<
				"<states><state name=\"playing\"><objects>" << objects << "</objects>"
				"<inputs><input button=\"space\">" << onSpace << "</input></inputs>" << more << "</state></states></game>";
		}
		~Scratch()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
		Scratch(const Scratch&) = delete;
		Scratch& operator=(const Scratch&) = delete;

		const std::string path = scratchName("descriptions/played_scratch.xml");
	};

	// An object of a 10 by 10 rectangle at x, y, and whatever else it is given.
	inline std::string box(const std::string& name, float x, float y, const std::string& rest = "")
	{
		return "<object name=\"" + name + "\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>" + rest + "</object>";
	}
}
