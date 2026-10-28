// maze.h
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026

#pragma once

#include "game/command.h"
#include "play/frame.h"
#include "game/object.h"
#include "game/types.h"

#include <vector>

namespace xge
{
	// Walking a <map>'s tiles: Pac-Man and the ghosts.
	//
	// An object with a <maze map="maze"> walks the tiles of that map. It keeps
	// to the middle of a lane (the row or column it is in), chooses its way
	// only in the middle of a tile, and never goes onto a tile that holds a
	// wall: an object in play on that map of a class its <maze> names, while
	// the walker shows that wall's look (any look, when it names none). Off
	// the map (a tunnel out to the window's edge) it goes straight on.
	//
	// How it chooses is what its last <go>, <hunt> or <wander> said (Walk):
	// - <go direction="left">: that way at the first tile it can go that way,
	//   straight on until then, and stopped by a wall; turned straight round,
	//   at once. A player's stick.
	// - <hunt object="pacman">: at each tile, of the ways open but back, the
	//   one whose next tile is nearest its target (CmdHunt). There is no
	//   search of the maze: this is the arcade's ghosts, and how they get
	//   stuck is part of how they play. A dead end turns it round.
	// - <wander>: of the same ways, one at random.
	//
	// It faces the way it goes (and turns its heading, if it has one), and
	// keeps the speed it was told, times the share of a <slow> for a tile
	// that holds something of its class (Pac-Man's ghosts in the tunnel);
	// a stopped walker chooses again at once,
	// any way open. A <reverse /> turns it round where it is, and it then
	// chooses again at the next tile. While it follows a <path> or is in the
	// air, the path or the jump moves it instead.

	// <go>, <hunt> and <wander>: what the walker does from now on.
	void startGo(Object& walker, const CmdGo& go);
	void startHunt(Object& walker, const CmdHunt& hunt);
	void startWander(Object& walker, const CmdWander& wander);

	// This frame's choosing, for every walker in play: its velocity (and,
	// where it turns, its place, so that it turns exactly in the middle of a
	// tile) for the move playOneFrame makes next.
	void walkMazes(std::vector<Object>& objects, FrameRules& rules);
}
