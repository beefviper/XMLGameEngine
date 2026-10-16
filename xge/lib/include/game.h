// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "collision_detector.h"
#include "command.h"
#include "frame.h"
#include "game_data.h"
#include "game_expr.h"
#include "object.h"
#include "sound.h"
#include "states.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <cmath>
#include <assert.h>

namespace xge
{
	// The interpreter: it reads the game file into objects and states, and
	// answers what the file says should happen (FrameRules), running the
	// verbs it names through CommandExecutor. It plays no part in how a
	// frame runs or what a verb does: playOneFrame (frame.h) and the verbs
	// (verbs.h, geometry.h) are shared with generated games.
	class Game : private FrameRules
	{
	public:
		// The game as written (see GameData), worked out and ready to run.
		explicit Game(GameData data);

		// The game in that file, read with the default XML library: the same
		// as Game(readGame(file)), see game_xml.h, which also picks another.
		// Defined in game_xml.cpp, so a program that has its game written
		// out as C++ (a generated one) builds without any XML library.
		explicit Game(const std::string& file);

		void updateObjects(void);

		// Works out the <position> of every object that uses the size of a text
		// or image (name.width / name.height, its own or another's) from the
		// now-measured sizes, and again whenever one of them has changed since.
		// Engine calls it once its Window has measured sizes, and each frame
		// after that. See Object::positionUsesSize.
		void resolveSizeDependentPositions(void);
		void printGame(void);

		WindowDesc& getWindowDesc(void) noexcept;

		// Whether the object is in play: it moves, animates, counts its timers
		// and collides (and is drawn, unless it is <drawn>false</drawn>). It
		// is, when it is not hidden or dead, and is the current state's (or
		// of its current scene), or the current state shows it.
		bool isShown(const Object& object) noexcept;

		// Whether the object is drawn still, underneath what is in play: it is
		// not in play, and would be in a state the current one shows with
		// <show state="..." /> (the game behind its pause).
		bool isStill(const Object& object) noexcept;

		// Whether the object belongs to a scene that is not its state's current
		// one: left as it was, out of reach of a <fire>, a <release> or a
		// <reveal> that names it by a name every scene's has.
		bool isAway(const Object& object) const;

		// The current scene of that state (its path, playing.room1), or empty
		// for a state with no scenes.
		std::string currentScene(const std::string& state) const;

		// <enter scene="playing.room2" />: that scene becomes its state's
		// current one, and what the state's own objects (or the game's) carry
		// out of the last one goes with them. Throws std::out_of_range for a
		// scene the game does not have.
		void enterScene(const std::string& scene);

		// <reset scene="playing.room1" />: every object that started in that
		// scene back as it started (into it, if it was carried out), and its
		// timers started over.
		void resetScene(const std::string& scene);

		// Where the current scene's way of that name leads (a scene's path),
		// or empty, with a warning, when the current state's current scene
		// has no such way.
		std::string wayLeadsTo(const std::string& way) const;

		// <enter> and <reset scene> as written: by a scene's path, or by a
		// way of the current scene.
		void enter(const CmdEnterScene& command);
		void reset(const CmdResetScene& command);

		// The object of that name or <group> in play (shown, not hidden or
		// dead) whose middle is nearest `from`'s, leaving `from` out; null if
		// there is none. What <chase> and <aim> look for.
		const Object* nearestInPlay(const Object& from, const std::string& name);
		Object& getObject(const std::string& name);
		Object* tryGetObject(const std::string& name) noexcept;
		float getVariable(const std::string& name);
		State getCurrentState(void);

		// Every state the game file defines, in file order (the current one is
		// a copy of one of these, pushed on the state stack).
		const std::vector<State>& getStates(void) const noexcept;
		std::vector<Object>& getCurrentObjects(void) noexcept;

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);

		// Throws std::out_of_range for a name that is not one of the game's
		// states (a game file is checked for that when it loads).
		void pushState(const std::string& name);

		// Does nothing when only the state the game started in is left.
		void popState(void) noexcept;

		// For an object whose start draws a <random> (Object::startDrawsRandom):
		// draws its random <variable>s again, then works out its starting
		// position and velocity again from them, so coming back into play is a
		// new draw (a new serve, a new fall speed): a reset, a <reveal>, a
		// <release> and a <fire> all call it first. Anything else is left alone.
		void drawStartAgain(Object& object);

		// Pops the current state and pushes `name` in its place (the first
		// state too: it is replaced, never left empty). An empty name is a
		// plain pop.
		void popState(const std::string& name);

		// Goes up by one every time the state stack changes (any push or pop,
		// including resets and condition-driven changes), so Engine can tell
		// that held keys need re-evaluating without comparing state names.
		unsigned long stateChangeCount(void) const noexcept;

		void setObjectParam(const std::string& name, const std::string& param, const float& value);

		// Every sound the game file describes (<sounds>), worked out; what an
		// Audio backend loads (see audio.h, Engine).
		const std::vector<SoundDesc>& getSounds(void) const noexcept;

		// <play sound="..." />: the game only asks for a sound, and Engine,
		// which owns the Audio, plays what was asked for once a frame
		// (takeSoundRequests). Game itself never makes a noise, so it runs and
		// is tested without a sound device. A sound asked for twice in one
		// frame (two bricks broken at once) is played once.
		void requestSound(const std::string& name);

		// The sounds asked for since the last call, in the order asked, and
		// forgets them.
		std::vector<std::string> takeSoundRequests(void);

		// A sound that loops (<loop />) is not asked for once but switched on:
		// requestSound puts it among the loopingSounds, where it stays, playing
		// over and over, until silenceSound takes it out (a <silence>, the key
		// that started it let go, or a reset of the whole game). Engine plays
		// what is in the list each frame and stops what has left it.
		const std::set<std::string>& loopingSounds(void) const noexcept;

		// <silence sound="..." />: stops a sound, looping or not.
		void silenceSound(const std::string& name);

		// The sounds silenced since the last call, and forgets them.
		std::vector<std::string> takeSilenceRequests(void);

		// Whether the sound of that name loops; false for no such sound.
		bool soundLoops(const std::string& name) const noexcept;

		// Sets an object's own <variable> to a value (for a front end that lets
		// the user edit one), and refreshes every text display bound to it, the
		// same way <inc /> does. Returns false when there is no such object or
		// variable.
		bool setVariable(const std::string& objectName, const std::string& variableName, float value);

		// A member of a lockstep block (see <lockstep> in <collisions>) hit the left or
		// right screen edge: the whole block turns around and steps once in
		// its new direction, every member the same amount, so the formation
		// keeps its spacing.
		void updateLockstepObjects(const Object& object) noexcept;

		// Thin forwarders so CommandExecutor (which only sees Game through a few
		// public entry points) can reach the xml subsystem without it being
		// made public wholesale.
		void incrementText(const std::string& objectName, float amount = 1.0f);

		// <dec variable="owner.variable" />: takes 1 (or the amount) off, the same way, and refreshes the
		// same bound texts. A variable may go below zero; a <condition> with
		// atmost= is what notices it has run out.
		void decrementText(const std::string& objectName, float amount = 1.0f);

		// <copy variable="player.score" into="best.score" />: the second
		// variable takes the number the first holds now, and every text bound
		// to it shows it.
		void copyVariable(const std::string& from, const std::string& into);

		// What the game <keeps>, written out and read back, so that it lasts
		// from one run of the program to the next (Engine::keepIn): each kept
		// object's look and variables, as XML. Reading takes only what this
		// game still keeps and still has; a file that is not one is passed
		// over with a warning.
		void saveKept(std::ostream& out) const;
		void loadKept(std::istream& in);

		// Whether the game <keeps> anything.
		bool keepsAnything(void) const noexcept;

		// Goes up by one at every full reset (a new game).
		unsigned long resetCount(void) const noexcept { return resets; }

		// <rank variable="entry.score" into="table" />: the entry takes its
		// place among the members of the group (the best first), by that
		// variable, the ones below it moving down a place (see CmdRank).
		void rank(const std::string& entry, const std::string& table);

		// <reset object="objectName" /> from a state's <input>/<condition> (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

		// Bare <reset /> from a state's <input>/<condition> (see CmdReset,
		// handled this way only in CommandExecutor::executeInput). Objects
		// the game <keeps> are passed over.
		//
		// handled this way only in CommandExecutor::executeInput - a collision's
		// own bare <reset /> still just resets that one object's position).
		// Resets every object back to how the game loaded (position, velocity,
		// every <variable>), refreshes every bound text display, and collapses
		// the whole state stack back down to the very first state - the same
		// state Engine's constructor pushes via setCurrentState(0) - so
		// mainmenu -> playing -> gameover -> mainmenu -> ... doesn't grow the
		// stack forever.
		void resetAll();

		// <become sprite="..." object="name" />: every object of that name or
		// group that has the look shows it.
		void become(const std::string& target, const std::string& sprite, const std::string& from = {});

		// <reveal object="name">count</reveal>: the first `count` objects of
		// that name or group that are out of play come back where they started.
		void reveal(const std::string& target, int count);

		// <follow path="..." />: the object sets off along that path, unless it
		// is on one already (it finishes that first), after waiting `wait`
		// frames. Throws std::out_of_range for a path the game does not have
		// (a game file is checked for that when it loads).
		void follow(Object& object, const std::string& path, int wait = 0);

		// <follow path="..." object="name"><stagger>s</stagger></follow>: every
		// object of that name or group that is in play sets off, each `stagger`
		// seconds after the one before, in the order written.
		void follow(const std::string& target, const std::string& path, float stagger);

		// Every path the game file describes (<paths>), worked out.
		const std::map<std::string, Path>& getPaths(void) const noexcept;

	private:
		// The one implementation behind incrementText and decrementText;
		// `verb` is only for the warning printed when the target is not found.
		void changeVariable(const std::string& target, float delta, const char* verb);

		// Marks the objects (and every member of the groups) of these names
		// as kept (Object::kept); a name that is no object stops the load.
		void keep(const std::vector<std::string>& names);

		// Shows `value` in every text object bound to ownerName.variableName.
		void refreshBoundTexts(const std::string& ownerName, const std::string& variableName, float value);

		WindowDesc windowDesc;

		game_expr expr;

		// How the game file was checked against xgedef.xsd (GameData),
		// only ever read back by printGame().
		std::string validation;

		std::vector<std::pair<std::string, RawValue>> rawVariables;
		std::map<std::string, float> variables;
		std::vector<RawState> rawStates;
		std::vector<State> states;
		std::vector<RawObject> rawObjects;
		std::vector<Object> objects;
		std::vector<RawSound> rawSounds;
		std::vector<SoundDesc> sounds;
		std::vector<RawPath> rawPaths;
		std::map<std::string, Path> paths;
		std::vector<std::string> soundRequests;
		std::vector<std::string> silenceRequests;
		std::set<std::string> loops;
		std::stack<State> currentState;
		unsigned long stateChanges = 0;
		unsigned long resets = 0;

		// FrameRules: what the game file says, asked by playOneFrame.
		bool inPlay(const Object& object) override { return isShown(object); }
		void timers() override { updateTimers(); }
		void followPaths() override { applyPaths(); }
		bool burn(Object& object, const std::string& variable, const char* verb) override;
		void atEdge(Object& object, Edge edge) override;
		bool sticksAt(const Object& object, Edge edge) override;
		bool canCollide(const Object& a, const Object& b) override;
		void touched(Object& a, Object& b, Edge edgeOfB) override;
		void conditions() override { checkConditions(); }

		// Sets the velocity of every shown object that is on a path for this
		// frame's part of it (see Path), starting its next leg when one is
		// done and running that leg's commands; at the end of the path it
		// comes to rest. Before the edge checks and the move, so a follower
		// moves, wraps and collides like anything else.
		void applyPaths(void);

		// Whether `object` is right now touching any other object in play of
		// this class - what a collision rule's unless= asks.
		bool isTouchingClass(const Object& object, const Object& excluding, const std::string& objClass);

		// Whether an edge rule with sprite= or unless= is passed over right
		// now: the object shows another look, or touches that class.
		bool isEdgeRulePassedOver(const Object& object, const EdgeGuard& guard);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();

		void runConditionCommands(std::vector<Command> commands);

		// Every state's <timers> as they are counting (State::timers is how
		// they start); kept here, by state name, because the state stack
		// holds copies of states. A reset starts them all over.
		std::map<std::string, std::vector<Timer>> stateTimers;

		// Each state's current scene, by state name, and every scene's
		// timers as they are counting, by its path. Goes up by one each time
		// a scene is entered (a timer or condition list stops for the frame).
		std::map<std::string, std::string> currentScenes;
		std::map<std::string, std::vector<Timer>> sceneTimers;
		unsigned long sceneChanges = 0;

		// Every state at its first scene, and every scene's timers from the start.
		void startScenes(void);

		// Whether the object would be in play were `state` the current one
		// (hidden or not).
		bool runsIn(const Object& object, const State& state) const;

		// Whether a condition that names no object looks at it: the game's
		// own, and the current state's in play or in its current scene.
		bool inScope(const Object& object) const;

		// The full name a name means: itself, when it names an object or a
		// group, or else the one path that ends with it (robots for
		// playing.room1.robots, for a test or a front end); else itself.
		std::string fullName(const std::string& name) const;

		// Works out what is shown in every text bound to a variable, after many have changed.
		void refreshAllBoundTexts(void);

		// Puts a text that shows variables together again from what they hold now.
		void relabel(Object& object);

		// The conditions of one list, as checkConditions does; true when one
		// fired (and the frame's checking is over).
		bool checkConditionList(const std::vector<Condition>& conditions);

		// Counts a frame on every object timer of what is shown and every
		// timer of the current state, and runs the commands of the ones that
		// go off (CommandExecutor::executeTimer).
		void updateTimers(void);

		// One frame off one timer; true when it has gone off.
		bool tickTimer(Timer& timer, const std::string& where);

	public:
		// A number of seconds as frames of the game, at least one.
		int framesFor(float seconds) const noexcept;

	private:
	};
}
