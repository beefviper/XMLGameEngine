// command.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "engine/keys.h"
#include "game/name.h"
#include "game/types.h"
#include "picture/bitmap.h"

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
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

	// What kind of picture an Object shows.
	enum class ShapeKind { Unknown, Circle, Rectangle, Text, Image, Line };

	// What an object (or one of its looks) shows, as the window draws it,
	// worked out once at load (load/expr's buildPicture). Every picture
	// the engine draws itself (<line>s, a <bitmap>, an <svg>, a tether) is
	// a Line: its pixels are the object's bitmap, and `size` is the
	// bitmap's. The engine draws the rest too, when a window asks for the
	// object's picture (Window::pictureOf): a text, a circle, a
	// rectangle, an image file.
	struct Picture
	{
		ShapeKind kind{ ShapeKind::Unknown };
		Vector2f size{};             // a rectangle's, or a Line's drawing's
		float radius{};              // a circle's
		Name color;                  // a circle's, rectangle's or text's color. name
		std::string text;            // a text's words as shown now
		int textSize{};              // a text's size in pixels
		Name font;                   // a text's font file; empty: the standard one (textFontFile)
		Name file;                   // an image's file, as written
		PictureId image;             // an image's pixels, flipped as asked (the game's Resources)
		bool flipHorizontal{ false }; // an image's, drawn mirrored
		bool flipVertical{ false };

		friend bool operator==(const Picture&, const Picture&) = default;
	};

	// A Picture held by pointer: an object's is its look's, one between
	// them, until something changes it (a text's words), when edit copies
	// a shared one first.
	struct HeldPicture
	{
		HeldPicture() = default;
		HeldPicture(Picture made) : held(std::make_shared<Picture>(std::move(made))) {}

		const Picture& shows() const noexcept
		{
			static const Picture none{};
			return held ? *held : none;
		}
		operator const Picture&() const noexcept { return shows(); }
		const Picture* operator->() const noexcept { return &shows(); }
		friend bool operator==(const HeldPicture& a, const HeldPicture& b) { return a.shows() == b.shows(); }
		Picture& edit()
		{
			if (!held) { held = std::make_shared<Picture>(); }
			else if (held.use_count() > 1) { held = std::make_shared<Picture>(*held); }
			return *held;
		}

	private:
		std::shared_ptr<Picture> held;
	};

	// --- Command: what a command tag in the XML becomes once it is loaded.
	// makeCommand() (game/command.cpp) is the one place that turns a RawCommand
	// into one of these.

	// <bounce />, <bounce>0.7</bounce>: off what it touched, mirrored about
	// the way the touched surface faces (a screen edge, a box's side, the
	// round of a circle, the slope of a drawn line), keeping that share of
	// the speed it came in with (1 when left out; above 1 it comes off
	// faster, a bumper). See xge::bounce.
	struct CmdBounce { float share{ 1.0f }; };
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
	// <drop />s it (see Object::Carry). One thing at a time: picking up
	// another puts the first down, and taking one another object carries
	// takes it from that one.
	struct CmdCarry {};

	// <drop /> - the object puts down what it carries, where it is.
	struct CmdDrop {};

	// <track />, in an action an <input axis="..."> triggers: the object
	// goes where the control points, across the window (an Atari paddle).
	// With path="...", its middle goes that far along the path instead, from
	// the path's <start> (0) to the end of its last leg (1): a paddle on an
	// arc, a gun on a rail.
	struct CmdTrack
	{
		std::string path;
		// On cells, with <onto class="..."/>s: the cell nearest where it
		// points that holds a piece of one of those classes.
		std::vector<std::string> onto;
	};

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

	// <knock /> - in a collision rule about another object: the two meet
	// as things with weight do, along the line through the point they touch
	// (see Touch::normal): the other is sent off with what the object was
	// carrying into it, shared out by their <mass>es, and the object keeps
	// what is left. Only what has a mass is moved; one without is as
	// heavy as can be (a wall, a paddle, a man's head). A knock on a thing
	// with no mass goes through it: whatever with a mass rests against its
	// far side is knocked instead, and runs its own rules about the object
	// as touched on that side (Mario bumping a floor from below flips what
	// walks on it). `bounce` (0 to 1, 1 when left out) is how much of the
	// coming together turns into going apart: 1 is pool balls, 0 leaves the
	// knocked thing going no faster than what knocked it. Nothing happens
	// unless they are coming together. See CommandExecutor::knock.
	struct CmdKnock
	{
		float bounce{ 1.0f };
	};

	// <hole>6<rough>0.5</rough></hole> - in a collision
	// rule about another object: a round hole is cut in the object's own
	// picture (a sprite of <line>s or a <bitmap>), centered where what it
	// touched struck it: a shot through a bunker, a bomb crater. `rough` (0 to
	// 1, 0 when left out) is how much of the radius each pixel at the edge
	// may keep at random, so a rough hole has a ragged edge. What is cut away
	// is not drawn and, for a <type>pixel</type> collision, not touched; a
	// reset puts the picture back whole. See xge::cutHole.
	struct CmdHole
	{
		float radius{};
		float rough{};
	};

	struct CmdMove
	{
		Direction direction{};
		float step{};
		// On a cell map: held, it steps one cell at once, waits this many
		// seconds, then steps `step` cells a second while held (0: no wait).
		float wait{};
	};

	// <settle /> - on a cell map: a shape's cells are left where they are as
	// loose pieces of the map and the shape is gone; then the map's
	// <checks> are asked. See Game::settle.
	struct CmdSettle {};

	// <slide direction="down" /> - on a cell map: a cell at a time that way
	// until the next is refused, all at once; the refusal is a touch.
	struct CmdSlide
	{
		Direction direction{};
		// class=: every piece of that class on cells, the one nearest that
		// way first; refused when none moved.
		std::string objClass{};
	};

	// <clear /> - in a cell map's check: the pieces it found are taken out
	// of the game.
	struct CmdClear {};

	// <collapse /> - in a cell map's check of full rows: every loose piece
	// above the row comes down one row.
	struct CmdCollapse {};

	// <fall /> - in a cell map's check: every loose piece of the map drops
	// down its column onto what is under it.
	struct CmdFall { std::string objClass; }; // only pieces of this class fall; empty: all

	// <cycle /> - a shape's cells each move on to the next cell of its
	// outline, in reading order, the last to the first.
	struct CmdCycle { bool backward{ false }; };

	// A variable a command names, owner.variable, taken apart once when the
	// command is made: the owner (an object, a path to one, or for <set> a
	// group) and the variable after the last dot. For <set>, `variable`
	// may be one of the numbers every object has (velocity.x, heading).
	struct VariableName
	{
		Name owner;
		Name variable;

		// The name as written (player.score) taken apart at its last dot.
		static VariableName of(const std::string& written);

		// As written.
		std::string written() const { return owner + "." + variable; }
	};

	// <inc variable="ownerName.variableName" /> - add 1 (or, for <inc variable="...">5</inc>,
	// the amount) to that object's named <variable> and refresh every text object whose display is bound to it (see
	// Game::addTo and Object::BoundText).
	struct CmdIncrement
	{
		VariableName target;
		float amount{ 1.0f };
	};

	// <copy variable="player.score" into="best.score" /> - puts the number one
	// variable holds now into another, and refreshes every text bound to that
	// one (see Game::copyVariable). Both are owner.variable. A high score:
	// a condition copies the score into the best whenever it is above it.
	struct CmdCopy
	{
		VariableName from;
		VariableName into;
	};

	// <rank variable="entry.score" into="table" /> - the entry goes into a
	// table of the best, a <group> whose members are its places, the best
	// first: it takes the place of the first member whose variable of that
	// name is below its own, with every variable it has that the member has,
	// and the members from there on move down a place, the last one falling
	// off. Below them all, nothing happens. See Game::rank.
	struct CmdRank
	{
		VariableName entry;    // the entry and what orders it
		std::string table;     // the group
	};

	// <alternate><player state="player1" /><player state="player2" /></alternate> -
	// the game from here is played by that many players taking turns, each
	// with a game of their own, all starting as the game is now. The first
	// player plays first; each player's state= (optional) is pushed whenever
	// their turn comes up, a screen saying whose it is. See Game::startTurns.
	struct CmdAlternate
	{
		std::vector<std::string> players;
	};

	// <demo /> - the game's recorded <demo> plays, the attract mode: a new
	// game, played by the keys recorded, until a key is pressed or the
	// recording ends, and then a new game again. See Engine.
	struct CmdDemo {};

	// <pass /> - the turn passes to the next player still in: this player's
	// game is put away as it is, theirs comes back. See Game::passTurn.
	// By a member of a group that takes turns, the next member's turn; with
	// member=, the turn of the group's member of that number (0: nobody's),
	// the group named by object= or else the doer's. See Game::passPlayers.
	struct CmdPass
	{
		std::string group;
		int member{ -1 };
	};

	// <out /> - this player is out of the game, and the turn passes; when
	// another is still in, nothing after it in the list runs, and when none
	// is, what follows does (<out /><reset />). See Game::passTurn.
	struct CmdOut {};

	// <dec variable="ownerName.variableName" /> - the opposite of <inc />: takes 1 off that
	// object's named <variable> and
	// refreshes every text bound to it.
	struct CmdDecrement
	{
		VariableName target;
		float amount{ 1.0f };
	};

	// <hop direction="up">distance</hop> (also down, left, right) - only ever in an
	// object's own <action>: a one-shot
	// jump of `distance` pixels, made once for each press of the key (see
	// CommandExecutor::triggerObjectAction and playOneFrame). Unlike
	// <move>, holding the key does nothing more, and a state change never
	// resumes it. A diagonal one (up-left, up-right, down-left, down-right)
	// goes `distance` along both, or <x> across and <y> up or down: a hop
	// from cube to cube of Q*bert's pyramid. `direction` is the way it then
	// faces (the up or down of a diagonal), `step` the whole of the hop.
	// With <seconds> after the distance it takes that
	// long, through the air: touching nothing until it
	// lands, and a second hop waits for the landing. 0: at once. Its <arc>
	// is how high it is drawn above its way at the halfway point (a parabola
	// through its altitude, up then down): a timed hop that leaves the ground.
	struct CmdHop
	{
		Direction direction{};
		float distance{};
		Vector2f step{};
		float seconds{};
		float arc{};
		// On cells, with <onto class="..."/>s: to the nearest cell that way
		// holding a piece of one of those classes (Game::hopOnto).
		std::vector<std::string> onto;
	};

	// <hunt object="qbert"><hop><x>32</x><y>48</y><seconds>0.3</seconds></hop></hunt>
	// - a hunt by hops, for an object that walks no maze: of the four hops
	// it could make (the diagonals with an <x> and a <y>, else up, down,
	// left and right by its distance), the one that lands nearest the
	// middle of the nearest in play of that name or <group>, among those that
	// stay in the window; a tie goes up before down, then left before right.
	// One hop each time it runs (a timer), so it hunts at its own pace:
	// Coily. A hop under way, or nothing to hunt, and it stays.
	struct CmdHuntHop
	{
		std::string target;
		float across{};
		float down{};
		bool diagonal{ false };
		float seconds{};
		float arc{};
	};

	// <land /> - in a collision rule about another object: coming down onto
	// its top, the object stands on it. It is put on the top, its fall stops,
	// and for the next frame it is on the ground, which a <jump> needs. Touching
	// any other side, or going up, does nothing, so a platform is solid from
	// above only. See landOnto in verbs/land.cpp.
	struct CmdLand {};

	// <jump>height</jump> - only ever in an object's own <action>: a jump up
	// under the object's own pull (its <acceleration>), rising `height`
	// pixels before it falls again. Only from the ground (after a <land />),
	// and the way across is fixed at the take-off: a held key does not steer
	// it until it lands. Donkey Kong's jump. See CommandExecutor::jump.
	struct CmdJump
	{
		float height{};
	};

	// <climb direction="up" class="ladder">step</climb> (or down) - only ever
	// in an object's own <action>, held like a <move>: while the object stands
	// at, or is on, an object of that class, it goes up or down it `step`
	// pixels a second, lined up with its middle, with no pull and no <land />
	// on the way, and no walking off it until it reaches either end. See
	// playOneFrame.
	struct CmdClimb
	{
		Direction direction{};
		float step{};
		std::string ladderClass;
	};

	// <chase object="player">90<near>100</near></chase> - the
	// object heads straight for the middle of the nearest one in play of that
	// name or <group>, at `speed` pixels a second; within `near` pixels of it
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
	// way at the first gap there is and keeps going, `speed` pixels a second,
	// until a wall stops it; turned straight round, it goes back at once.
	// Pac-Man's stick. See play/maze.h.
	struct CmdGo
	{
		Direction direction{};
		float speed{};
	};

	// <hunt object="pacman">2<ahead>4</ahead><with object="blinky"
	// /><retreat object="corner">8</retreat></hunt> - for an object that walks
	// a <maze>: from now on, at every tile, it takes the open way (never back
	// the way it came) whose next tile is nearest its target, as the arcade's
	// ghosts do. The target is the middle of the nearest one in play of that
	// name or <group>; `ahead` tiles in front of the way that one faces;
	// with a partner, as far past that point again as the partner is short of
	// it (Inky's pincer); and while the hunter is within `retreatWithin`
	// tiles of what it hunts, the middle of `retreatTo` instead (Clyde backing
	// off). Kept until another <hunt>, a <go>, a <wander> or a reset. See
	// play/maze.h.
	struct CmdHunt
	{
		std::string target;
		float speed{};
		float ahead{};
		std::string partner;
		std::string retreatTo;
		float retreatWithin{};
		// <search />: the first step of the shortest way to the target's tile
		// (play/maze.h's mazeDistances), not the next tile nearest it.
		bool search{ false };
	};

	// <wander>1</wander> - for an object that walks a <maze>:
	// at every tile it takes an open way at random, never back the way it
	// came: a frightened ghost. Kept as a <hunt> is.
	struct CmdWander
	{
		float speed{};
	};

	// <aim object="player" /> - the object turns to the nearest one in play
	// of that name or <group>: its next <fire>s go straight at where that was,
	// at any angle, and its <facing> (or <heading>) turns that way. Kept until
	// it aims again, is reset, or a key moves it. With <lined>8</lined>, only
	// along one of the eight ways (across, up or down, or a true diagonal),
	// when that one is within 8 pixels of one; otherwise it is refused, and
	// what follows it in the timer does not run. See aimAtNearest in verbs/aim.cpp.
	struct CmdAim
	{
		std::string target;
		bool lined{ false };
		float within{};
	};

	// <become sprite="blue" /> - the object shows another of its named
	// <sprite>s from now on (its look; a reset goes back to the first), or,
	// with object="name", every object of that name or <group> does: a row of
	// ice turning blue when it is stood on, a light going green. A collision
	// rule with sprite="..." runs only while its object shows that sprite, so
	// a look is also a simple state. With from="normal", only an object
	// showing that look changes: the ghosts turn blue, but not the eyes of
	// one already eaten. See Object::looks. With style="angry" (beside or
	// instead of sprite=) its bitmaps that take a style take that one instead:
	// the same pictures in other colors, a palette swap (Object::Looks::palettes).
	struct CmdBecome
	{
		std::string sprite; // empty: the look stays
		std::string target; // empty: the object running the command
		std::string from;   // from="normal": only one showing that look; empty: any
		std::string style;  // empty: the colors stay
	};

	// <reveal object="igloo">count</reveal> - brings back the first `count`
	// (1 when left out) objects of that name or <group> that are out of play
	// (inactive at the start, or taken out by <die />), each where it started
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

	// <thrust direction="up" burn="fuel">144</thrust> - only ever in an
	// object's own <action>: while the key is held the object's velocity
	// gains `amount` pixels a second each second in that direction (a thruster), where
	// <move> would set the velocity itself. burn names one of the object's own
	// <variable>s that 60 a second is taken off while the thrust is on, and the
	// thrust does nothing while it is at 0 or below (fuel); empty means free
	// (see playOneFrame).
	struct CmdThrustAcross
	{
		Direction direction{};
		float amount{};
		std::string burn;
	};

	// <turn direction="counterclockwise">180</turn> (or clockwise) - only ever in an object's own
	// <action>: while the key is held the object's heading changes by `rate`
	// degrees a second, counterclockwise for left and clockwise for right
	// (see Object::heading, playOneFrame). Held like a <move>.
	struct CmdTurn
	{
		Direction direction{};
		float rate{};
	};

	// <thrust burn="fuel">0.12</thrust> - only ever in an object's own <action>:
	// while the key is held the object's velocity changes by `amount` every
	// frame along the way it is facing (its heading), where <thrust direction> pushes
	// along an axis. burn works as it does there.
	struct CmdThrust
	{
		float amount{};
		std::string burn;
	};

	// <swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing>
	// - only ever in an object's own <action>: while the key is held the
	// heading turns that many degrees that way (as <turn>: counterclockwise takes from
	// the heading, right adds) in a stroke `seconds` long, starting from
	// still, fastest half way, slowing to a stop; let go, it comes back the
	// same way to the heading it started with (a flipper). `by` is the turn
	// with its sign (counterclockwise below 0). See playOneFrame.
	struct CmdSwing
	{
		float by{};
		float seconds{};
	};

	// <release object="mediumrocks">2</release> - in a collision rule: puts
	// <stop /> - in a collision rule: the object comes to rest where it is and
	// stays there. Its velocity goes to 0, it is no longer pulled by its
	// <acceleration> or pushed by a held <thrust>, until a reset gives
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

	// <play sound="bounce" /> - starts one of the game's <sounds> (see audio/sound.h
	// and audio/audio.h). Anywhere a command can go: a collision rule, an object's
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

	// Cells about an object on a map of cells, for a <count> or a
	// <trigger>: <here /> its own cell (on any map of cells laid in the
	// same place), <around /> the eight cells round it (<straight />: only
	// the four beside it), there too. <alike variable="v" /> in <around>:
	// only pieces whose v is the object's own.
	struct CellPlace
	{
		enum class Where { None, Here, Around };
		Where where{ Where::None };
		bool straight{ false };
		std::string alike;
		bool any() const noexcept { return where != Where::None; }
	};

	// <trigger object="objectName" action="actionName" /> - only ever appears in a
	// State's <input> list; triggers one of another object's own named <action> entries.
	// from="normal": only those of them showing that look run it.
	struct CmdTriggerAction
	{
		std::string object;
		std::string action;
		std::string from{};
		// In a key's commands: how many after it are its <otherwise>, run
		// only when its action is refused (Engine::runAsPress).
		int otherwise{ 0 };
		// <here /> or <around /> with class=: each piece of that class in
		// those cells about the object running it, each one once a press
		// however the actions call one another (Minesweeper's flood).
		std::string objClass{};
		CellPlace place{};
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
		// With target: only this many of them, the nearest the object
		// running it that are not on a path already (0: all).
		int nearest{ 0 };
	};

	// <shift object="aliens"><x>2</x></shift> - moves every object of that
	// name or group by x across and y down, and the place each goes home to
	// (a path's <home />) with it. One on a path keeps on its path, and comes
	// home to the shifted place: Galaxian's formation sways. A reset puts
	// the place back. See Game::shift.
	struct CmdShift
	{
		std::string target;
		Vector2f by;
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

	// <set variable="ball.speed">value</set> - puts the value, worked out as
	// it runs, into a variable (owner.variable, and every text showing it is
	// refreshed), or into one of the numbers every object has: owner.x,
	// owner.y (its top left), owner.velocity.x, owner.velocity.y and
	// owner.heading. See Game::setNumber.
	struct CmdSet
	{
		VariableName target;
		float value{};
	};

	// <spawn object="shots"> with a <count>, a <position>, a <velocity> and
	// an <acceleration>, each optional - `count` new copies of the piece
	// `target` (a group: its members in turn, or at random), each put where the position says (its
	// top left, or, for a half that is <centered>, its middle; a half left
	// out, the spawner's middle) and going as the velocity says. Past its
	// <most>: no more. The values are worked out as it runs, so they can say
	// where the object spawning it is (self.x) and how it is going. See
	// CommandExecutor::spawn.
	struct CmdSpawn
	{
		std::string target;
		// <count>N</count>: how many copies (1 when left out).
		int count{ 1 };
		std::optional<float> x;
		std::optional<float> y;
		bool centerX{ false };
		bool centerY{ false };
		std::optional<float> velocityX;
		std::optional<float> velocityY;
		std::optional<float> accelerationX;
		std::optional<float> accelerationY;
		// <random />: one of a group's members at random, not in turn.
		bool random{ false };
		// <member>N</member>: the group's member of that number, from 1.
		std::optional<float> member;
		// On a cell map: the column and row its top left cell goes in,
		// counting from 1 (left out: 1).
		std::optional<float> column;
		std::optional<float> row;
		// into=: the map of cells it goes on, when not its own.
		std::string into;
		// <free />: in a free cell of the map, at random; with <below>N</below>,
		// one under row N.
		bool free{ false };
		int freeBelow{ 0 };
	};

	// <grow /> - an object with a <chain>: one link longer, at the tail,
	// on its next step.
	struct CmdGrow {};

	// <split /> - a piece of a <chain>: the chain is cut there. The piece
	// leaves it; the links behind it are a chain of their own, led by the
	// first of them, which takes the head's going, look and variables.
	struct CmdSplit {};

	// <through />: in a rule for a refused cell step, the step is made
	// after all, into the cell with what is there (Centipede's turn down
	// onto a mushroom).
	struct CmdThrough {};

	// <shove />: in a rule for a refused cell step, what is in the way is
	// moved one cell on, the way it was met (a crate pushed); the step goes
	// on if that left the cell free. The shoved piece's own step touches
	// what is beyond it, so a crate against a wall or another crate stays.
	struct CmdShove {};

	// <merge variable="value" />: in a rule for a refused cell step, when
	// the piece in the way has the same number in that variable, it is
	// taken away and its number added to the mover's, and the step goes on.
	// A piece merged once does not merge again in the same <slide>.
	struct CmdMerge
	{
		std::string variable;
	};

	// <undo />: the pieces on maps of cells, and every object's variables,
	// back as they were before the last press that moved a piece.
	struct CmdUndo {};

	// A <condition> in an action: its commands run, in order, while its
	// test about the object running the action holds (Minesweeper's flood
	// goes on only from a square with no mines round it). The test is made
	// once every object is known (WhenBody, in game/condition.h).
	struct WhenBody;
	struct CmdWhen
	{
		std::shared_ptr<WhenBody> body;
	};


	// <pick class="red" />: a picker over a board game's map of cells takes
	// up the piece of that class in its cell (column and row), one with a
	// move it can make; refused when there is none, or while the piece it
	// holds must jump on. See Game::pickPiece.
	// <only />: the one piece of that class that has a move, wherever the
	// picker is, when only one has (a jump that must be made).
	struct CmdPick
	{
		std::string objClass;
		bool only{ false };
	};

	// <place />: the piece the picker holds makes the one of its moves
	// that ends in the picker's cell. Refused when none does, and while
	// the piece can jump on (the turn is not over). See Game::placePiece.
	// By a picker of what a <holds> holds (a hand of cards), <pick /> takes
	// what it points at and <place /> lays what it holds where it points,
	// or with into= on the first of those holders that accepts it.
	struct CmdPlace
	{
		std::string into;
	};

	// <call object="discard" />: a hand names the suit of the card it points
	// at in its own holder for that holder, waiting with a <wild> card on top.
	// Refused when the holder is not waiting or no card of its own is pointed at.
	struct CmdCall
	{
		std::string holder;
	};

	// <put class="black" />: a picker puts a new piece of that class (a
	// copy of the first written) in its cell of the pieces' map. Refused
	// when the cell is taken, and, when the pieces' moves have <flank>s,
	// where it traps no line; the pieces it traps turn into its kind. See
	// Game::putPiece.
	// A piece put whose <moves> have a <close> may wall in a piece beside
	// it and claim it; then the put counts as refused, so the turn does
	// not pass.
	struct CmdPut
	{
		std::string objClass;
	};

	// <count class="black" into="score.black" />: how many objects of that
	// class are in play now (pieces put this frame too), into a variable.
	struct CmdCount
	{
		std::string objClass;
		VariableName target;
		// <here /> or <around />: only those in those cells about the
		// object running it.
		CellPlace place;
	};

	// <send object from into>: the object's piece on map `from` goes to map
	// `into`, in that column and row (left out: the cell it is in).
	// With no object=, members of a held class: <send from into>N</send>
	// the top N of holder `from` (or <all />; from a group: every member,
	// wherever it is) into holder `into`, showing `sprite` if given
	// (Game::sendHeld).
	struct CmdSend
	{
		std::string target;
		std::string from;
		std::string into;
		std::optional<float> column;
		std::optional<float> row;
		bool held{ false };
		int count{};
		bool all{ false };
		std::string sprite;
	};

	// <order group><random /></order>: what that holder holds, in a random
	// order (Game::orderHeld).
	struct CmdOrder
	{
		std::string group;
	};

	// <despawn /> - the object takes itself out of the game; with object=,
	// every object of that name or group (Game::despawn). Not the same as
	// <die />, which leaves it in the game, out of play, for a <reveal> or a
	// <fire> to bring back: a despawned object is not there at all.
	struct CmdDespawn
	{
		std::string target;
	};

	// <hold button="up" /> - only in a <condition> of a state's <computer>:
	// while that condition holds the computer holds the button down, as a
	// player would, and lets it go when it no longer does. See
	// Game::computerButtons.
	struct CmdHold
	{
		KeyCode button{};
	};

	// <think class="white" object="cursor" button="space"> - only in a
	// <condition> of a state's <computer>: while it holds, the computer
	// plays the board game pieces of that class. It looks `depth` moves
	// ahead (each side's in turn), weighing pieces by `worths` (by look;
	// 1 for any other), picks the best, then walks the picker to the
	// piece and to each cell of its move, a cell at a time, pressing the
	// button on each as a player would. See Game::think (game/think.cpp).
	struct CmdThink
	{
		std::string objClass;
		std::string picker;
		KeyCode button{};
		int depth{ 4 };
		std::vector<std::pair<std::string, float>> worths;
		bool avoid{ false };   // <avoid />: a card player that keeps out of tricks
	};

	struct RawCommand;
	struct RawCondition;
	struct LoadedCommand;

	// A command whose numbers name something that changes while the game
	// runs (a variable of an object, where an object is or how it is going,
	// the touch a rule is answering): made again from what was written each
	// time it runs, so a variable can always be changed. `loaded` is the
	// command as it was made at load, which says what kind of command it is
	// (see shapeOf). See Game::makeNow. One that says self (`namesSelf`)
	// is made for the object running it, as it runs; one that says other
	// (`namesOther`), for the one it touched.
	struct CmdLive
	{
		std::shared_ptr<const RawCommand> raw;
		std::shared_ptr<const LoadedCommand> loaded;
		std::string where;
		bool namesSelf{ false };
		bool namesOther{ false };   // the same for other, in a rule about touching
	};

	using Command = std::variant<
		CmdBounce, CmdStick, CmdReset, CmdDie, CmdWrap, CmdRide, CmdCarry, CmdDrop, CmdTrack, CmdDeflect, CmdKnock, CmdHole, CmdReverse,
		CmdMove, CmdHop, CmdHuntHop, CmdLand, CmdJump, CmdClimb, CmdChase, CmdGo, CmdHunt, CmdWander, CmdAim, CmdThrustAcross, CmdTurn, CmdThrust, CmdSwing, CmdStop, CmdIncrement, CmdDecrement, CmdCopy, CmdRank, CmdAlternate, CmdPass, CmdOut, CmdDemo, CmdPushState, CmdPopState,
		CmdFire, CmdTriggerAction, CmdResetObject, CmdPlay, CmdSilence, CmdBecome, CmdReveal, CmdFollow, CmdShift, CmdEnterScene, CmdResetScene, CmdSet, CmdSpawn, CmdSend, CmdDespawn, CmdHold, CmdSettle, CmdSlide, CmdClear, CmdCollapse, CmdFall, CmdCycle, CmdGrow, CmdSplit, CmdThrough, CmdShove, CmdMerge, CmdUndo, CmdWhen, CmdPick, CmdPlace, CmdPut, CmdOrder, CmdCount, CmdThink, CmdCall, CmdLive>;

	struct LoadedCommand
	{
		Command command;
	};

	// What kind of command it is: itself, or for one made as it runs, the
	// command it was at load.
	const Command& shapeOf(const Command& command) noexcept;

	// --- What the XML says, before any of it is evaluated.

	struct RawOperation;

	// A value in the XML: an element's content wherever a number is wanted.
	// It is one of
	//   * a math expression written out as text (`window.width.center -
	//     title.width / 2`);
	//   * one tag that makes the number, such as
	//     <random><min>-7</min><max>7</max></random>;
	//   * an operation (<subtract>, <clamp>, ...): arithmetic whose operands are
	//     themselves numbers, operations included (Formula);
	//   * in a <position> only, <centered>: the object's middle at the
	//     expression it holds, or that <offset> from the window's middle
	//     (the window's middle when empty), on that axis (Centered).
	//     <left>, <right>, <top> and <bottom> are Centered too: that side of
	//     the object on the window's, an <offset> in from it (Edge).
	//     centerOnOwnSize makes it an Expression once the object's name is
	//     final, so nothing evaluates it as it is.
	// load/expr evaluates it (see load/expr::evaluate), so a new kind of value
	// tag is one more Kind here, one more case in load/xml's readValue and one
	// more in load/expr's evaluate.
	struct RawValue
	{
		enum class Kind { Expression, Random, Formula, Centered };

		Kind kind{ Kind::Expression };

		// Expression: the expression. Centered: where the middle goes, or
		// empty. Otherwise unused.
		std::string text;

		// Centered with an <offset>: how far from the window's middle the
		// object's middle goes (right or down; negative, left or up), an
		// expression. `text` is then empty. Otherwise empty.
		std::string offset;

		// Centered: which of the object's own sides goes to the window's: its
		// middle to the middle (<centered>), its left or top to the window's
		// (<left>, <top>: Start), its right or bottom to the window's (<right>,
		// <bottom>: End), then `offset` in from that side. An edge holds no text.
		enum class Edge { Middle, Start, End };
		Edge edge{ Edge::Middle };

		// Random: the two ends, each itself an expression.
		std::string min;
		std::string max;

		// Formula: the one operation it is. Shared, not copied, when a value
		// is. Random with an end that is an operation: one operation "random"
		// of two operands, `min` and `max` (each an expression or an operation),
		// and min and max above left empty.
		std::shared_ptr<const std::vector<RawOperation>> operations;

		// update="live": worked out again every frame from what it reads, not
		// once as the game loads. Only an object's <variable>, the halves of
		// its <position> and its <heading> take it (Object::follows).
		bool live{ false };

		static RawValue expression(std::string text)
		{
			RawValue value;
			value.text = std::move(text);
			return value;
		}

		// Every expression written inside this value, for the ones that only
		// need to look at what the expressions name (see
		// load/expr::sizeDependenciesOf). The operands of an operation are names
		// and numbers, and are listed the same way.
		std::vector<const std::string*> expressions() const;

		// Whether working it out draws a <random> (itself, or one inside a
		// operation), so it comes out different each time.
		bool drawsRandom() const;
	};

	// An <x>/<y> pair, each still a RawValue.
	struct RawVector2
	{
		RawValue x;
		RawValue y;
	};

	// One operand of an operation: the number it is, and the role it plays
	// (`value`, or a test word such as `atleast`; see operationShape).
	struct RawOperand
	{
		std::string role;
		RawValue value;
	};

	// One arithmetic operation: `op` is the tag (add,
	// subtract, multiply, divide, smallest, largest, absolute, clamp). The
	// first operand of the four arithmetic ones is combined with each of the
	// others in turn, left to right, so that `a - b - c` is one subtract with
	// three values.
	struct RawOperation
	{
		std::string op;
		std::vector<RawOperand> operands;
	};

	// What an arithmetic tag is: its name, the role of its first operand and of
	// the others, how many operands it takes (`most` 0 for no limit), the role
	// of its last when that differs (clamp's <atmost>), and how it is printed:
	// infix with `symbol`, or as `function(a, b)` when symbol is 0.
	struct OperationShape
	{
		const char* tag;
		const char* first;
		const char* rest;
		char symbol;
		const char* function{ nullptr };
		std::size_t fewest{ 2 };
		std::size_t most{ 0 };
		const char* last{ nullptr };

		// <compare>: its second operand is named for the test, one of
		// comparisonWords (<value>a</value><atleast>b</atleast>), not `rest`.
		bool compares{ false };

		// The role the operand at `index` plays.
		const char* roleAt(std::size_t index) const noexcept
		{
			if (index == 0) { return first; }
			if (last && most > 0 && index + 1 == most) { return last; }
			return rest;
		}
	};

	// The shape of an arithmetic tag, or null when the tag is not one.
	const OperationShape* operationShape(const std::string& tag);

	// Every operation tag, for a mistake: "<add>, ... or <clamp>".
	std::string operationTags();

	// The tests of a <compare>, the same words as a condition's: above,
	// below, atleast, atmost, equal.
	bool isComparisonWord(const std::string& word);

	// A value as one line of text, the way a game is printed: an expression as
	// written, an operation tree in infix with its brackets.
	std::string valueText(const RawValue& value);

	// A <sound> as written (load/xml.cpp reads it, and load/expr.cpp turns it
	// into a SoundDesc, audio/sound.h).

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
		RawValue decay;     // its sound's or part's <decay>; empty text for none
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
		std::string path;   // a recording (<path>), in place of notes
	};

	// --- A verb's own data as written: what its tag says besides the names
	// every command may have, each part named for what it is there and a
	// std::optional when it may be left out. A struct is named for its tag
	// (RawInc for <inc>). A verb without one yet reads into RawCommand's
	// shared fields (plan CR7). values() lists its numbers, for
	// RawCommand::forEachValue.

	// <bounce>N</bounce>: the share of its speed it keeps; left out, all.
	struct RawBounce
	{
		std::optional<RawValue> share;
		static auto values(auto& own) { return std::tie(own.share); }
	};

	// <hop>: one distance as its content, or a diagonal's <x> across and <y>
	// up or down; its <seconds> through the air (left out: at once) and the
	// <arc> it is drawn off its way (left out: none). On cells, its <onto
	// class>es: what it may land on (nothing else written: one cell). A
	// <hunt>'s <hop> is one too, its direction chosen as it hunts.
	struct RawHop
	{
		std::optional<RawValue> distance;
		std::optional<RawVector2> diagonal;
		std::optional<RawValue> seconds;
		std::optional<RawValue> arc;
		std::vector<std::string> onto;
		static auto values(auto& own) { return std::tie(own.distance, own.diagonal, own.seconds, own.arc); }
	};

	// <spawn object="...">: its <count> (left out: one); where the copy goes
	// (<position>, a half may be <centered>; on a cell map a <column> and a
	// <row>, or <free /> with the row it goes <below>), how it is going
	// (<velocity>, <acceleration>), each half left out when not written; and
	// of a group, <random /> or the <member> of that number.
	struct RawSpawn
	{
		std::optional<RawValue> count;
		std::optional<RawValue> positionX;
		std::optional<RawValue> positionY;
		std::optional<RawValue> velocityX;
		std::optional<RawValue> velocityY;
		std::optional<RawValue> accelerationX;
		std::optional<RawValue> accelerationY;
		std::optional<RawValue> column;
		std::optional<RawValue> row;
		bool free{ false };
		std::optional<RawValue> freeBelow;
		bool random{ false };
		std::optional<RawValue> member;
		static auto values(auto& own)
		{
			return std::tie(own.count, own.positionX, own.positionY, own.velocityX, own.velocityY, own.accelerationX, own.accelerationY,
				own.column, own.row, own.freeBelow, own.member);
		}
	};

	// <inc variable="...">N</inc> and <dec>: by how much; left out, 1.
	struct RawInc { std::optional<RawValue> by; static auto values(auto& own) { return std::tie(own.by); } };
	struct RawDec { std::optional<RawValue> by; static auto values(auto& own) { return std::tie(own.by); } };

	// <set variable="...">N</set>: what it is set to.
	struct RawSet { RawValue value; static auto values(auto& own) { return std::tie(own.value); } };

	// <jump>N</jump>: how high.
	struct RawJump { RawValue height; static auto values(auto& own) { return std::tie(own.height); } };

	// <knock>N</knock>: how much of the knock bounces back, 0 to 1; left out, all.
	struct RawKnock { std::optional<RawValue> bounce; static auto values(auto& own) { return std::tie(own.bounce); } };

	// <pass member="N" />: whose turn it is next; left out, the next player's.
	struct RawPass { std::optional<RawValue> member; static auto values(auto& own) { return std::tie(own.member); } };

	// <reveal object="...">N</reveal>: how many are turned up; left out, one.
	struct RawReveal { std::optional<RawValue> count; static auto values(auto& own) { return std::tie(own.count); } };

	// <go>, <wander> and <climb>: how fast, in pixels a second.
	struct RawGo { RawValue speed; static auto values(auto& own) { return std::tie(own.speed); } };
	struct RawWander { RawValue speed; static auto values(auto& own) { return std::tie(own.speed); } };
	struct RawClimb { RawValue speed; static auto values(auto& own) { return std::tie(own.speed); } };

	// <follow path="..."><stagger>s</stagger><nearest>N</nearest></follow>:
	// the seconds between each one setting off, and how many nearest set
	// off (left out: all).
	struct RawFollow { std::optional<RawValue> stagger, nearest; static auto values(auto& own) { return std::tie(own.stagger, own.nearest); } };

	// <move direction="...">step<wait>s</wait></move>.
	struct RawMove { RawValue step; std::optional<RawValue> wait; static auto values(auto& own) { return std::tie(own.step, own.wait); } };

	// <turn direction="...">degrees a second</turn>; left out on a cell map.
	struct RawTurn { std::optional<RawValue> rate; static auto values(auto& own) { return std::tie(own.rate); } };

	// <thrust>N</thrust>: how hard it pushes.
	struct RawThrust { RawValue amount; static auto values(auto& own) { return std::tie(own.amount); } };

	// <deflect>degrees</deflect>: the widest angle it goes back off at.
	struct RawDeflect { RawValue widest; static auto values(auto& own) { return std::tie(own.widest); } };

	// <hole>radius<rough>share</rough></hole>.
	struct RawHole { RawValue radius; std::optional<RawValue> rough; static auto values(auto& own) { return std::tie(own.radius, own.rough); } };

	// <swing><degrees>N</degrees><seconds>s</seconds></swing>.
	struct RawSwing { RawValue degrees, seconds; static auto values(auto& own) { return std::tie(own.degrees, own.seconds); } };

	// <chase object="...">speed<near>N</near></chase>.
	struct RawChase { RawValue speed; std::optional<RawValue> near; static auto values(auto& own) { return std::tie(own.speed, own.near); } };

	// <aim object="..."><lined>N</lined></aim>: only when lined up within N.
	struct RawAim { std::optional<RawValue> lined; static auto values(auto& own) { return std::tie(own.lined); } };

	// <shift object="..."><x>N</x><y>N</y></shift>: by how far.
	struct RawShift { std::optional<RawValue> x, y; static auto values(auto& own) { return std::tie(own.x, own.y); } };

	// <hunt object="...">speed<ahead>N</ahead><with object="..." />
	// <retreat object="...">N</retreat><search /></hunt> in a maze (a hunt
	// by hops is a RawHop): tiles ahead of the one hunted, a partner, what
	// it backs off to within that many tiles, and the shortest way.
	struct RawHunt
	{
		RawValue speed;
		std::optional<RawValue> ahead;
		std::string partner;
		std::string retreat;
		RawValue retreatWithin;
		bool search{ false };
		static auto values(auto& own) { return std::tie(own.speed, own.ahead, own.retreatWithin); }
	};

	// <send>: on a cell map, the cell it goes to; holder to holder (and a
	// <distribute>), how many, or <all />.
	struct RawSend
	{
		std::optional<RawValue> column, row;
		std::optional<RawValue> count;
		bool all{ false };
		static auto values(auto& own) { return std::tie(own.column, own.row, own.count); }
	};

	// <pick><only /><most>N</most></pick>: pick the only piece that can
	// move; from how many holders a picker may hold one at once.
	struct RawPick { bool only{ false }; int most{ 0 }; static auto values(auto&) { return std::tie(); } };

	// <think button="..."><depth>N</depth><avoid /><worth sprite="...">N</worth></think>.
	struct RawThink
	{
		std::string button;
		std::optional<RawValue> depth;
		bool avoid{ false };
		std::vector<std::pair<std::string, float>> worths;
		static auto values(auto& own) { return std::tie(own.depth); }
	};

	// <hold button="..." />: the key the computer holds down.
	struct RawHold { std::string button; static auto values(auto&) { return std::tie(); } };

	// <alternate><player state="..." />...</alternate>: each player's state
	// (empty for none).
	struct RawAlternate { std::vector<std::string> players; static auto values(auto&) { return std::tie(); } };

	// <track><onto class="..." /></track> on cells.
	struct RawTrack { std::vector<std::string> onto; static auto values(auto&) { return std::tie(); } };

	// <trigger>: in a key's input, how many commands after it are its
	// <otherwise>; with class=, the cells about the one running it.
	struct RawTrigger { int otherwise{ 0 }; CellPlace cells; static auto values(auto&) { return std::tie(); } };

	// <count class="..." into="..."><here /> or <around /></count>.
	struct RawCount { CellPlace cells; static auto values(auto&) { return std::tie(); } };

	using RawOwn = std::variant<std::monostate, RawBounce, RawHop, RawSpawn,
		RawInc, RawDec, RawSet, RawJump, RawKnock, RawPass, RawReveal, RawGo, RawWander, RawClimb,
		RawFollow, RawMove, RawTurn, RawThrust, RawDeflect, RawHole, RawSwing, RawChase, RawAim, RawShift,
		RawHunt, RawSend, RawPick, RawThink, RawHold, RawAlternate, RawTrack, RawTrigger, RawCount>;

	// Each number among a verb's own parts that was written: a RawValue, or
	// an optional one, or pair, that holds something.
	template <typename Visit, typename... Parts> void visitValues(Visit& visit, Parts&... parts)
	{
		// A verb with no numbers has nothing to visit.
		if constexpr (sizeof...(Parts) > 0)
		{
			const auto one = [&](auto& part)
			{
				using Part = std::remove_const_t<std::remove_reference_t<decltype(part)>>;
				if constexpr (std::is_same_v<Part, RawValue>) { visit(part); }
				else if constexpr (std::is_same_v<Part, std::optional<RawValue>>) { if (part) { visit(*part); } }
				else
				{
					static_assert(std::is_same_v<Part, std::optional<RawVector2>>, "a verb's number is a RawValue, an optional one or an optional pair");
					if (part) { visit(part->x); visit(part->y); }
				}
			};
			(one(parts), ...);
		}
	}

	// One command tag from the XML - <bounce/>, <inc variable="a.b"/>,
	// <move direction="up">step</move>, ... - before its number is worked out.
	// `verb` is the tag name; the names below are the attributes any command
	// may have, and its numbers and the rest of what it says are its own
	// (`own`).

	struct RawCommand
	{
		std::string verb;
		int line{ 0 }; // of its tag in the game file; 0 when not known (a generated program)
		std::string object;    // reset, fire, release, trigger, chase, aim
		std::string variable;  // inc, dec, copy (the one read), set (the one written)
		std::string into;      // copy (the one written)
		std::string state;     // push
		std::string action;    // trigger
		std::string direction; // move, hop, thrust, turn, climb
		std::string burn;      // thrust
		std::string sound;     // play
		std::string sprite;    // become
		std::string style;     // become: the palette its bitmaps take
		std::string path;      // follow
		std::string objClass;  // climb (what it climbs)
		std::string scene;     // enter, reset
		std::string way;       // enter, reset: a way of the current scene
		std::string from;      // become, trigger: only one showing this look
		// condition (a <condition> in an action): its test, and its commands
		// as the test's own (shared; copied before it is changed).
		std::shared_ptr<RawCondition> test;

		// The verb's own data, as written, for a verb that has any (RawOwn).
		RawOwn own;

		// Every number it holds, written or not: what self, a copy's name, a
		// scene's names and live numbers are looked for in. Not its test's.
		template <typename Visit> void forEachValue(Visit&& visit) { eachValue(*this, visit); }
		template <typename Visit> void forEachValue(Visit&& visit) const { eachValue(*this, visit); }

	private:
		template <typename Command, typename Visit> static void eachValue(Command& command, Visit& visit)
		{
			std::visit([&](auto& own)
			{
				using Own = std::remove_const_t<std::remove_reference_t<decltype(own)>>;
				if constexpr (!std::is_same_v<Own, std::monostate>)
				{
					std::apply([&](auto&... parts) { visitValues(visit, parts...); }, Own::values(own));
				}
			}, command.own);
		}
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

	// A count of seconds that comes this close to zero has reached it, so the
	// float sums of many short steps do not miss their moment by a hair.
	inline constexpr float kTimeSlack = 1e-4f;

	// A <timer>, on an object or in a state, made ready to run. It counts
	// the seconds of each step down; a step longer than what is left sets it
	// off and the rest counts toward the next round, so it goes off as often
	// as the time passed owes, however that time is cut into steps. The
	// interval is kept as written and worked out again each time the timer
	// starts over, so <every><random><min>1</min><max>3</max></random></every> waits a
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

		// name="fuse": on an object, its time left is the object's number of
		// that name (bomb.fuse), whole seconds rounded up, kept by
		// Game::updateTimers; `seconds` is what it starts at. In a state or a
		// scene the name only lets a from= copy replace it.
		std::string name;
		float seconds{};

		// Seconds until it goes off, while `counting`; not counting means not
		// started yet (worked out on the first step it counts). `done` is a
		// once-only timer that has gone off.
		// A double: a long timer counted down in many short steps stays
		// true to the ten-thousandth of a second.
		// A state's timer keeps its own; an object's are the object's
		// (Object::clocks), its timers being its kind's (Kind::timers).
		struct Clock
		{
			double secondsLeft{};
			bool counting{ false };
			bool done{ false };
		};
		Clock clock;
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
		std::optional<RawVector2> start;  // <start>, when written
		std::vector<RawPathStep> steps;
	};

	struct PathStep
	{
		bool home{ false };
		Vector2f by;
		std::vector<Command> commands;
		// A half that names something that changes (ship.x - self.x, where
		// self is the follower) is worked out as the step sets off.
		std::shared_ptr<const RawValue> liveX;
		std::shared_ptr<const RawValue> liveY;
		// Commands that say something of the follower (self.pays, a
		// <trigger> with no object=) are its own: made for each object that
		// can fly the path when the game loads (`ownBy`, by its name), and
		// for a copy made while it runs the first time it flies it, from
		// `ownRaw` with self made that one's name. `commands` are the rest.
		std::shared_ptr<const std::vector<RawCommand>> ownRaw;
		std::map<std::string, std::vector<Command>> ownBy;
	};

	// A path: a way through the window that objects <follow>, written once
	// and flown by any number of them (Galaxian's aliens flying in and
	// diving). It is a list of legs flown one after the other at `speed`
	// pixels a second: a step moves the object by `by` from wherever the step
	// began, and runs its commands (an object's own, as on a timer: firing,
	// a sound) as it sets off; home takes it back to where it started the
	// game (its <position>, moved by any <shift>), however far it has come.
	// With a start, the object is first put there; without, it sets off from
	// where it is. The numbers are worked out once, when the game loads,
	// except a step's that name something that changes (PathStep::liveX). A follower is moved
	// by its velocity like anything else, so it collides, and a <wrap /> on
	// the way carries it round without changing what is left of the step.
	struct Path
	{
		std::string name;
		float speed{};
		std::optional<Vector2f> start;  // where 0 is; when written
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

	// The picture's size, with no window needed: a circle's box and a
	// rectangle exactly as written (Object::pictureSize rounds them up to
	// the pixels drawn), a Line's bitmap, a text as drawText draws it, an
	// image's pixels (Picture::image, read as the game loads).
	// ShapeKind::Unknown is {0,0}.
	Vector2f measureShapeSize(const Picture& picture);

	// If `expression` is nothing but an "owner.variable" reference (for
	// example paddle1.score, or playing.room1.tally.kills, whose owner is a
	// path), returns {owner, variable}, split at the last dot. Returns nullopt
	// for anything else - a number, a sum, a name with no owner. Used to tell a <number> in a text sprite that should stay live
	// (bound to that variable) from one that is just worked out once; it is a
	// plain scan of the text, since once the text is worked out only the
	// number is left.
	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression);

	// Formats a live numeric value for on-screen display: whole numbers print
	// without a decimal point (scores, HP, ammo, ...), matching what someone
	// hand-typing a <text> content of 0 would have written.
	std::string formatDisplayNumber(float value);

	std::ostream& operator<<(std::ostream& o, const Command& command);
	std::ostream& operator<<(std::ostream& o, const Picture& picture);

	// How a value and a command tag read in printGame()'s output.
	std::ostream& operator<<(std::ostream& o, const RawValue& value);
	std::ostream& operator<<(std::ostream& o, const RawCommand& command);
	std::ostream& operator<<(std::ostream& o, const std::vector<RawCommand>& commands);
}
