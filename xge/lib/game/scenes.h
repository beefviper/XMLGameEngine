// scenes.h
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026

#pragma once

#include "game/data.h"

namespace xge
{
	// Lays out what states and scenes own among the game's own objects, once,
	// before anything is worked out (Game's constructor), the same for a game
	// read from its file and one a generated program wrote out.
	//
	// Each scene's objects are made: what it is from= first, then, in the
	// order written, the pieces it takes from its state's kit (or from that
	// scene) with what it changes in them, and its own. Each copy is its own
	// object, named by its path: the state, the scene and the name as
	// written (playing.room1.robots.3); a state's own objects by the state and
	// the name (playing.harry). They join the game's objects after the game's
	// own, each state's scenes' first and then its own, so a state's objects
	// are drawn over its scenes'. A scene's <variables> join the game's,
	// named by its path too (playing.wave2.pace).
	//
	// Then every name the game uses (in an expression, a command, a rule, a
	// condition, a <show>) is written out in full, looked for from the inside
	// out where it is used: in its scene, its state, then the game, then as
	// the end of exactly one path; so the rules of a room say robots and mean
	// their own. A name given twice where both can be seen, a path that could
	// mean two things, and a take of nothing stop the load (std::runtime_error).
	//
	// A game whose states own nothing is left exactly as it was.
	void layOutScenes(GameData& data);
}
