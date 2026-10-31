// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "play/collision.h"
#include "game/command.h"
#include "play/frame.h"
#include "game/data.h"
#include "load/expr.h"
#include "game/object.h"
#include "audio/sound.h"
#include "game/states.h"

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <assert.h>

namespace xge
{
	// The interpreter: it reads the game file into objects and states, and
	// answers what the file says should happen (FrameRules), running the
	// verbs it names through CommandExecutor. It plays no part in how a
	// frame runs or what a verb does: playOneFrame (play/frame.h) and the verbs
	// (play/verbs.h, play/geometry.h) are shared with generated games.
	class Game : private FrameRules
	{
	public:
		// The game as written (see GameData), worked out and ready to run.
		explicit Game(GameData data);

		// The game in that file, read with the default XML library: the same
		// as Game(readGame(file)), see load/xml.h, which also picks another.
		// Defined in load/xml.cpp, so a program that has its game written
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

		// The state on top of the stack. Only good until the stack changes: a
		// caller that runs commands (which may push or pop) copies what it needs.
		const State& getCurrentState(void) const;

		// Every state the game file defines, in file order (the current one is
		// a copy of one of these, pushed on the state stack).
		const std::vector<State>& getStates(void) const noexcept;
		std::vector<Object>& getCurrentObjects(void) noexcept;

		// `self`'s collision rules about `other`, which it has touched as
		// `touch` says (its own edge, and the rest): the ones that hold run.
		// Half of a touch; a knock passed through a floor is only this half.
		void reactTo(Object& self, Object& other, const Touch& touch);

		// A value worked out now, from where everything is and how it is
		// going (see game_expr::liveNumbers), and every variable as it is.
		float evaluateNow(const RawValue& value, const std::string& where);
		// A path step's half that names something that changes, worked out
		// for the follower as the step sets off (self is the follower).
		float stepHalf(const RawValue& value, const Object& follower);

		// A command that reads something that changes (CmdLive), made again
		// from what is so now. A value that cannot be worked out (a division
		// by nothing is fine; an expression gone wrong is not) leaves it as it
		// was made at load, with one warning.
		Command makeNow(const CmdLive& live);

		// <set variable="target">: a variable of an object's (owner.name), or
		// one of the numbers every object has (owner.x, owner.y,
		// owner.velocity.x, owner.velocity.y, owner.heading), on an object or
		// on every member of a group. A variable of the object's own of that
		// name wins. Returns false, with a warning, for no such thing.
		bool setNumber(const VariableName& target, float value);

		// The buttons the current state's <computer> holds down this frame:
		// those its conditions that hold name in their <hold>s.
		std::set<KeyCode> computerButtons();

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

		// Every sound the game file describes (<sounds>), worked out; what an
		// Audio backend loads (see audio/audio.h, Engine).
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

		// The loops that should be playing now: the loopingSounds, and the
		// current state's <music> when it loops. What Engine plays.
		std::set<std::string> soundsOn(void) const;

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

		// <inc variable="owner.variable" /> (and <dec />, a negative amount):
		// adds the amount to that variable, and every text bound to it shows
		// the new number. A variable may go below zero; a <condition> with
		// atmost= is what notices it has run out.
		void addTo(const VariableName& target, float amount = 1.0f);

		// <copy variable="player.score" into="best.score" />: the second
		// variable takes the number the first holds now, and every text bound
		// to it shows it.
		void copyVariable(const VariableName& from, const VariableName& into);

		// What the game <keeps>, written out and read back, so that it lasts
		// from one run of the program to the next (Engine::keepIn): each kept
		// object's look and variables, as XML. Reading takes only what this
		// game still keeps and still has; a file that is not one is passed
		// over with a warning.
		void saveKept(std::ostream& out) const;
		void loadKept(std::istream& in);

		// The game's recorded <demo> (RawDemo::recorded false for none).
		const RawDemo& getDemo(void) const noexcept { return demo; }

		// <demo />: asks for the demo to play; Engine, which has the keys,
		// plays it (takeDemoRequest).
		void requestDemo(void) noexcept { demoAsked = true; }
		bool takeDemoRequest(void) noexcept { return std::exchange(demoAsked, false); }

		// Whether the game <keeps> anything.
		bool keepsAnything(void) const noexcept;

		// Goes up by one at every full reset (a new game).
		unsigned long resetCount(void) const noexcept { return resets; }

		// <turns><player state="..." />...</turns>: from now on that many
		// players take turns, each with a game of their own that starts as
		// the game is now (every object it does not keep, every timer, the
		// current scenes). The first player's turn begins (beginTurn). The
		// state current now is the one each turn is played in. A full reset
		// ends the turns.
		void startTurns(const std::vector<std::string>& playerStates);

		// <pass /> (out false) and <out /> (out true): the next player still
		// in takes over, their game coming back as they left it and the
		// state stack going back to the state the turns are played in, with
		// their state= pushed over it. With no turns, or no one else in,
		// nothing changes; when everyone is out, the turns are over.
		void passTurn(bool out);

		// True once after an <out /> handed the turn to another player: the
		// list of commands it was in stops there. Read and cleared by
		// whatever runs a list of commands in a state.
		bool takeTurnStop(void) noexcept;

		// Called just before a player's game is put away, so whatever the
		// keys held are doing to it (a ship moving) stops there and does not
		// carry on when their turn comes back. Engine sets it.
		void onTurnPutAway(std::function<void()> stop) { turnPutAway = std::move(stop); }

		// How many players take turns (0 without <turns>), and whose turn it
		// is, from 1.
		std::size_t playerCount(void) const noexcept { return turns.size(); }
		std::size_t currentPlayer(void) const noexcept { return turns.empty() ? 0 : turn + 1; }

		// <rank variable="entry.score" into="table" />: the entry takes its
		// place among the members of the group (the best first), by that
		// variable, the ones below it moving down a place (see CmdRank).
		void rank(const VariableName& entry, const std::string& table);

		// <reset object="objectName" /> from a state's <input>/<condition> (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

		// Bare <reset /> in a state's list (an <input>, a <condition>, a
		// state's <timer>; an object's own bare <reset /> only puts that
		// object back, see CommandExecutor::run). Objects the game <keeps>
		// are passed over.
		// Resets every object back to how the game loaded (position, velocity,
		// every <variable>), refreshes every bound text display, and collapses
		// the whole state stack back down to the very first state - the same
		// state Engine's constructor pushes via setCurrentState(0) - so
		// mainmenu -> playing -> gameover -> mainmenu -> ... doesn't grow the
		// stack forever.
		void resetAll();

		// <become sprite="..." style="..." object="name" />: every object of
		// that name or group that has the look shows it, in that palette.
		void become(const std::string& target, const std::string& sprite, const std::string& from = {}, const std::string& style = {});

		// <reveal object="name">count</reveal>: the first `count` objects of
		// that name or group that are out of play come back where they started.
		void reveal(const std::string& target, int count);

		// <spawn object="name">: a new object, a copy of the one of that name
		// as the file says it starts (or, for a group, of its members in
		// turn), named after it and a number (shot.1, shot.2), and in the
		// group of its name, so whatever names that one names the copy too.
		// Its own rules, timers and values that named the one it is copied
		// from (or self) name the copy. It is made now, its <random>s drawn,
		// and joins the game's objects when this frame is over (or the next
		// one starts): until then it is not in play. The caller puts it where
		// it goes. Null, and nothing made, when the object has a <most> and
		// that many copies of it exist, or when no object has that name.
		Object* spawn(const std::string& target, bool random = false);

		// <despawn />: the object is no longer in the game. A copy that was
		// spawned is gone for good when the frame is over; one the file wrote
		// stays among the objects, not existing (Object::exists), until a
		// reset. Whatever carried it lets go.
		void despawn(Object& object);

		// <despawn object="name" />: every object of that name or group.
		void despawn(const std::string& target);

		// Whether the objects of that name (or group) are all ones the file
		// says do not exist at the start (<exists>false</exists>): what a
		// <fire> or a <release> makes copies of, rather than taking from a
		// pool of them.
		bool isDescription(const std::string& name);

		// The objects spawned and not yet joined (spawn), for a front end or
		// a test that wants to see one before the frame is over.
		const std::deque<Object>& justSpawned(void) const noexcept { return spawned; }

		// --- A map with <cells /> (game/cells.cpp). Its pieces move a cell
		// at a time; a move that is refused (a cell off the map, or holding
		// another's piece) touches what refused it: the map's edge, or the
		// piece in the way.

		// A spawned copy on a cell map, put in that cell (its top left one,
		// for a shape, whose cells are spawned with it); false, and the
		// copy taken back, when a cell it needs is off the map or full.
		// `map`: another map of cells it goes on instead of its own.
		bool putOnCells(Object& made, int column, int row, const std::string& map = {});

		// <send>: the first piece of that name or group on map `from` (a
		// shape with its cells, or a loose piece) goes to map `into`, in
		// that cell (left out: the one it is in); false when there is none
		// or it has no room there.
		bool sendCells(const std::string& target, const std::string& from, const std::string& into, std::optional<int> column, std::optional<int> row);

		// The object (a shape, its cells with it) a whole number of cells
		// across and down; false, after the touch, when refused.
		bool stepCells(Object& mover, int across, int down);

		// That way a cell at a time until the next is refused (which
		// touches), all at once.
		void slideCells(Object& mover, Direction way);

		// A shape a quarter turn about the middle of its square, trying the
		// standard kicks when it does not fit where it is; false (and no
		// touch) when none fits.
		bool turnCells(Object& shape, bool clockwise);

		// <settle />: a shape's cells are left as loose pieces and the shape
		// is gone; then its map's checks are asked.
		void settle(Object& mover);

		// A held <move> on a cell map: one step on the press, then more while
		// it is held (see CmdMove::wait); let go, it stops.
		void holdCellMove(Object& object, const CmdMove& move, bool pressed);

		// What a check found, cleared (<clear />), and what is above it
		// brought down (<collapse />).
		void clearCells(const Object::CellMatch& match);
		void collapseCells(const Object::CellMatch& match);

		// Joins the objects spawned to the game's, and takes out the copies
		// despawned: what updateObjects does before and after a frame. A
		// caller that spawns or despawns between frames and wants it seen at
		// once calls it too.
		void settleSpawns(void);

		// <follow path="..." />: the object sets off along that path, unless it
		// is on one already (it finishes that first), after waiting `wait`
		// frames. Throws std::out_of_range for a path the game does not have
		// (a game file is checked for that when it loads).
		void follow(Object& object, const std::string& path, int wait = 0);

		// <follow path="..." object="name"><stagger>s</stagger></follow>: every
		// object of that name or group that is in play sets off, each `stagger`
		// seconds after the one before, in the order written.
		void follow(const std::string& target, const std::string& path, float stagger, int nearest = 0, const Object* from = nullptr);
		// <shift>: every object of that name or group, and the place it goes
		// home to, moved by `by`; one on a path only has its place moved.
		void shift(const std::string& target, Vector2f by);

		// Every path the game file describes (<paths>), worked out.
		const std::map<std::string, Path>& getPaths(void) const noexcept;

	private:
		// Cell maps (game/cells.cpp): every object of that name, joined or
		// just spawned; the piece in that cell of the map, leaving out the
		// pieces of `mover` (null for none); the cells of a mover (a shape's,
		// or itself); putting one in a cell; its map's checks, asked; and a
		// held move's next step, each frame (FrameRules::cellMoves).
		Object* findAnywhere(const std::string& name) noexcept;
		Object* cellHolder(const std::string& map, int column, int row, const std::vector<Object*>& mover);
		std::vector<Object*> cellsOf(Object& mover);
		void putInCell(Object& object, int column, int row);
		// Onto another map of cells: its place, tile, size and checks, as
		// any piece of that map has them (false for a map with none).
		bool moveToMap(Object& object, const std::string& map);
		void askChecks(const std::string& map);
		void cellMoves() override;

		// The variable of that name, or null when its owner or it is not there.
		float* variableOf(const VariableName& name) noexcept;

		// Marks the objects (and every member of the groups) of these names
		// as kept (Object::kept); a name that is no object stops the load.
		void keep(const std::vector<std::string>& names);

		// Writes where every object is and how it is going into the
		// expressions' live numbers, before one is worked out as the game runs.
		void refreshLive(void);

		// Each live number the expressions read, and whose it is and which
		// part, worked out the first time (objects keep their places in
		// `objects`); and the numbers of the touch a rule is answering.
		struct LiveLink { float* number; std::size_t object; std::size_t part; };
		std::vector<LiveLink> liveLinks;
		bool liveLinked = false;
		std::array<float*, 6> touchNumbers{};
		std::set<std::string> liveWarned;

		// The copy of an object's variables that texts read (load/calc), made
		// what the object holds: after a reset puts them back.
		void holdVariablesForExpressions(const Object& object);

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

		// What isShown asks of each object (runsIn the top state), kept
		// until the state stack, a scene or an object's scene changes
		// (stageChanged), or a frame starts or ends: `stage` counts those
		// changes, and runsInTopStage says which one each answer is for.
		// Asked several times an object a frame otherwise.
		std::vector<char> runsInTop;
		std::vector<unsigned long> runsInTopStage;
		unsigned long stage = 1;
		void stageChanged(void) noexcept { ++stage; }

		// How many objects the file wrote: these keep their places in
		// `objects` for the whole game, and the copies spawned come after.
		std::size_t loadedCount{ 0 };

		// Copies spawned this frame, waiting to join (settleSpawns); a deque,
		// so the one handed back by spawn stays where it is as more come.
		std::deque<Object> spawned;

		// What the file says of each copy, under the copy's name (what
		// drawStartAgain draws a <random> from), and, for a group spawned
		// from, which of its members is next.
		std::map<std::string, RawObject> copyRaws;
		std::map<std::string, std::size_t> spawnTurns;
		std::map<std::string, bool> descriptions;

		// After objects have joined or left: every answer kept by an
		// object's place in `objects` is worked out again.
		void rosterChanged(void);

		// tryGetObject's answers: names never change while a game runs.
		std::unordered_map<std::string, Object*> foundObjects;
		Object* findObject(const std::string& name) noexcept;

		// The objects a condition's class and name filter matches (both
		// fixed while a game runs), by index, kept per filter.
		std::map<std::pair<std::string, std::string>, std::vector<std::size_t>> filtered;
		const std::vector<std::size_t>& matching(const std::string& filterClass, const std::string& filterObject);
		unsigned long resets = 0;
		RawDemo demo;
		bool demoAsked = false;

		// FrameRules: what the game file says, asked by playOneFrame.
		bool inPlay(const Object& object) override { return isShown(object); }
		void timers() override { updateTimers(); }
		void followPaths() override { applyPaths(); }
		bool burn(Object& object, const std::string& variable) override;
		void atEdge(Object& object, Edge edge) override;
		bool sticksAt(const Object& object, Edge edge) override;
		bool canCollide(const Object& a, const Object& b) override;
		bool hasRules(const Object& object) override { return !object.collisionData.others.empty(); }
		void touched(Object& a, Object& b, const Contact& contact) override;
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

		// A <demo /> in a game with no <demo> stops the load.
		void checkDemo(void) const;

		// One player's own game while another plays (see startTurns): every
		// object (those the game keeps are left alone), every state's and
		// scene's timers, the current scenes; their state= and whether they
		// are out.
		struct Turn
		{
			std::vector<Object> objects;
			std::map<std::string, std::vector<Timer>> stateTimers;
			std::map<std::string, std::string> currentScenes;
			std::map<std::string, std::vector<Timer>> sceneTimers;
			std::string state;
			bool out = false;
		};
		std::vector<Turn> turns;
		std::size_t turn = 0;
		std::string turnsState;
		bool turnHandedOver = false;
		std::function<void()> turnPutAway;
		void putTurnAway(Turn& into) const;
		void takeTurnUp(const Turn& from);
		void beginTurn(void);

		// A state just begun (pushed, or switched to): its <music>, if it
		// plays once, is asked for.
		void beginMusic(void);

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

		// Whether a condition's test holds now: its <all> or <any> of
		// checks, a total over what it matches, or any one object it
		// matches passing it.
		bool holds(const Condition& condition);

		// Counts a frame on every object timer of what is shown and every
		// timer of the current state, and runs the commands of the ones that
		// go off (CommandExecutor::executeTimer).
		void updateTimers(void);

		// One frame off one timer; true when it has gone off.
		bool tickTimer(Timer& timer, const std::string& where);

		// A named object timer's time left, whole seconds rounded up (0 once
		// it has gone off), into the object's number of that name.
		void showTimeLeft(Object& object, const Timer& timer);

	public:
		// A number of seconds as frames of the game, at least one.
		int framesFor(float seconds) const noexcept;

	private:
	};
}
