// played.h
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// What the tests of one game's worth of play share: a game played with no
// window, frame by frame, and a game of one state written out from a few
// objects to be loaded and played.

#pragma once

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
				object.size = xge::measureShapeSize(object.spriteParams, object.shapeKind);
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
	};

	// A game of one state, "playing", with these objects, these commands on
	// the space bar and whatever else the state is given, written out to be
	// loaded and played, and taken away again.
	class Scratch
	{
	public:
		Scratch(const std::string& objects, const std::string& onSpace, const std::string& more = "")
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
				"<window name=\"scratch\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
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

		const std::string path = "descriptions/played_scratch.xml";
	};

	// An object of a 10 by 10 rectangle at x, y, and whatever else it is given.
	inline std::string box(const std::string& name, float x, float y, const std::string& rest = "")
	{
		return "<object name=\"" + name + "\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>" + rest + "</object>";
	}
}
