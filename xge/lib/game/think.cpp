// think.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// <think>: the computer playing a board game's pieces. The pieces on the
// board are copied into a small table (where each is, its side and its
// look), and every move each side could make is played out on it, a few
// moves deep, by the same rules the pieces have (<moves>: steps, jumps,
// forced jumps, runs of jumps, promotion). The best move is then made by
// walking the picker to the piece and to each cell of the move and
// pressing the button there, as a player would.

#include "game/game.h"
#include "game/random.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>

namespace xge
{
	namespace
	{
		int whole(float number) noexcept { return static_cast<int>(std::lround(number)); }

		// Frames between the picker's steps and presses, so a person can
		// follow the computer's move.
		constexpr int pace = 8;

		// A won or lost game, worth more than any count of pieces.
		constexpr float decided = 1.0e6f;

		struct Piece
		{
			const Object* object{ nullptr };
			int column{};
			int row{};
			std::size_t look{};
			int side{ -1 };       // 0 the computer's, 1 the other's, -1 neither (it only takes up room)
			bool alive{ true };
		};

		// One move: the piece, the cells it goes to in turn, the pieces it
		// takes, and the look it ends with.
		struct Move
		{
			std::size_t piece{};
			std::vector<std::pair<int, int>> cells;
			std::vector<std::size_t> taken;
			std::size_t look{};
		};

		struct Board
		{
			std::vector<Piece> pieces;
			int columns{};
			int rows{};
			std::vector<int> grid;    // a piece's index + 1 in each cell, 0 for none
			std::vector<std::string> sides;

			int& at(int column, int row) { return grid[static_cast<std::size_t>((row - 1) * columns + (column - 1))]; }
			bool onBoard(int column, int row) const { return column >= 1 && row >= 1 && column <= columns && row <= rows; }
			bool empty(int column, int row) { return onBoard(column, row) && at(column, row) == 0; }

			const Object::MoveSet* setOf(const Piece& piece) const
			{
				const Object::MoveSet* any = nullptr;
				for (const Object::MoveSet& set : piece.object->moveSets)
				{
					if (set.look && *set.look == piece.look) { return &set; }
					if (!set.look && !any) { any = &set; }
				}
				return any;
			}

			// The runs of jumps from where the piece is now, each to its end:
			// a piece taken is off the board at once, as the game despawns it
			// at once, and a promotion ends the run.
			void jumps(std::size_t index, Move so, std::vector<Move>& out)
			{
				const Piece& piece = pieces[index];
				const Object::MoveSet* set = setOf(piece);
				bool any = false;
				if (set)
				{
					for (const Object::PieceMove& move : set->moves)
					{
						if (!move.jump) { continue; }
						const int overColumn = piece.column + move.x;
						const int overRow = piece.row + move.y;
						const int toColumn = piece.column + 2 * move.x;
						const int toRow = piece.row + 2 * move.y;
						if (!empty(toColumn, toRow) || !onBoard(overColumn, overRow)) { continue; }
						const int over = at(overColumn, overRow);
						if (over == 0 || pieces[static_cast<std::size_t>(over - 1)].object->objClass != move.objClass) { continue; }
						any = true;

						// Made on the board, gone on with, then taken back.
						const std::size_t taken = static_cast<std::size_t>(over - 1);
						const Piece before = piece;
						pieces[taken].alive = false;
						at(overColumn, overRow) = 0;
						at(before.column, before.row) = 0;
						pieces[index].column = toColumn;
						pieces[index].row = toRow;
						at(toColumn, toRow) = static_cast<int>(index) + 1;

						Move next = so;
						next.cells.emplace_back(toColumn, toRow);
						next.taken.push_back(taken);
						bool promoted = false;
						if (set->promoteLook && toRow == set->promoteRow && pieces[index].look != *set->promoteLook)
						{
							pieces[index].look = *set->promoteLook;
							promoted = true;
						}
						next.look = pieces[index].look;
						if (promoted) { out.push_back(next); }
						else { jumps(index, next, out); }

						at(toColumn, toRow) = 0;
						pieces[index] = before;
						at(before.column, before.row) = static_cast<int>(index) + 1;
						pieces[taken].alive = true;
						at(overColumn, overRow) = static_cast<int>(taken) + 1;
					}
				}
				if (!any && !so.taken.empty()) { out.push_back(std::move(so)); }
			}

			// Every move the side can make, by the rules its pieces have.
			std::vector<Move> moves(int side)
			{
				std::vector<Move> jumping;
				std::vector<Move> stepping;
				bool forced = false;
				for (std::size_t i = 0; i < pieces.size(); ++i)
				{
					const Piece& piece = pieces[i];
					if (!piece.alive || piece.side != side) { continue; }
					const Object::MoveSet* set = setOf(piece);
					if (!set) { continue; }
					forced = forced || set->forced;
					Move start;
					start.piece = i;
					start.cells.emplace_back(piece.column, piece.row);
					start.look = piece.look;
					jumps(i, start, jumping);
					for (const Object::PieceMove& move : set->moves)
					{
						if (move.jump || !empty(piece.column + move.x, piece.row + move.y)) { continue; }
						Move step = start;
						step.cells.emplace_back(piece.column + move.x, piece.row + move.y);
						if (set->promoteLook && piece.row + move.y == set->promoteRow) { step.look = *set->promoteLook; }
						stepping.push_back(std::move(step));
					}
				}
				if (forced && !jumping.empty()) { return jumping; }
				jumping.insert(jumping.end(), stepping.begin(), stepping.end());
				return jumping;
			}

			struct Undo
			{
				Piece before;
				std::vector<Piece> taken;
			};

			Undo make(const Move& move)
			{
				Undo undo;
				Piece& piece = pieces[move.piece];
				undo.before = piece;
				at(piece.column, piece.row) = 0;
				for (const std::size_t taken : move.taken)
				{
					undo.taken.push_back(pieces[taken]);
					pieces[taken].alive = false;
					at(pieces[taken].column, pieces[taken].row) = 0;
				}
				piece.column = move.cells.back().first;
				piece.row = move.cells.back().second;
				piece.look = move.look;
				at(piece.column, piece.row) = static_cast<int>(move.piece) + 1;
				return undo;
			}

			void unmake(const Move& move, const Undo& undo)
			{
				Piece& piece = pieces[move.piece];
				at(piece.column, piece.row) = 0;
				piece = undo.before;
				at(piece.column, piece.row) = static_cast<int>(move.piece) + 1;
				for (std::size_t i = 0; i < move.taken.size(); ++i)
				{
					pieces[move.taken[i]] = undo.taken[i];
					at(undo.taken[i].column, undo.taken[i].row) = static_cast<int>(move.taken[i]) + 1;
				}
			}
		};

		struct Search
		{
			Board& board;
			const CmdThink& think;

			float worthOf(const Piece& piece) const
			{
				const std::string& look = piece.look < piece.object->looks.size() ? piece.object->looks[piece.look].name : std::string();
				for (const auto& [name, worth] : think.worths)
				{
					if (name == look) { return worth; }
				}
				return 1.0f;
			}

			// The pieces each side has, by their worth: the computer's less
			// the other's.
			float score() const
			{
				float total = 0.0f;
				for (const Piece& piece : board.pieces)
				{
					if (!piece.alive || piece.side < 0) { continue; }
					total += piece.side == 0 ? worthOf(piece) : -worthOf(piece);
				}
				return total;
			}

			// How good it is for `side` to move now, `depth` moves left to
			// look at (negamax with alpha-beta). A side with no move has lost;
			// sooner is worse, so it plays for the quickest win.
			float value(int side, int depth, float alpha, float beta)
			{
				const std::vector<Move> moves = board.moves(side);
				if (moves.empty()) { return -(decided + static_cast<float>(depth)); }
				if (depth == 0) { return side == 0 ? score() : -score(); }
				for (const Move& move : moves)
				{
					const Board::Undo undo = board.make(move);
					const float got = -value(1 - side, depth - 1, -beta, -alpha);
					board.unmake(move, undo);
					alpha = std::max(alpha, got);
					if (alpha >= beta) { break; }
				}
				return alpha;
			}
		};
	}

	std::vector<std::pair<int, int>> Game::bestMove(const CmdThink& think, const Object& picker)
	{
		// The board: the map of cells its pieces are on (the first piece of
		// its class with moves), and everything on that map.
		const Object* first = nullptr;
		for (const Object& object : objects)
		{
			if (object.objClass == think.objClass && !object.moveSets.empty() && object.onCells && isShown(object)) { first = &object; break; }
		}
		if (!first) { return {}; }

		Board board;
		board.columns = first->mapColumns;
		board.rows = first->mapRows;
		board.grid.assign(static_cast<std::size_t>(board.columns * board.rows), 0);
		// The other side: what its pieces' jumps take.
		std::string other;
		for (const Object::MoveSet& set : first->moveSets)
		{
			for (const Object::PieceMove& move : set.moves)
			{
				if (move.jump && other.empty()) { other = move.objClass; }
			}
		}
		for (const Object& object : objects)
		{
			if (!object.onCells || object.mapName != first->mapName || &object == &picker || !isShown(object)) { continue; }
			const int column = whole(object.cellColumn);
			const int row = whole(object.cellRow);
			if (!board.onBoard(column, row)) { continue; }
			Piece piece;
			piece.object = &object;
			piece.column = column;
			piece.row = row;
			piece.look = object.look;
			if (!object.moveSets.empty())
			{
				if (object.objClass == think.objClass) { piece.side = 0; }
				else if (other.empty() || object.objClass == other) { piece.side = 1; }
			}
			board.pieces.push_back(piece);
			board.at(column, row) = static_cast<int>(board.pieces.size());
		}

		// Every move it can make now, each played out; the best, one of
		// the equals at random.
		Search search{ board, think };
		const std::vector<Move> moves = board.moves(0);
		std::vector<const Move*> best;
		float bestValue = -std::numeric_limits<float>::infinity();
		for (const Move& move : moves)
		{
			const Board::Undo undo = board.make(move);
			const float got = -search.value(1, think.depth - 1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
			board.unmake(move, undo);
			if (got > bestValue) { bestValue = got; best.clear(); }
			if (got == bestValue) { best.push_back(&move); }
		}
		if (best.empty()) { return {}; }
		const std::size_t pick = std::min(best.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(best.size()))));
		return best[pick]->cells;
	}

	std::optional<KeyCode> Game::think(const CmdThink& think)
	{
		Object* picker = nullptr;
		const std::string name = fullName(think.picker);
		for (Object& object : objects)
		{
			if (object.exists && object.named(name)) { picker = &object; break; }
		}
		if (!picker || !picker->onCells) { return std::nullopt; }

		if (!thinking.planned)
		{
			thinking = {};
			thinking.planned = true;
			thinking.cells = bestMove(think, *picker);
			thinking.wait = pace;
		}
		if (--thinking.wait > 0) { return std::nullopt; }
		thinking.wait = pace;

		// Done, and still its turn (a press refused): think again.
		if (thinking.next >= thinking.cells.size())
		{
			thinking.planned = false;
			return std::nullopt;
		}

		// A cell at a time to the next cell of the move, then the button.
		const auto [column, row] = thinking.cells[thinking.next];
		const int atColumn = whole(picker->cellColumn);
		const int atRow = whole(picker->cellRow);
		if (atColumn != column || atRow != row)
		{
			const int across = column > atColumn ? 1 : column < atColumn ? -1 : 0;
			const int down = atColumn != column ? 0 : (row > atRow ? 1 : -1);
			putInCell(*picker, atColumn + across, atRow + down);
			return std::nullopt;
		}
		++thinking.next;
		return think.button;
	}
}
