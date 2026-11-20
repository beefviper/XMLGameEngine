// maze.h
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026

#pragma once

#include "game/command.h"
#include "play/frame.h"
#include "game/object.h"
#include "game/types.h"

#include <functional>
#include <vector>

namespace xge
{
	// Walking a <map>'s tiles with a <maze>: in the middle of a lane, choosing
	// only in the middle of a tile, never onto a wall's tile, as its last
	// <go>, <hunt> or <wander> says (Walk). What each means:
	// docs/reference/maps.md. <hunt> without <search /> is the arcade's
	// nearest-tile rule, no search: how it gets stuck is part of the play.

	// The fewest steps from each tile of a grid `columns` by `rows` to the
	// tile at toColumn, toRow, going across or up and down and never onto
	// a tile `blocked` says holds a wall; -1 where there is no way. Row by
	// row: the tile at column, row is [row * columns + column].
	std::vector<int> mazeDistances(int columns, int rows, int toColumn, int toRow, const std::function<bool(int, int)>& blocked);

	// <go>, <hunt> and <wander>: what the walker does from now on.
	void startGo(Maze& maze, const CmdGo& go);
	void startHunt(Maze& maze, const CmdHunt& hunt);
	void startWander(Maze& maze, const CmdWander& wander);

	// This step's choosing, for every walker in play: its velocity (and,
	// where it turns, its place, so that it turns exactly in the middle of a
	// tile) for the move playOneFrame makes next, `t` long in the game's
	// units. A long step chooses at every middle it passes.
	void walkMazes(std::vector<Object>& objects, FrameRules& rules, float t);
}
