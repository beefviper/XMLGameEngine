// command.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "types.h"

#include <functional>
#include <memory>
#include <ostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace xge
{
	// A small std::visit helper: overload{lambdas...} builds a callable overload
	// set out of several lambdas, one per Command alternative.
	template <typename... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

	template <typename... Ts>
	overload(Ts...) -> overload<Ts...>;

	// What kind of sprite an Object was built from. Set once in game_expr::init()
	// from the object's spriteParams tag, so collision code no longer has to
	// re-derive it by searching the object's raw XML src string.
	enum class ShapeKind { Unknown, Circle, Rectangle, Text, Image, Line };

	// --- Command: what a command tag in the XML becomes once it is loaded.
	// makeCommand() (command.cpp) is the one place that turns a RawCommand
	// into one of these.

	struct CmdBounce {};
	struct CmdStick {};
	struct CmdReset {};
	struct CmdDie {};

	// <wrap /> - once the object has gone right off the screen through an edge,
	// it comes back in from the opposite one (see CommandExecutor::wrap).
	struct CmdWrap {};

	// <ride /> - in a collision rule about an object it is touching: the
	// object rides along with that other one, at its velocity, for as long
	// as they keep touching (see CommandExecutor::ride).
	struct CmdRide {};

	// <carry /> - in a collision rule about an object it is touching: the
	// object picks that one up and takes it wherever it goes, until it
	// <drop />s it (see Object::carrying). One thing at a time: picking up
	// another puts the first down, and taking one another object carries
	// takes it from that one.
	struct CmdCarry {};

	// <drop /> - the object puts down what it carries, where it is.
	struct CmdDrop {};

	// <track />, in an action an <input axis="..."> triggers: the object
	// goes where the control points, across the window (an Atari paddle).
	struct CmdTrack {};

	// <deflect>45</deflect> - in a collision rule about another object: a
	// bounce whose angle depends on where along the touched side of the other
	// object it hit. Square in the middle goes straight back out; at either
	// end it leaves at maxAngle degrees, towards that end; in between, in
	// proportion. The speed is kept (see CommandExecutor::deflect). A Pong
	// paddle.
	struct CmdDeflect
	{
		float maxAngle{};
	};

	struct CmdMove
	{
		Direction direction{};
		float step{};
	};

	// <inc variable="objectName" /> - add 1 to (or, for <inc variable="...">5</inc>, add the amount to) a text object's own displayed number, OR
	// <inc variable="ownerName.variableName" /> - increment that object's named <variable>
	// and refresh every text object whose display is bound to it (see
	// Game::incrementText and Object::textPieces).
	struct CmdIncrement
	{
		std::string target;
		float amount{ 1.0f };
	};

	// <copy variable="player.score" into="best.score" /> - puts the number one
	// variable holds now into another, and refreshes every text bound to that
	// one (see Game::copyVariable). Both are owner.variable. A high score:
	// a condition copies the score into the best whenever it is above it.
	struct CmdCopy
	{
		std::string from;
		std::string into;
	};

	// <rank variable="entry.score" into="table" /> - the entry goes into a
	// table of the best, a <group> whose members are its places, the best
	// first: it takes the place of the first member whose variable of that
	// name is below its own, with every variable it has that the member has,
	// and the members from there on move down a place, the last one falling
	// off. Below them all, nothing happens. See Game::rank.
	struct CmdRank
	{
		std::string entry;     // owner.variable: the entry and what orders it
		std::string table;     // the group
	};

	// <turns><player state="player1" /><player state="player2" /></turns> -
	// the game from here is played by that many players taking turns, each
	// with a game of their own, all starting as the game is now. The first
	// player plays first; each player's state= (optional) is pushed whenever
	// their turn comes up, a screen saying whose it is. See Game::startTurns.
	struct CmdTurns
	{
		std::vector<std::string> players;
	};

	// <demo /> - the game's recorded <demo> plays, the attract mode: a new
	// game, played by the keys recorded, until a key is pressed or the
	// recording ends, and then a new game again. See Engine.
	struct CmdDemo {};

	// <pass /> - the turn passes to the next player still in: this player's
	// game is put away as it is, theirs comes back. See Game::passTurn.
	struct CmdPass {};

	// <out /> - this player is out of the game, and the turn passes; when
	// another is still in, nothing after it in the list runs, and when none
	// is, what follows does (<out /><reset />). See Game::passTurn.
	struct CmdOut {};

	// <dec variable="ownerName.variableName" /> - the opposite of <inc />: takes 1 off that
	// object's named <variable> (or off a text object's own number) and
	// refreshes every text bound to it.
	struct CmdDecrement
	{
		std::string target;
		float amount{ 1.0f };
	};

	// <hop direction="up">distance</hop> (also down, left, right) - only ever in an
	// object's own <action>: a one-shot
	// jump of `distance` pixels, made once for each press of the key (see
	// CommandExecutor::triggerObjectAction and playOneFrame). Unlike
	// <move>, holding the key does nothing more, and a state change never
	// resumes it.
	struct CmdHop
	{
		Direction direction{};
		float distance{};
	};

	// <jump direction="down"><distance>40</distance><seconds>0.4</seconds></jump>
	// (also up, left, right) - only ever in an object's own <action>: once for
	// each press, the object travels `distance` pixels that way over `seconds`
	// (0.3 when left out), through the air: while it is in the air it touches
	// no other object, so it cannot be hit, ride or drown until it lands, and a
	// second jump waits for the landing. Unlike <hop> (an instant step) it
	// takes time and passes over what is between. See playOneFrame.
	struct CmdJump
	{
		Direction direction{};
		float distance{};
		float seconds{ 0.3f };
	};

	// <land /> - in a collision rule about another object: coming down onto
	// its top, the object stands on it. It is put on the top, its fall stops,
	// and for the next frame it is on the ground, which a <leap> needs. Touching
	// any other side, or going up, does nothing, so a platform is solid from
	// above only. See CommandExecutor::land.
	struct CmdLand {};

	// <leap>height</leap> - only ever in an object's own <action>: a jump up
	// under the object's own pull (its <acceleration>), rising `height`
	// pixels before it falls again. Only from the ground (after a <land />),
	// and the way across is fixed at the take-off: a held key does not steer
	// it until it lands. Donkey Kong's jump. See CommandExecutor::leap.
	struct CmdLeap
	{
		float height{};
	};

	// <climb direction="up" class="ladder">step</climb> (or down) - only ever
	// in an object's own <action>, held like a <move>: while the object stands
	// at, or is on, an object of that class, it goes up or down it `step`
	// pixels a frame, lined up with its middle, with no pull and no <land />
	// on the way, and no walking off it until it reaches either end. See
	// playOneFrame.
	struct CmdClimb
	{
		Direction direction{};
		float step{};
		std::string ladderClass;
	};

	// <chase object="player"><speed>1.5</speed><near>100</near></chase> - the
	// object heads straight for the middle of the nearest one in play of that
	// name or <group>, at `speed` pixels a frame; within `near` pixels of it
	// (0 when left out: never) it stops instead. Once: a timer that repeats
	// it keeps it on the trail. See CommandExecutor::chase.
	struct CmdChase
	{
		std::string target;
		float speed{};
		float stopWithin{}; // not "near": windows.h defines near (and far) as nothing
	};

	// <go direction="left">2</go> - only ever in an object's own <action>,
	// once for each press, for an object that walks a <maze>: it turns that
	// way at the first gap there is and keeps going, `speed` pixels a frame,
	// until a wall stops it; turned straight round, it goes back at once.
	// Pac-Man's stick. See maze.h.
	struct CmdGo
	{
		Direction direction{};
		float speed{};
	};

	// <hunt object="pacman"><speed>2</speed><ahead>4</ahead><with object="blinky"
	// /><retreat object="corner">8</retreat></hunt> - for an object that walks
	// a <maze>: from now on, at every tile, it takes the open way (never back
	// the way it came) whose next tile is nearest its target, as the arcade's
	// ghosts do. The target is the middle of the nearest one in play of that
	// name or <group>; `ahead` tiles in front of the way that one faces;
	// with a partner, as far past that point again as the partner is short of
	// it (Inky's pincer); and while the hunter is within `retreatWithin`
	// tiles of what it hunts, the middle of `retreatTo` instead (Clyde backing
	// off). Kept until another <hunt>, a <go>, a <wander> or a reset. See
	// maze.h.
	struct CmdHunt
	{
		std::string target;
		float speed{};
		float ahead{};
		std::string partner;
		std::string retreatTo;
		float retreatWithin{};
	};

	// <wander><speed>1</speed></wander> - for an object that walks a <maze>:
	// at every tile it takes an open way at random, never back the way it
	// came: a frightened ghost. Kept as a <hunt> is.
	struct CmdWander
	{
		float speed{};
	};

	// <aim object="player" /> - the object turns to the nearest one in play
	// of that name or <group>: its next <fire>s go straight at where that was,
	// at any angle, and its <facing> (or <heading>) turns that way. Kept until
	// it aims again, is reset, or a key moves it. See CommandExecutor::aim.
	struct CmdAim
	{
		std::string target;
	};

	// <become sprite="blue" /> - the object shows another of its named
	// <sprite>s from now on (its look; a reset goes back to the first), or,
	// with object="name", every object of that name or <group> does: a row of
	// ice turning blue when it is stood on, a light going green. A collision
	// rule with sprite="..." runs only while its object shows that sprite, so
	// a look is also a simple state. With from="normal", only an object
	// showing that look changes: the ghosts turn blue, but not the eyes of
	// one already eaten. See Object::looks.
	struct CmdBecome
	{
		std::string sprite;
		std::string target; // empty: the object running the command
		std::string from;   // from="normal": only one showing that look; empty: any
	};

	// <reveal object="igloo">count</reveal> - brings back the first `count`
	// (1 when left out) objects of that name or <group> that are out of play
	// (hidden at the start, or taken out by <die />), each where it started
	// and at its own starting velocity. Where <release> puts them in the
	// middle of the object running it (rocks breaking), this puts them where
	// they belong: an igloo built a block at a time, a door that appears.
	struct CmdReveal
	{
		std::string target;
		int count{ 1 };
	};

	// <reverse /> - the object's velocity turns round, both ways at once: a
	// bomber pacing the top of the screen changing its mind (from a <timer>),
	// or anything bouncing back off something it has no edge to bounce from.
	struct CmdReverse {};

	// <accelerate direction="up" burn="fuel">0.04</accelerate> - only ever in an
	// object's own <action>: while the key is held the object's velocity
	// changes by `amount` every frame in that direction (a thruster), where
	// <move> would set the velocity itself. burn names one of the object's own
	// <variable>s that one is taken off every frame the thrust is on, and the
	// thrust does nothing while it is at 0 or below (fuel); empty means free
	// (see playOneFrame).
	struct CmdAccelerate
	{
		Direction direction{};
		float amount{};
		std::string burn;
	};

	// <turn direction="left">3</turn> (or right) - only ever in an object's own
	// <action>: while the key is held the object's heading changes by `rate`
	// degrees every frame, counterclockwise for left and clockwise for right
	// (see Object::heading, playOneFrame). Held like a <move>.
	struct CmdTurn
	{
		Direction direction{};
		float rate{};
	};

	// <thrust burn="fuel">0.12</thrust> - only ever in an object's own <action>:
	// while the key is held the object's velocity changes by `amount` every
	// frame along the way it is facing (its heading), where <accelerate> pushes
	// along an axis. burn works as it does for <accelerate>.
	struct CmdThrust
	{
		float amount{};
		std::string burn;
	};

	// <release object="mediumrocks">2</release> - in a collision rule: puts
	// `count` (1 if it is left out) of the objects of that name that are out of
	// play - hidden at the start, or taken out with <die /> - back in play, in
	// the middle of the object that is running the rule, at their own starting
	// velocity. A group is how a pool of them is named: a rock that breaks
	// into smaller rocks releases them from the pool of smaller rocks.
	struct CmdRelease
	{
		std::string target;
		int count{ 1 };
	};

	// <stop /> - in a collision rule: the object comes to rest where it is and
	// stays there. Its velocity goes to 0, it is no longer pulled by its
	// <acceleration> or pushed by a held <accelerate>, until a reset gives
	// those back. A landing.
	struct CmdStop {};

	struct CmdPushState
	{
		std::string name;
	};

	// <pop /> - back to the state underneath. <pop state="wave2" /> - this
	// state goes and that one takes its place, so moving on (a wave, a room)
	// does not grow the stack the way a <push> would.
	struct CmdPopState
	{
		std::string name;
	};

	// <play sound="bounce" /> - starts one of the game's <sounds> (see sound.h
	// and audio.h). Anywhere a command can go: a collision rule, an object's
	// action (on the press), a key in a state, a condition. The game only asks
	// for it (Game::requestSound); Engine hands it to the Audio backend.
	struct CmdPlay
	{
		std::string sound;
	};

	// <silence sound="thrust" /> - stops one of the game's sounds if it is
	// playing: above all a sound that loops (<loop />), which otherwise plays
	// until something silences it. Anywhere <play> can go.
	struct CmdSilence
	{
		std::string sound;
	};

	// <fire object="projectileName" /> - spawn/launch a named projectile object
	struct CmdFire
	{
		std::string projectileName;
	};

	// <trigger object="objectName" action="actionName" /> - only ever appears in a
	// State's <input> list; triggers one of another object's own named <action> entries.
	struct CmdTriggerAction
	{
		std::string object;
		std::string action;
	};

	// <reset object="objectName" /> - only ever appears in a State's <input> or
	// <condition> (unlike the bare, untargeted CmdReset above, which is
	// also what a collision's own <reset /> produces); resets that named
	// object's position, velocity, and every <variable> back to their
	// starting values (see Game::resetObject).
	struct CmdResetObject
	{
		std::string target;
	};

	// <follow path="dive" /> - the object sets off along one of the game's
	// <paths> (see Path): from an object's own rule, action or timer, that
	// object; with object="name", every object of that name or <group>, each
	// `stagger` seconds after the one before, so a line of them flies in one
	// behind another. An object already on a path finishes it first. See
	// Game::follow and Game::applyPaths.
	struct CmdFollow
	{
		std::string path;
		std::string target; // empty: the object running the command
		float stagger{};
	};

	// <enter scene="room2" /> - that scene (its path, playing.room2) becomes
	// the current one of its state: its objects come into play and the last
	// one's stop where they are, kept as they were for when it is entered
	// again. What the state's own objects carry goes along. See Game::enter.
	// <enter way="east" /> (`way` set, `scene` empty) goes where the current
	// scene's way of that name leads, worked out when it runs.
	struct CmdEnterScene
	{
		std::string scene;
		std::string way;
	};

	// <reset scene="room1" /> - every object of that scene back as it started
	// (as <reset object> puts one back), and its timers started over: a room
	// full of robots again. See Game::resetScene. <reset way="out" /> resets
	// the scene that way of the current scene leads to.
	struct CmdResetScene
	{
		std::string scene;
		std::string way;
	};

	using Command = std::variant<
		CmdBounce, CmdStick, CmdReset, CmdDie, CmdWrap, CmdRide, CmdCarry, CmdDrop, CmdTrack, CmdDeflect, CmdReverse,
		CmdMove, CmdHop, CmdJump, CmdLand, CmdLeap, CmdClimb, CmdChase, CmdGo, CmdHunt, CmdWander, CmdAim, CmdAccelerate, CmdTurn, CmdThrust, CmdRelease, CmdStop, CmdIncrement, CmdDecrement, CmdCopy, CmdRank, CmdTurns, CmdPass, CmdOut, CmdDemo, CmdPushState, CmdPopState,
		CmdFire, CmdTriggerAction, CmdResetObject, CmdPlay, CmdSilence, CmdBecome, CmdReveal, CmdFollow, CmdEnterScene, CmdResetScene>;

	// --- What the XML says, before any of it is evaluated.

	struct RawOperation;

	// A value in the XML: an element's content wherever a number is wanted.
	// It is one of
	//   * a math expression written out as text (`window.width.center -
	//     title.width / 2`);
	//   * one tag that makes the number, such as <random min="-7" max="7"/>;
	//   * an <equation>: arithmetic as a list of steps, each one operation on
	//     two names or numbers, a step able to name its answer for the steps
	//     after it (Equation);
	//   * a <formula>: arithmetic as one operation whose operands are
	//     themselves numbers, operations included (Formula).
	// game_expr evaluates it (see game_expr::evaluate), so a new kind of value
	// tag is one more Kind here, one more case in game_xml's readValue and one
	// more in game_expr's evaluate.
	struct RawValue
	{
		enum class Kind { Expression, Random, Equation, Formula };

		Kind kind{ Kind::Expression };

		// Expression: the expression. Otherwise unused.
		std::string text;

		// Random: the two ends, each itself an expression.
		std::string min;
		std::string max;

		// Equation: its steps in the order written, the last being the answer.
		// Formula: the one operation it is. Shared, not copied, when a value
		// is.
		std::shared_ptr<const std::vector<RawOperation>> operations;

		static RawValue expression(std::string text)
		{
			RawValue value;
			value.text = std::move(text);
			return value;
		}

		// Every expression written inside this value, for the ones that only
		// need to look at what the expressions name (see
		// game_expr::sizeDependenciesOf). The operands of an equation or a
		// formula are names and numbers, and are listed the same way.
		std::vector<const std::string*> expressions() const;

		// Whether working it out draws a <random> (itself, or one inside an
		// equation or formula), so it comes out different each time.
		bool drawsRandom() const;
	};

	// One operand of an operation: the number it is, and the role it plays
	// (`dividend`, `subtrahend`, ...; see operationShape).
	struct RawOperand
	{
		std::string role;
		RawValue value;
	};

	// One arithmetic operation from an <equation> or a <formula>: `op` is the
	// tag (add, subtract, multiply, divide). The first operand is combined
	// with each of the others in turn, left to right, so that `a - b - c` is
	// one subtract with two subtrahends. `name` is only for an equation's
	// step, and what later steps call its answer.
	struct RawOperation
	{
		std::string op;
		std::string name;
		std::vector<RawOperand> operands;
	};

	// What an arithmetic tag is: its name, the role of its first operand and of
	// the others (a formula wants one of the first and one or more of the
	// second; an equation's step has one of each, as attributes), and the
	// symbol it is printed with.
	struct OperationShape
	{
		const char* tag;
		const char* first;
		const char* rest;
		char symbol;
	};

	// The shape of an arithmetic tag, or null when the tag is not one.
	const OperationShape* operationShape(const std::string& tag);

	// A value as one line of text, the way a game is printed: an expression as
	// written, an operation tree in infix with its brackets.
	std::string valueText(const RawValue& value);

	// A <sound> as written (game_xml.cpp reads it, and game_expr.cpp turns it
	// into a SoundDesc, sound.h).

	// One <note> or <rest> as written: <note pitch="C5" to="C6" wave="noise">0.1</note>.
	// wave, pitch and to are left as the words in the file; length is a value
	// (an expression or a value tag), in seconds.
	struct RawNote
	{
		bool rest{ false };
		std::string wave;   // "" means the sound's own
		std::string pitch;
		std::string to;     // "" means no slide
		RawValue length;
		unsigned part{ 0 }; // which <part> it is in, from 0 (0 for a sound of one part)
	};

	// One <sound name="..." wave="square"> as written; volume is empty text
	// when the sound gives no <volume>.
	struct RawSound
	{
		std::string name;
		std::string wave;   // "" means square
		bool loop{ false }; // <loop />
		RawValue volume;
		std::vector<RawNote> notes;
	};

	// One command tag from the XML - <bounce/>, <inc variable="a.b"/>,
	// <move direction="up">step</move>, ... - before its number is worked out.
	// `verb` is the tag name; which of the other fields are used depends on it
	// (see makeCommand).
	struct RawCommand
	{
		std::string verb;
		std::string object;    // reset, fire, release, trigger, chase, aim
		std::string variable;  // inc, dec, copy (the one read)
		std::string into;      // copy (the one written)
		std::string state;     // push
		std::string action;    // trigger
		std::string direction; // move, hop, accelerate, turn, climb
		std::string burn;      // accelerate, thrust
		std::string sound;     // play
		std::string sprite;    // become
		std::string path;      // follow
		std::string objClass;  // climb (what it climbs)
		std::string scene;     // enter, reset
		std::string way;       // enter, reset: a way of the current scene
		std::string from;      // become: only one showing this look
		std::string partner;   // hunt: <with object="...">
		std::string retreat;   // hunt: <retreat object="...">
		std::vector<std::string> players; // turns: each <player>'s state= (empty for none)
		RawValue amount;       // move, hop, accelerate, turn, thrust, deflect (the widest angle), release (how many), jump (distance), leap (height), climb, go, hunt and wander (the speed)
		RawValue seconds;      // jump (empty text: the default), follow (the stagger; empty text: 0)
		RawValue distance;     // chase (near; empty text: 0), hunt (how many tiles near is near enough to retreat)
		RawValue ahead;        // hunt (tiles in front of the one hunted; empty text: 0)
	};

	// A <timer> as written: <every> (again and again) or <after> (once), a
	// number of seconds, then the commands it runs each time it goes off.
	// See Timer.
	struct RawTimer
	{
		bool repeat{ true };
		RawValue interval;
		std::vector<RawCommand> commands;
		std::string name; // name="...": a scene from= this one's can put another in its place
		std::string sprite; // sprite="blue": an object's timer that counts only while it shows that look
	};

	// A <timer>, on an object or in a state, made ready to run. It counts
	// frames of the game (seconds times the window's <framerate>), so like
	// everything else it runs slow on a machine that cannot keep up. The
	// interval is kept as written and worked out again each time the timer
	// starts over, so <every><random min="1" max="3" /></every> waits a
	// different time each round. An object's timers count while it is shown
	// and in play; a state's while it is the current state. A reset starts
	// them over. See Game::updateTimers.
	struct Timer
	{
		bool repeat{ true };
		RawValue interval;
		std::vector<Command> commands;

		// sprite="blue": it counts only while its object shows that look, and
		// starts over whenever a <become> gives it that look (even the look it
		// shows already), so <after>6</after> is six seconds of being blue.
		// Empty: it counts whatever the object shows.
		std::string sprite;

		// Frames until it goes off; -1 means not started yet (worked out on
		// the first frame it counts). `done` is a once-only timer that has
		// gone off.
		int framesLeft{ -1 };
		bool done{ false };
	};

	// One leg of a path as written: a step of x and y (and the commands run as
	// it sets off), or home. See Path.
	struct RawPathStep
	{
		bool home{ false };
		RawValue x;
		RawValue y;
		std::vector<RawCommand> commands;
	};

	// A <path> as written, in the game's <paths>. See Path.
	struct RawPath
	{
		std::string name;
		RawValue speed;
		bool hasStart{ false };
		RawValue startX;
		RawValue startY;
		std::vector<RawPathStep> steps;
	};

	struct PathStep
	{
		bool home{ false };
		Vector2f by;
		std::vector<Command> commands;
	};

	// A path: a way through the window that objects <follow>, written once
	// and flown by any number of them (Galaxian's aliens flying in and
	// diving). It is a list of legs flown one after the other at `speed`
	// pixels a frame: a step moves the object by `by` from wherever the step
	// began, and runs its commands (an object's own, as on a timer: firing,
	// a sound) as it sets off; home takes it back to where it started the
	// game (its <position>), however far it has come. With a start, the
	// object is first put there; without, it sets off from where it is. The
	// numbers are worked out once, when the game loads. A follower is moved
	// by its velocity like anything else, so it collides, and a <wrap /> on
	// the way carries it round without changing what is left of the step.
	struct Path
	{
		std::string name;
		float speed{};
		bool hasStart{ false };
		Vector2f start;
		std::vector<PathStep> steps;
	};

	// Works out a RawValue to a number; makeCommand is given one so that it can
	// stay free of the expression library (and be tested without it).
	using ValueEvaluator = std::function<float(const RawValue&)>;

	// Turns what the XML says into the typed Commands the engine runs. This is
	// the one place that knows what each command tag means; an unknown tag or
	// direction throws std::runtime_error naming it, so a typo in a game file
	// is reported when it loads.
	Command makeCommand(const RawCommand& raw, const ValueEvaluator& evaluate);
	std::vector<Command> makeCommands(const std::vector<RawCommand>& raw, const ValueEvaluator& evaluate);

	// Maps a spriteParams tag ("circle", "rectangle", "text", "image", "line") to a
	// ShapeKind. Returns ShapeKind::Unknown for anything else (including an
	// empty tag).
	ShapeKind shapeKindFromTag(const std::string& tag) noexcept;

	// The object's own footprint, straight from its spriteParams - no Window
	// backend required. Circle/rectangle sizes are fully implied by the
	// numeric params already sitting in spriteParams (radius, or width and
	// height), so this is exact for Rectangle and a very close approximation
	// for Circle (matches a rendered circle's bounding box to within
	// sub-pixel rounding). Text/image footprints genuinely depend on a real
	// font/image load - there's no way to know them without a backend - so
	// those (and ShapeKind::Unknown) come back {0,0}. Used by
	// game_expr::init() to lay out a group's cells entirely within Game's
	// own construction, before any Window exists (see window.h's Window::
	// init(), which now only measures the *real* rendered Object::size for
	// drawing/collision, not position).
	Vector2f measureShapeSize(const std::vector<std::string>& spriteParams, ShapeKind shapeKind) noexcept;

	// A <circle> or a <rectangle> sprite as the window backends draw it and
	// measureShapeSize measures it: the tag, the size, and the color's name.
	// The engine makes one from the file; a generated game calls these.
	std::vector<std::string> circleSprite(float radius, const std::string& color);
	std::vector<std::string> rectangleSprite(float width, float height, const std::string& color);

	// If `expression` is nothing but an "owner.variable" reference (for
	// example paddle1.score, or playing.room1.tally.kills, whose owner is a
	// path), returns {owner, variable}, split at the last dot. Returns nullopt
	// for anything else - a number, a sum, a name with no owner. Used to tell a <number> in a text sprite that should stay live
	// (bound to that variable) from one that is just worked out once; it is a
	// plain scan of the text rather than an exprtk lookup, since once
	// exprtk has evaluated the expression only the resolved value is left.
	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression);

	// Formats a live numeric value for on-screen display: whole numbers print
	// without a decimal point (scores, HP, ammo, ...), matching what someone
	// hand-typing a <text> content of 0 would have written.
	std::string formatDisplayNumber(float value);

	std::ostream& operator<<(std::ostream& o, const Command& command);

	// How a value and a command tag read in printGame()'s output.
	std::ostream& operator<<(std::ostream& o, const RawValue& value);
	std::ostream& operator<<(std::ostream& o, const RawCommand& command);
	std::ostream& operator<<(std::ostream& o, const std::vector<RawCommand>& commands);
}
