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
#include "game/parts.h"
#include "game/object.h"
#include "audio/sound.h"
#include "game/states.h"
#include "game/think.h"
#include "game/cells.h"
#include "game/holders.h"
#include "game/players.h"
#include "game/sounds.h"

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
#include <memory>
#include <optional>
#include <tuple>
#include <assert.h>

namespace xge
{

	class CommandExecutor;

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

		// Not copied or moved: what it is made of holds on to its parts.
		Game(const Game&) = delete;
		Game& operator=(const Game&) = delete;
		Game(Game&&) = delete;
		Game& operator=(Game&&) = delete;

		// Plays one step of `seconds` (playOneFrame).
		void updateObjects(float seconds);

		// A computer's <think> searches on a thread of its own, so the game
		// goes on (drawn, heard) while it thinks; off, it searches in the
		// frame, the same each run (the tests). Engine::loop turns it on.
		void thinkInBackground(bool aside) noexcept { thinker.inBackground(aside); }
		// Maps with <cells /> and the board games on them.
		CellMaps& cellMaps() noexcept { return cellMapping; }
		Holders& holders() noexcept { return itemHolders; }
		Players& players() noexcept { return playerGroups; }
		Sounds& sounds() noexcept { return soundBoard; }

		// The full name a name means: itself, when it names an object or a
		// group, or else the one path that ends with it (robots for
		// playing.room1.robots, for a test or a front end); else itself.
		Name fullName(Name name) const;
		// The copies spawned this frame, which join the game when it ends.
		const std::deque<Object>& spawning() const noexcept { return spawned; }
		std::deque<Object>& spawning() noexcept { return spawned; }
		// Every object of that name, joined or just spawned.
		Object* findSpawning(Name name) noexcept;
		// The variable of that name, or null when its owner or it is not there.
		float* variableOf(const VariableName& name) noexcept;
		// How many objects the game loaded with (copies come after).
		std::size_t loadedObjectCount() const noexcept { return loadedCount; }
		// A copy's written form is no longer needed (it was taken back).
		void forgetCopy(Name name) { writtenCopies.erase(name); }
		// What a piece is, for a <line> of alike: its group, or for a copy
		// the group of what it copies.
		std::string kindOf(const Object& piece) noexcept;
		// Its rules about meeting that edge of the window run.
		void meetEdge(Object& object, Edge edge) { atEdge(object, edge); }

		// Works out the <position> of every object that uses the size of a text
		// (name.width / name.height, its own or another's) again, when one of
		// those sizes has changed since. The game calls it when it loads and
		// after a frame in which a text's words changed its size. See
		// Object::Layout.
		void resolveSizeDependentPositions(void);
		void printGame(void);

		WindowDesc& getWindowDesc(void) noexcept;

		// Every picture the game has loaded or made, each held once.
		const Resources& resources() const noexcept { return expr.resources; }
		// The expressions worked out while loading, and the names each reads.
		const Evaluator& evaluator() const noexcept { return expr; }
		// The <patterns> of that name; null when there are none.
		const Patterns* patternsNamed(Name name) const noexcept;

		// An object's variable is now `value`: expressions read it, and every
		// text that shows it is made again.
		void variableChanged(Name ownerName, Name variableName, float value);

		// Whether the object is in play: it moves, animates, counts its timers
		// and collides (and is drawn, unless it is <hidden />). It
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
		// Whether it is flying a <follow>'s path.
		bool isFollowing(const Object& object) const noexcept { return objectParts.onPaths.has(object.id); }

		// The current scene of that state (its path, playing.room1), or empty
		// for a state with no scenes.
		Name currentScene(Name state) const;

		// <enter scene="playing.room2" />: that scene becomes its state's
		// current one, and what the state's own objects (or the game's) carry
		// out of the last one goes with them. Throws std::out_of_range for a
		// scene the game does not have.
		void enterScene(Name scene);

		// <reset scene="playing.room1" />: every object that started in that
		// scene back as it started (into it, if it was carried out), and its
		// timers started over.
		void resetScene(Name scene);

		// Where the current scene's way of that name leads (a scene's path),
		// or empty, with a warning, when the current state's current scene
		// has no such way.
		Name wayLeadsTo(Name way) const;

		// <enter> and <reset scene> as written: by a scene's path, or by a
		// way of the current scene.
		void enter(const CmdEnterScene& command);
		void reset(const CmdResetScene& command);

		// The object of that name or <group> in play (shown, not hidden or
		// dead) whose middle is nearest `from`'s, leaving `from` out; null if
		// there is none. What <chase> and <aim> look for.
		const Object* nearestInPlay(const Object& from, Name name);

		// Whether the straight line from the middle of `from` to the middle of
		// `to` crosses nothing in play of the classes `past` names (<sees>).
		bool seesPast(const Object& from, const Object& to, const std::vector<Name>& past);

		// The object of that name, or the first member of that group; a short
		// name that ends one path finds it too. object throws when there is
		// none, find gives null.
		Object& object(Name name);
		Object* find(Name name) noexcept;

		// The object a handle names, among those in the game or spawned this
		// frame; nullptr once it has been despawned and gone.
		Object* find(ObjectId id) noexcept;

		// Every object's handle and the parts it has.
		Parts& parts() noexcept override { return objectParts; }
		Resources& resources() noexcept override { return expr.resources; }

		// Its holder part: whether it has one, and the part (std::out_of_range
		// for an object that is no holder).
		bool isHolder(const Object& object) const noexcept { return objectParts.holders.has(object.id); }
		Holder& holderOf(const Object& object) { return objectParts.holders.get(object.id); }
		const Holder& holderOf(const Object& object) const { return objectParts.holders.get(object.id); }

		// Its hand part (it picks up held items): whether it has one, and the part
		// (std::out_of_range for an object that is no hand).
		bool isHand(const Object& object) const noexcept { return objectParts.hands.has(object.id); }
		Hand& handOf(const Object& object) { return objectParts.hands.get(object.id); }
		const Hand& handOf(const Object& object) const { return objectParts.hands.get(object.id); }

		// Its cell part (Object::Cells): what it has as a piece of a cell
		// map, a chain or a shape; nothing (an empty part) for one with none.
		// cellPartFor gives it one.
		const Object::Cells& cellPart(const Object& object) const noexcept
		{
			static const Object::Cells none{};
			const Object::Cells* cells = objectParts.cells.find(object.id);
			return cells ? *cells : none;
		}
		Object::Cells& cellPartFor(const Object& object)
		{
			if (Object::Cells* cells = objectParts.cells.find(object.id)) { return *cells; }
			// Its column and row are live numbers from now on.
			liveLinked = false;
			liveLinks.clear();
			return objectParts.give(objectParts.cells, object.id);
		}
		bool isShape(const Object& object) const noexcept { return cellPart(object).shapeSize > 0; }
		// Its place on a cell map (Object::Cells::Place): an empty one for
		// one with no cell part; placeFor gives it one.
		const Object::Cells::Place& place(const Object& object) const noexcept { return cellPart(object).place; }
		Object::Cells::Place& placeFor(const Object& object) { return cellPartFor(object).place; }

		// Whether its <position> uses a text's size (Object::SizeUse).
		bool usesSize(const Object& object) const noexcept { return objectParts.sizeUses.has(object.id); }

		// Its item part (Object::Item): an empty one (no item) for one with
		// none. Only a held member has one, given at load.
		const Object::Item& itemOf(const Object& object) const noexcept
		{
			static const Object::Item none{};
			const Object::Item* found = objectParts.items.find(object.id);
			return found ? *found : none;
		}
		Object::Item& itemFor(const Object& object) { return objectParts.items.get(object.id); }

		// Its turns part (Object::Turns): an empty one (takes no turns) for
		// one with none.
		const Object::Turns& turnsOf(const Object& object) const noexcept
		{
			static const Object::Turns none{};
			const Object::Turns* found = objectParts.turns.find(object.id);
			return found ? *found : none;
		}

		// How it started (Parts::starts); startFor gives it one.
		const Moment& startOf(const Object& object) const noexcept { return xge::startOf(objectParts, object); }
		// Its look of that name, its timers that follow it started over
		// (showLookNamed); false if it has none.
		bool showLook(Object& object, Name name)
		{
			const bool shown = showLookNamed(objectParts, object, name);
			if (shown && objectParts.boundTexts.has(object.id)) { relabel(object); }
			return shown;
		}
		// How far each of its timers has counted: none for one without.
		std::vector<Timer::Clock>* clocksOf(const Object& object) noexcept { return objectParts.clocks.find(object.id); }
		Moment& startFor(const Object& object) { return objectParts.give(objectParts.starts, object.id); }

		// Its place on a <map> (Object::MapPlace): an empty one for one on
		// none; mapFor gives it one.
		const Object::MapPlace& mapOf(const Object& object) const noexcept { return xge::mapOf(objectParts, object); }
		Object::MapPlace& mapFor(const Object& object) { return objectParts.give(objectParts.mapPlaces, object.id); }
		// A piece of a cell map sent to another (MapPlace::home).
		bool awayFromHome(const Object& object) const noexcept { return object.onCells && mapOf(object).name != mapOf(object).home; }

		// The pieces of its text that show variables (Object::BoundText):
		// none for a text that shows none, and for anything else.
		const std::vector<TextPiece>& textPiecesOf(const Object& object) const noexcept
		{
			static const std::vector<TextPiece> none;
			const Object::BoundText* found = objectParts.boundTexts.find(object.id);
			return found ? found->pieces : none;
		}
		// Whether one of its text's pieces shows that variable of that object.
		bool showsVariable(const Object& object, Name ownerName, Name variableName) const noexcept
		{
			const std::vector<TextPiece>& pieces = textPiecesOf(object);
			return std::any_of(pieces.begin(), pieces.end(),
				[&](const TextPiece& piece) { return piece.owner == ownerName && piece.variable == variableName; });
		}

		// Its board-game moves (Object::Moves): none for one with no
		// <moves>, given at load.
		const Object::Moves& pieceMoves(const Object& object) const noexcept
		{
			static const Object::Moves none{};
			const Object::Moves* found = objectParts.moves.find(object.id);
			return found ? *found : none;
		}

		// Its picker part (Object::Picker): what it holds, where its last
		// move went, its marks; an empty part for one with none. pickerFor
		// gives it one.
		const Object::Picker& pickerOf(const Object& object) const noexcept
		{
			static const Object::Picker none{};
			const Object::Picker* picker = objectParts.pickers.find(object.id);
			return picker ? *picker : none;
		}
		Object::Picker& pickerFor(const Object& object) { return objectParts.give(objectParts.pickers, object.id); }

		// The name of what it carries (<carry />), empty for nothing.
		Name carrying(const Object& object) const noexcept
		{
			const Object::Carry* carry = objectParts.carries.find(object.id);
			return carry ? carry->what : Name{};
		}

		// Its maze part (<maze>), nullptr for one that walks none.
		Maze* mazeOf(const Object& object) { return objectParts.mazes.find(object.id); }

		// What its held keys drive: given on the first key that drives it,
		// gone when it stops or is reset.
		Drive& driveFor(const Object& object) { return objectParts.give(objectParts.drives, object.id); }
		void stopDrive(const Object& object) { objectParts.drives.remove(object.id); }
		const Parts& parts() const noexcept { return objectParts; }
		float getVariable(Name name);

		// The state on top of the stack. Only good until the stack changes: a
		// caller that runs commands (which may push or pop) copies what it needs.
		const State& getCurrentState(void) const;
		bool hasState() const noexcept { return !currentState.empty(); }

		// Every state the game file defines, in file order (the current one is
		// a copy of one of these, pushed on the state stack).
		const std::vector<State>& getStates(void) const noexcept;
		std::vector<Object>& getCurrentObjects(void) noexcept { return objects; }
		const std::vector<Object>& getCurrentObjects(void) const noexcept { return objects; }

		// `self`'s collision rules about `other`, which it has touched as
		// `touch` says (its own edge, and the rest): the ones that hold run.
		// Half of a touch; a knock passed through a floor is only this half.
		// What the rules that ran asked of a refused cell step.
		struct Reaction { bool through{ false }; };
		Reaction reactTo(Object& self, Object& other, const Touch& touch);
		// A rule naming other, made for a copy (shot.3) of the object it was
		// made for (shot): once, the first time that copy is touched.
		// A test made about the object `from`, made about `to`, a copy of it.
		void retargetCheck(Condition& check, Name from, Name to);

		// A value worked out now, from where everything is and how it is
		// going (see Evaluator::liveNumbers), and every variable as it is.
		float evaluateNow(const WrittenValue& value, const std::string& where);
		// The same for a value of that object's, saying it is the object's.
		float evaluateFor(const WrittenValue& value, const Object& object);
		// A path step's half that names something that changes, worked out
		// for the follower as the step sets off (self is the follower).
		float stepHalf(const WrittenValue& value, const Object& follower);

		// A command that reads something that changes (CmdLive), made again
		// from what is so now. A value that cannot be worked out (a division
		// by nothing is fine; an expression gone wrong is not) leaves it as it
		// was made at load, with one warning.
		Command makeNow(const CmdLive& live, const Object* self, const Object* other);
		// What makeNow names self and other in.
		std::vector<WrittenCommand> ownCommands;

		// <set variable="target">: a variable of an object's (owner.name), or
		// one of the numbers every object has (owner.x, owner.y,
		// owner.velocity.x, owner.velocity.y, owner.heading), on an object or
		// on every member of a group. A variable of the object's own of that
		// name wins. Returns false, with a warning, for no such thing.
		bool setNumber(const VariableName& target, float value);

		// The buttons the current state's <computer> holds down this frame:
		// those its conditions that hold name in their <hold>s. Into `taps`,
		// those a <think> presses once (down and up at once, so a person's
		// press of the same key straight after is never taken for it held).
		// `seconds` is the step's, which the computer's pace counts.
		std::set<KeyCode> computerButtons(std::set<KeyCode>& taps, float seconds);

		void setCurrentState(const int& index);
		void setCurrentState(Name name);

		// Throws std::out_of_range for a name that is not one of the game's
		// states (a game file is checked for that when it loads).
		void pushState(Name name);

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
		void popState(Name name);

		// Goes up by one every time the state stack changes (any push or pop,
		// including resets and condition-driven changes), so Engine can tell
		// that held keys need re-evaluating without comparing state names.
		unsigned long stateChangeCount(void) const noexcept;

		// Every sound the game file describes (<sounds>), worked out; what an
		// Audio backend loads (see audio/audio.h, Engine).
		const std::vector<SoundDesc>& getSounds(void) const noexcept;

		// Sets an object's own <variable> to a value (for a front end that lets
		// the user edit one), and refreshes every text display bound to it, the
		// same way <inc /> does. Returns false when there is no such object or
		// variable.
		bool setVariable(Name objectName, Name variableName, float value);

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

		// The game as it is now, as XML, for a person or a program to read
		// (xgecli --dump): a <moment> holding the frame number given, the
		// current state and its scene, the game's variables, and each object
		// in play with its name, class, group, look, position, velocity and
		// variables. Each line starts with `indent`. games/xgedump.xsd is its
		// schema.
		void describe(std::ostream& out, unsigned long frame, double seconds, const std::string& indent = "");

		// The step being played, set as it begins: a verb that heads for a
		// point (<chase>) stops there rather than going past inside it.
		void beginStep(float seconds) noexcept { stepSeconds = seconds; }
		float stepTime() const noexcept { return stepSeconds; }

		// The game's recorded <demo> (WrittenDemo::recorded false for none).
		const WrittenDemo& getDemo(void) const noexcept { return demo; }

		// <demo />: asks for the demo to play; Engine, which has the keys,
		// plays it (takeDemoRequest).
		void requestDemo(void) noexcept { demoAsked = true; }
		bool takeDemoRequest(void) noexcept { return std::exchange(demoAsked, false); }

		// <people>N</people>: the first N seats are people's, the others the
		// computer's. One until a title says otherwise. A players group
		// member's live `person` says whether its seat is a person's.
		void seatPeople(int count);
		bool personSeat(int seat) const noexcept { return seat <= people; }
		// Whether a press of `key` counts now, from a person or (byComputer)
		// from the computer: a key in a seat is that seat's player's alone;
		// in several (a seat from= another), the seat of the players group
		// member on its turn. A pointing control is only a person's.
		bool seatAnswers(KeyCode key, bool byComputer) const;
		bool seatAnswers(Axis axis) const;

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
		// The list of commands running now stops after this one (a <pass />
		// handed the turn to the next member of a group).
		void stopCommands() noexcept { turnHandedOver = true; }

		// Whether a condition's test holds now: its <all> or <any> of
		// checks, a total over what it matches, or any one object it
		// matches passing it; with `who`, every object its test holds for
		// (the last test of an <all>'s), for commands that name self.
		bool holds(const Condition& condition, std::vector<const Object*>* who = nullptr);

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
		void rank(const VariableName& entry, Name table);

		// <reset object="objectName" /> from a state's <input>/<condition> (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(Name name);

		// A bare <reset /> in a state's list: every object but those the game
		// <keeps> back as it loaded, every text shown again, and the state
		// stack back to the first state, so going round the states does not
		// grow it.
		void resetAll();

		// <become sprite="..." style="..." object="name" />: every object of
		// that name or group that has the look shows it, in that palette.
		void become(Name target, Name sprite, Name from = {}, Name style = {});

		// <reveal object="name">count</reveal>: the first `count` objects of
		// that name or group that are out of play come back where they started.
		void reveal(Name target, int count);

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
		Object* spawn(Name target, bool random = false, std::optional<int> member = std::nullopt);

		// <despawn />: the object is no longer in the game. A copy that was
		// spawned is gone for good when the frame is over; one the file wrote
		// stays among the objects, not existing (Object::exists), until a
		// reset. Whatever carried it lets go.
		void despawn(Object& object);

		// <despawn object="name" />: every object of that name or group.
		void despawn(Name target);

		// Whether the objects of that name (or group) are all ones the file
		// says do not exist at the start (<exists>false</exists>): what a
		// <fire> or a <release> makes copies of, rather than taking from a
		// pool of them.
		bool isDescription(Name name);

		// The objects spawned and not yet joined (spawn), for a front end or
		// a test that wants to see one before the frame is over.
		const std::deque<Object>& justSpawned(void) const noexcept { return spawned; }

		// A <condition> in an action: whether its test holds now.
		bool testHolds(const Condition& condition, const Object* self, const Object* other) { return holdsFor(condition, self, other); }

		// The copy of an object's variables that texts read (load/calc), made
		// what the object holds: after a reset puts them back.
		void shareVariables(const Object& object);
		// What is in play changed: what runs in the state is looked at again.
		void stageChanged(void) noexcept { ++stage; }

		// Joins the objects spawned to the game's, and takes out the copies
		// despawned: what updateObjects does before and after a frame. A
		// caller that spawns or despawns between frames and wants it seen at
		// once calls it too.
		void settleSpawns(void);
		// Spawned and despawned objects settled, and what is in play seen
		// again (stageChanged).
		void settle();
		// A text that grew or shrank (a score): the positions made from it.
		void placeByResizedTexts();

		// <follow path="..." />: the object sets off along that path, unless it
		// is on one already (it finishes that first), after waiting `wait`
		// seconds. Throws std::out_of_range for a path the game does not have
		// (a game file is checked for that when it loads).
		void follow(Object& object, Name path, float wait = 0.0f);

		// <follow path="..." object="name"><stagger>s</stagger></follow>: every
		// object of that name or group that is in play sets off, each `stagger`
		// seconds after the one before, in the order written.
		void follow(Name target, Name path, float stagger, int nearest = 0, const Object* from = nullptr);
		// <shift>: every object of that name or group, and the place it goes
		// home to, moved by `by`; one on a path only has its place moved.
		void shift(Name target, Vector2f by);

		// Every path the game file describes (<paths>), worked out.
		const std::map<std::string, Path>& getPaths(void) const noexcept;

	private:
		// A text's words changed its size since the positions made from text
		// sizes were last worked out (resolveSizeDependentPositions).
		bool textsResized = false;
		void cellMoves(float seconds) override;
		Thinker thinker;
		CellMaps cellMapping{ *this };
		Holders itemHolders{ *this };
		Players playerGroups{ *this };
		Sounds soundBoard{ *this };


		// Marks the objects (and every member of the groups) of these names
		// as kept (Object::kept); a name that is no object stops the load.
		void keep(const std::vector<Name>& names);

		// Writes where every object is and how it is going into the
		// expressions' live numbers, before one is worked out as the game runs.
		void refreshLive(void);

		// Each live number the expressions read, and whose it is and which
		// part, worked out the first time (objects keep their places in
		// `objects`).
		// Only those a text has read are kept up (Evaluator::liveRead).
		struct LiveLink { float* number; std::size_t object; std::size_t part; };
		std::vector<LiveLink> liveLinks;
		// For an object's name, the number each live part is read through
		// (null: no text reads it), found by text once while calc reads the
		// same live numbers.
		std::unordered_map<Name, std::vector<float*>> liveNumbersOf;
		std::size_t liveNumbersAt = 0;
		const std::vector<float*>& liveNumbersFor(Name owner);
		bool liveLinked = false;
		std::size_t liveLinkedAt = 0;
		std::set<std::string> liveWarned;

		// Where expressions read that object's variable (player.score), found
		// by its text once; null when none reads it.
		float* expressionSlot(Name owner, Name variable);
		std::unordered_map<VariableName, float*, VariableName::Hash> expressionSlots;
		std::size_t expressionSlotsOf = 0;


		WindowDesc windowDesc;

		Evaluator expr;

		// How the game file was checked against xgedef.xsd (GameData),
		// only ever read back by printGame().
		std::string validation;

		std::vector<std::pair<std::string, WrittenValue>> writtenVariables;
		std::map<std::string, float> variables;
		std::vector<WrittenState> writtenStates;
		std::vector<State> states;
		std::vector<WrittenObject> writtenObjects;
		std::vector<Object> objects;
		std::vector<WrittenSound> writtenSounds;
		std::vector<SoundDesc> soundDescs;
		std::vector<WrittenPath> writtenPaths;
		std::vector<Patterns> patterns;
		std::map<std::string, Path> paths;
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

		// How many objects the file wrote: these keep their places in
		// `objects` for the whole game, and the copies spawned come after.
		std::size_t loadedCount{ 0 };

		// Copies spawned this frame, waiting to join (settleSpawns); a deque,
		// so the one handed back by spawn stays where it is as more come.
		std::deque<Object> spawned;

		// What the file says of each copy, under the copy's name (what
		// drawStartAgain draws a <random> from), and, for a group spawned
		// from, which of its members is next.
		std::map<std::string, WrittenObject> writtenCopies;
		std::map<Name, std::size_t> spawnTurns;
		std::map<std::string, bool> descriptions;

		// After objects have joined or left: every answer kept by an
		// object's place in `objects` is worked out again.
		void rosterChanged(void);

		// Every object's handle and the parts it has (game/parts.h).
		Parts objectParts;

		// find's answers: names never change while a game runs.
		std::unordered_map<Name, Object*> foundObjects;
		// What fullName found for a name, until the objects change.
		mutable std::unordered_map<Name, Name> fullNames;
		Object* search(Name name) noexcept;

		// The objects a condition's class and name filter matches (both
		// fixed while a game runs), by index, kept per filter.
		struct ByTwoNames
		{
			std::size_t operator()(const std::pair<Name, Name>& names) const noexcept
			{
				return std::hash<Name>{}(names.first) * 31u + std::hash<Name>{}(names.second);
			}
		};
		std::unordered_map<std::pair<Name, Name>, std::vector<std::size_t>, ByTwoNames> filtered;
		const std::vector<std::size_t>& matching(Name filterClass, Name filterObject);
		unsigned long resets = 0;
		WrittenDemo demo;
		bool demoAsked = false;
		int people = 1;
		float stepSeconds = kSixtieth;

		// FrameRules: what the game file says, asked by playOneFrame.
		bool inPlay(const Object& object) override { return isShown(object); }
		void timers(float seconds) override { updateTimers(seconds); }
		void animationEnded(std::size_t index, std::size_t look) override;
		void frameCameUp(std::size_t index, std::size_t look, std::size_t picture) override;
		void followPaths(float seconds) override { applyPaths(seconds); }
		bool burn(Object& object, Name variable) override;
		void atEdge(Object& object, Edge edge) override;
		bool sticksAt(const Object& object, Edge edge) override;
		bool canCollide(const Object& a, const Object& b) override;
		std::vector<std::size_t> answerGroups(const std::vector<Object>& among) override;
		bool touchesByPicture(const Object& a, const Object& b) override;
		bool hasRules(const Object& object) override { return !object.kind->collisionData.others.empty(); }
		void touched(Object& a, Object& b, const Contact& contact) override;
		void follow() override;
		void conditions() override { itemHolders.scoreDueHolders(); cellMapping.countMoves(); cellMapping.showMarks(); checkConditions(); }

		// Sets the velocity of every shown object that is on a path for this
		// frame's part of it (see Path), starting its next leg when one is
		// done and running that leg's commands; at the end of the path it
		// comes to rest. Before the edge checks and the move, so a follower
		// moves, wraps and collides like anything else.
		void applyPaths(float seconds);

		// Whether `object` is right now touching any other object in play of
		// this class - what a collision rule's unless= asks.
		bool isTouchingClass(const Object& object, const Object& excluding, Name objClass);

		// Whether an edge rule with sprite= or unless= is passed over right
		// now: the object shows another look, or touches that class.
		bool isEdgeRulePassedOver(const Object& object, Name look, const EdgeGuard& guard);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();

		void runConditionCommands(std::vector<Command> commands);

		// Every state's <timers> as they are counting (State::timers is how
		// they start); kept here, by state name, because the state stack
		// holds copies of states. A reset starts them all over.
		std::unordered_map<Name, std::vector<Timer>> stateTimers;

		// Each state's current scene, by state name, and every scene's
		// timers as they are counting, by its path. Goes up by one each time
		// a scene is entered (a timer or condition list stops for the frame).
		std::unordered_map<Name, Name> currentScenes;
		std::unordered_map<Name, std::vector<Timer>> sceneTimers;
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
			Parts parts;
			std::unordered_map<Name, std::vector<Timer>> stateTimers;
			std::unordered_map<Name, Name> currentScenes;
			std::unordered_map<Name, std::vector<Timer>> sceneTimers;
			Name state;
			bool out = false;
		};
		std::vector<Turn> turns;
		std::size_t turn = 0;
		Name turnsState;
		bool turnHandedOver = false;
		std::function<void()> turnPutAway;
		void putTurnAway(Turn& into) const;
		// What a copy that leaves lets go of (settleSpawns).
		void letGoOwnPictures(const Object& object);
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

		Name findFullName(Name name) const;

		// Works out what is shown in every text bound to a variable, after many have changed.
		void relabelAll(void);

		// Puts a text that shows variables together again from what they hold now.
		void relabel(Object& object);

		// The conditions of one list, as checkConditions does; true when one
		// fired (and the frame's checking is over).
		bool checkConditionList(const std::vector<Condition>& conditions);

		// Whether it holds, tested by `self` (a rule's, an action's object):
		// what says self in it is that one.
		bool holdsFor(const Condition& condition, const Object* self, const Object* other = nullptr);
		const Object* testing{ nullptr };
		const Object* testingOther{ nullptr };
		// A name in the condition, with self the object testing it.
		Name selfIn(const Condition& condition, Name name) const noexcept;
		// The parts of holds.
		bool considers(const Condition& condition, const Object& object) const;
		float comparedWith(const Condition& condition);
		std::optional<float> measure(const Condition& condition, const Object& object);
		bool holdsTotal(const Condition& condition, const std::vector<std::size_t>& candidates, float against);
		bool holdsExtreme(const Condition& condition, const std::vector<std::size_t>& candidates, std::vector<const Object*>* who);

		// Counts `seconds` on every object timer of what is shown and every
		// timer of the current state, and runs the commands of the ones that
		// go off (CommandExecutor::run).
		void updateTimers(float seconds);

		// `owed` seconds off one timer; true when it has gone off, with what
		// the step had beyond that left in `owed` (0 when it did not go off),
		// to be paid by calling again once its commands have run.
		// Whose a timer is, for a message: an object, a state or a scene.
		struct TimerOwner { const char* kind; Name name; };
		bool tickTimer(const Timer& timer, Timer::Clock& clock, const TimerOwner& owner, float& owed);

		// A named object timer's time left, whole seconds rounded up (0 once
		// it has gone off), into the object's number of that name.
		void showTimeLeft(Object& object, const Timer& timer, const Timer::Clock& clock);
	};
}
