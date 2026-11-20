// cells.h
// XML Game Engine
// author: beefviper
// date: Nov 20, 2026

#pragma once

#include "game/command.h"
#include "game/name.h"
#include "game/object.h"

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	class Game;
	class CommandExecutor;

	// Maps with <cells /> and the board games on them (game/cells.cpp). Their
	// pieces move a cell at a time; a move that is refused (a cell off the
	// map, or holding another's piece) touches what refused it: the map's
	// edge, or the piece in the way. The game holds one (Game::cellMaps).
	class CellMaps
	{
	public:
		explicit CellMaps(Game& owner) noexcept : game(owner) {}

		// A board game piece's moves that it can make now: where each ends,
		// and the piece it jumps (null for a step). Only its jumps when
		// `jumpsOnly`, or when its set is forced and a piece of its class
		// can jump.
		struct PieceMoveTo
		{
			int column{};
			int row{};
			Object* taken{ nullptr };
		};
		// Puts a piece in that cell of its map of cells.
		void putInCell(Object& object, int column, int row);
		// A spawned copy on a cell map, put in that cell (its top left one,
		// for a shape, whose cells are spawned with it); false, and the
		// copy taken back, when a cell it needs is off the map or full.
		// `map`: another map of cells it goes on instead of its own.
		bool putOnCells(Object& made, int column, int row, Name map = {});
		// <send>: the first piece of that name or group on map `from` (a
		// shape with its cells, or a loose piece) goes to map `into`, in
		// that cell (left out: the one it is in); false when there is none
		// or it has no room there.
		bool sendCells(Name target, Name from, Name into, std::optional<int> column, std::optional<int> row);
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
		// <go> on a map of cells: that way a cell at a time, `speed` cells
		// a second, until told otherwise; false (refused) straight back into
		// its chain's first link.
		bool goCells(Object& mover, Direction way, float speed);
		// <split />: the chain the piece is in is cut there (see CmdSplit).
		void splitChain(Object& piece);
		// <shove />: the piece in the way of a refused step goes one cell on
		// through the side met (`way`, the mover's side); false when it
		// cannot (after its own touch).
		bool shoveCells(Object& piece, Edge way);
		// <merge variable>: the piece in the way, holding the same number,
		// is taken away and its number added to the mover's; false when
		// they differ or either has merged in this slide.
		bool mergeCells(Object& mover, Object& other, Name variable);
		// <slide class>: every piece of that class, nearest that way first;
		// false when none moved.
		bool slideClass(Name objClass, Direction way);
		// The pieces of that class (any, when empty) in the cells about
		// `piece` that `place` names, on every map of cells laid where its
		// own is, in reading order.
		std::vector<Object*> piecesAbout(const Object& piece, Name objClass, const CellPlace& place);
		// A <spawn><free />: in a free cell, at random (none: taken back,
		// false).
		bool putInFreeCell(Object& made, Name map = {}, int below = 0);
		// <fall />: every loose piece of the map, with the pieces joined to
		// it, down onto what is under it.
		void fallCells(Name map, Name objClass);
		// <joins>: each piece that has them shows the look for where the
		// piece it is joined to lies; at the end of every frame.
		void showJoins();
		// <cycle />: a shape's cells each on to the next of its outline.
		void cycleCells(Object& shape, bool backward = false);
		// <pick class>: the piece of that class in the picker's cell, with a
		// move it can make, is the one it holds; false when there is none,
		// or while the one it holds must jump on.
		// `only`: the one piece of the class with a move, wherever it is,
		// when just one has.
		bool pickPiece(Object& picker, Name objClass, bool only = false);
		// <drop /> on a picker: the piece held is put back (it stays where
		// it is); false with none held, or in the middle of a run of jumps.
		bool dropPiece(Object& picker);
		// <put class>: a new piece of that class in the picker's cell (see
		// CmdPut); false when it may not go there.
		bool putPiece(Object& picker, const CmdPut& put);
		// The piece a <put> of that class copies: the first written of the
		// class on a map of cells; null for none. putTemplates: the first of
		// each group of the class (a put takes the first that fits).
		Object* putTemplate(Name objClass);
		std::vector<Object*> putTemplates(Name objClass);
		// The pieces a new piece of `kind`'s class put in that cell of
		// `kind`'s map would trap by its <flank>s; `can` is whether it may
		// go there at all (a free cell, and something trapped when it has
		// flanks).
		std::vector<Object*> flanked(const Object& kind, int column, int row, bool& can);
		// The cells a new piece of `kind`'s class may be put in.
		std::vector<std::pair<int, int>> putCells(const Object& kind);
		// <count class>: the objects of that class in play, those spawned
		// this frame too.
		float countClass(Name objClass);
		// <track /> by a pointer, for a piece on a map of cells: into the
		// cell under `at` (a pixel across, or down), when it is on the map.
		void trackCells(Object& object, bool horizontal, float at, const std::vector<Name>& onto = {});
		bool hopOnto(Object& mover, Direction direction, const std::vector<Name>& onto);
		std::vector<std::pair<int, int>> ontoCells(const std::vector<Name>& onto);
		// <marks>: each picker's marks put in their cells (game/cells.cpp).
		void showMarks();
		// <place />: the piece held makes the move that ends in the picker's
		// cell, taking what it jumps; false when none does, or when it can
		// jump on (it stays held, and must).
		bool placePiece(Object& picker);
		// Every piece's moveCount (the live number `moves`), counted again.
		void countMoves();
		Object* cellHolder(Name map, int column, int row, const std::vector<Object*>& mover);

		std::vector<Object*> cellsOf(Object& mover);
		// Onto another map of cells: its place, tile, size and checks, as
		// any piece of that map has them (false for a map with none).
		bool moveToMap(Object& object, Name map);
		void askChecks(Name map);
		// A <chain>: whether a piece is one of its links; the links of a
		// head in order (found from where they lie when not yet known); and
		// the links brought after a head that has stepped from that cell.
		bool isLink(const Object& piece, Name chain);
		std::vector<Object*> chainOf(Object& head);
		void followChain(Object& head, const std::vector<Object*>& links, int column, int row);
		void askLines(const Object::CellCheck& check, Name map, int columns, int rows, CommandExecutor& executor);

		const Object::MoveSet* moveSetOf(const Object& piece) const noexcept;
		std::vector<PieceMoveTo> movesOf(Object& piece, bool jumpsOnly);
		std::vector<PieceMoveTo> legalMoves(Object& piece, bool jumpsOnly);

		// The cells a refused step was going into, while its touch's rules
		// run: no <free /> spawn goes there.
		struct Stepping
		{
			Name map;
			int column{};
			int row{};
		};
		std::vector<Stepping> stepping;
		// The pieces that have merged in this slide (or this press).
		std::vector<const Object*> merged;

		// <undo />: back as before the last press that moved a piece on
		// cells; false when there is none left. A press begins and ends
		// (Engine::runAsPress) so that each one that moves a piece is kept.
		// Kept only in a game with an <undo /> (keepUndo, at load), and
		// forgotten on a new level.
		bool undo();
		void pressBegins();
		void pressEnds();
		void keepUndo() noexcept { keepsUndo = true; }
		void forgetUndo() noexcept { undoSteps.clear(); }

	private:
		Game& game;
		// How every object was before each press that moved a piece on
		// cells, the last last; the one taken as this press began; whether
		// the game has an <undo /> at all, and this press undid.
		struct Kept
		{
			Name name;
			Moment moment;
			Object::Cells::Place place;
			std::size_t look{};
		};
		std::vector<std::vector<Kept>> undoSteps;
		std::vector<Kept> pressKept;
		bool keepsUndo = false;
		bool undoneThisPress = false;
		std::vector<Kept> keepAll() const;
		// While nothing moves (counting the moves, laying out the marks):
		// who holds each cell, found once per map and looked up after
		// (cellHolder with no mover), instead of looking at every object
		// for every cell. Held for as long as the returned guard lives.
		struct HeldCells
		{
			bool on{ false };
			std::unordered_map<Name, std::unordered_map<long long, Object*>> maps;
		};
		struct CellsStill
		{
			CellMaps& maps;
			bool first;  // the outermost guard lets go
			explicit CellsStill(CellMaps& owner) : maps(owner), first(!owner.heldCells.on) { maps.heldCells.on = true; }
			~CellsStill() { if (first) { maps.heldCells = {}; } }
			CellsStill(const CellsStill&) = delete;
			CellsStill& operator=(const CellsStill&) = delete;
		};
		HeldCells heldCells;
	};
}
