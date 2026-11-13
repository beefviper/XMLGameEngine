// pong.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Pong, from games/descriptions/pong.xml, written as a person would write
// it: the example of what the windows-cpp generator is to write for a game
// of states, texts, sounds, a score that is kept and a computer player (plan
// task A5, step 5). It decides what happens; the engine's own small
// functions do it: playOneFrame (play/frame.h) runs the frame in the
// engine's order and the verbs (play/verbs.h) move things. No Game, no
// reader, no XML.

#pragma once

#include "audio/sound.h"
#include "engine/keys.h"
#include "game/object.h"
#include "play/frame.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace pong
{
	class Pong : public xge::FrameRules
	{
	public:
		// The game as it starts, on its title.
		Pong();

		// <window> and <sounds>
		xge::WindowDesc window;
		std::vector<xge::SoundDesc> sounds;

		// Every object, in the file's order, which is the order they are
		// moved, met and drawn in: the game's own, then each state's.
		std::vector<xge::Object> objects;

		// What lasts from one game to the next (<keeps>): who plays the
		// right paddle and how well, read from the file and written to it at
		// every new game.
		void keepIn(const std::string& file);

		// A key went down (pressed) or came up: the state's <inputs>.
		void key(xge::KeyCode key, bool pressed);

		// One frame of play.
		void frame();

		// Once the window has measured what changed: the texts put where
		// their size puts them (centred, or the left score against the net).
		void place();

		// The sounds asked for since the last time.
		std::vector<std::string> takeSounds();

		// Drawn still, under the state on top (paused and game over show
		// the game as it stopped).
		bool still(const xge::Object& object) const;

		// What pong.xml says should happen, as playOneFrame asks.
		bool inPlay(const xge::Object& object) override;
		void atEdge(xge::Object& object, xge::Edge edge) override;
		bool sticksAt(const xge::Object& object, xge::Edge edge) override;
		bool canCollide(const xge::Object& a, const xge::Object& b) override;
		bool hasRules(const xge::Object& object) override;
		void touched(xge::Object& a, xge::Object& b, const xge::Contact& contact) override;
		void conditions() override;

	private:
		enum class State { Title, Playing, Paused, Over };
		static const char* nameOf(State state);

		// The states, the one on top played: the title at the bottom.
		std::vector<State> states{ State::Title };
		State top() const { return states.back(); }
		void push(State state);
		void pop();

		// Each state's keys, and the moves a key still down goes on with
		// when its state comes back (a paddle after a pause).
		void titleKey(xge::KeyCode key, bool pressed);
		void playingKey(xge::KeyCode key, bool pressed);
		void pausedKey(xge::KeyCode key, bool pressed);
		void overKey(xge::KeyCode key, bool pressed);
		void keyIn(State state, xge::KeyCode key, bool pressed);
		void holdAgain(xge::KeyCode key);
		void keysToState();

		// The keys down, the player's and the computer's, and the state
		// each was pressed in, so letting go undoes what that press did.
		std::set<xge::KeyCode> keysDown;
		std::map<xge::KeyCode, State> pressedIn;

		// Counted at each change of state, so the keys follow it once.
		unsigned stateChanges{ 0 };
		unsigned keysFollowed{ 0 };

		// <computer>: the keys it holds for the right paddle.
		void computerPlays();
		std::set<xge::KeyCode> computerHolds;

		// The paddle's action of the second player, which waits while the
		// computer plays.
		void secondPlayer(xge::Direction direction, bool pressed);

		// A point to a paddle, the ball served again.
		void point(xge::Object& paddle);
		void ballStart();
		void ballMeetsPaddle(xge::Object& ball, xge::Object& paddle, const xge::Touch& touch);
		void showScores();
		void showLevel();

		// A new game: everything as it started but what is kept.
		void newGame();
		void writeKept() const;
		std::string keptFile;

		std::vector<std::string> requested;
		void play(const char* sound) { requested.push_back(sound); }

		xge::Object& computer() { return objects[0]; }
		xge::Object& paddle1() { return objects[21]; }
		xge::Object& paddle2() { return objects[22]; }
		xge::Object& score1() { return objects[23]; }
		xge::Object& score2() { return objects[24]; }
		xge::Object& ball() { return objects[25]; }
		bool isBall(const xge::Object& object) const { return &object == &objects[25]; }
		bool isPaddle(const xge::Object& object) const { return object.objClass == "paddle"; }
	};
}
