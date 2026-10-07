// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "collision_detector.h"
#include "command.h"
#include "game_xml.h"
#include "game_expr.h"
#include "object.h"
#include "sound.h"
#include "states.h"
#include "xml_document.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <cmath>
#include <assert.h>

namespace xge
{
	class Game
	{
	public:
		// xmlBackend picks which XmlDocument implementation actually parses
		// the file (Xerces, TinyXML2, PugiXML, or RapidXML - see
		// xml_document.h); defaults to Xerces, or the first one built when Xerces
		// is not (XmlDocumentFactory::defaultBackend), so existing callers
		// (main.cpp) don't have to name one - same pattern as Engine's own
		// WindowBackend parameter.
		explicit Game(const std::string& game, XmlBackend xmlBackend = XmlDocumentFactory::defaultBackend());

		void updateObjects(void);

		// Works out the <position> of every object that uses the size of a text
		// or image (name.width / name.height, its own or another's) from the
		// now-measured sizes, and again whenever one of them has changed since.
		// Engine calls it once its Window has measured sizes, and each frame
		// after that. See Object::positionUsesSize.
		void resolveSizeDependentPositions(void);
		void printGame(void);

		WindowDesc& getWindowDesc(void) noexcept;
		bool isShown(const Object& object) noexcept;

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
		// position and velocity again from them, so a reset is a new draw (a
		// new serve, a new fall speed). Anything else is left alone.
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

		// <reset object="objectName" /> from a state's <input>/<condition> (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

		// Bare <reset /> from a state's <input>/<condition> (see CmdReset,
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
		void become(const std::string& target, const std::string& sprite);

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

		// Shows `value` in every text object bound to ownerName.variableName.
		void refreshBoundTexts(const std::string& ownerName, const std::string& variableName, float value);

		std::string filename;
		WindowDesc windowDesc;

		game_xml xml;
		game_expr expr;

		// How xml above checked this game's file against
		// xgedef.xsd - set once, by xml.init() inside the
		// constructor, and only ever read back by printGame() (see
		// xml_document.h for what each value means).
		SchemaValidation xmlValidation = SchemaValidation::None;

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
		std::stack<State> currentState;
		unsigned long stateChanges = 0;

		void checkEdge(Object& object, Edge edge);

		// After the frame's move: pushes an object back inside any screen edge
		// it has a stick() rule for, even if it has stopped moving.
		void keepStuckObjectInBounds(Object& object);

		// Whether a and b have anything to do about touching each other.
		static bool canCollide(const Object& a, const Object& b) noexcept;

		// How far an object moves in one frame: its own velocity, plus the
		// velocity of whatever it is riding (see Object::carry).
		static Vector2f motionOf(const Object& object) noexcept;

		// Whether it is in motion at all this frame: moving, being carried, or
		// having just made a hop.
		static bool isMoving(const Object& object) noexcept;

		// Sets the velocity of every shown object that is on a path for this
		// frame's part of it (see Path), starting its next leg when one is
		// done and running that leg's commands; at the end of the path it
		// comes to rest. Before the edge checks and the move, so a follower
		// moves, wraps and collides like anything else.
		void applyPaths(void);

		// Changes every shown object's velocity by its <acceleration> and by
		// whatever <accelerate> thrust is being held (burning its fuel), once a
		// frame, before anything moves: a constant pull, or a thruster.
		void applyClimbing(void);
		void applyAcceleration(void);

		// The start of a frame's move: makes every hop queued by a hop.*()
		// action, unless it would leave the window, and forgets last frame's
		// carrying (this frame's collisions work it out again).
		void applyHops(void);

		// Whether `object` is right now touching any other object in play of
		// this class - what a collision rule's unless= asks.
		bool isTouchingClass(const Object& object, const Object& excluding, const std::string& objClass);

		// Moves everything shown by its velocity for one frame, stopping at each
		// touch that has a rule on the way to run it - see the definition.
		void moveObjects(void);

		// Runs both objects' rules for touching each other; edgeOfB is the edge
		// of b that was hit.
		void applyObjectCollision(Object& a, Object& b, Edge edgeOfB);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();

		void runConditionCommands(std::vector<Command> commands);

		// Every state's <timers> as they are counting (State::timers is how
		// they start); kept here, by state name, because the state stack
		// holds copies of states. A reset starts them all over.
		std::map<std::string, std::vector<Timer>> stateTimers;

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
