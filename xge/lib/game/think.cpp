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
// pressing the button there, as a player would. A game whose pieces are
// put rather than moved (<flank>) is played the same way: each move is a
// new piece in a cell and the pieces it turns, a side with no move passes,
// and the game is over when neither side can move.

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
		// takes, and the look it ends with. A put: a new piece (index
		// `piece`, at the end of the table) in its one cell, and the pieces
		// it turns.
		struct Move
		{
			std::size_t piece{};
			std::vector<std::pair<int, int>> cells;
			std::vector<std::size_t> taken;
			std::size_t look{};
			bool put{ false };
			int side{ 0 };
			std::vector<std::size_t> turned;
		};

		// A <line> check of the board's map: so many alike in a row (of
		// that class, or any) wins for the side that makes it.
		struct LineRule
		{
			std::string objClass;
			int count{};
			bool straight{ false };
		};

		struct Board
		{
			std::vector<Piece> pieces;
			int columns{};
			int rows{};
			std::vector<int> grid;    // a piece's index + 1 in each cell, 0 for none
			std::vector<std::string> sides;
			// Each side's piece a put copies (none: its pieces are moved).
			const Object* kinds[2]{ nullptr, nullptr };
			std::vector<LineRule> lines;

			int& at(int column, int row) { return grid[static_cast<std::size_t>((row - 1) * columns + (column - 1))]; }
			bool onBoard(int column, int row) const { return column >= 1 && row >= 1 && column <= columns && row <= rows; }
			bool empty(int column, int row) { return onBoard(column, row) && at(column, row) == 0; }

			static const Object::MoveSet* setOf(const Object& object, std::size_t look)
			{
				const Object::MoveSet* any = nullptr;
				for (const Object::MoveSet& set : object.moveSets)
				{
					if (set.look && *set.look == look) { return &set; }
					if (!set.look && !any) { any = &set; }
				}
				return any;
			}
			const Object::MoveSet* setOf(const Piece& piece) const { return setOf(*piece.object, piece.look); }

			// Whether the side's pieces are put rather than moved.
			bool puts(int side) const
			{
				const Object* kind = kinds[side];
				const Object::MoveSet* set = kind ? setOf(*kind, kind->look) : nullptr;
				return set && set->puts();
			}

			// Whether the side's pieces turn others (reversi): then a game
			// that stops is won by more pieces; else it is drawn.
			bool flanks(int side) const
			{
				const Object* kind = kinds[side];
				const Object::MoveSet* set = kind ? setOf(*kind, kind->look) : nullptr;
				return set && !set->flanks.empty();
			}

			// Whether the piece just put makes a line of a <line> rule.
			bool madeLine(const Move& move)
			{
				if (!move.put || lines.empty()) { return false; }
				const auto [column, row] = move.cells.front();
				const auto ours = [&](int c, int r)
				{
					if (!onBoard(c, r) || at(c, r) == 0) { return false; }
					return pieces[static_cast<std::size_t>(at(c, r) - 1)].side == move.side;
				};
				const int ways[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { -1, 1 } };
				for (const LineRule& rule : lines)
				{
					if (!rule.objClass.empty() && kinds[move.side]->objClass != rule.objClass) { continue; }
					for (int w = 0; w < (rule.straight ? 2 : 4); ++w)
					{
						int length = 1;
						for (int c = column + ways[w][0], r = row + ways[w][1]; ours(c, r); c += ways[w][0], r += ways[w][1]) { ++length; }
						for (int c = column - ways[w][0], r = row - ways[w][1]; ours(c, r); c -= ways[w][0], r -= ways[w][1]) { ++length; }
						if (length >= rule.count) { return true; }
					}
				}
				return false;
			}

			// How near each side is to a line: every run of cells as long as
			// a line with pieces of one side only, the more the better.
			float lineScore()
			{
				float total = 0.0f;
				const int ways[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { -1, 1 } };
				for (const LineRule& rule : lines)
				{
					for (int row = 1; row <= rows; ++row)
					{
						for (int column = 1; column <= columns; ++column)
						{
							for (int w = 0; w < (rule.straight ? 2 : 4); ++w)
							{
								const int endColumn = column + ways[w][0] * (rule.count - 1);
								const int endRow = row + ways[w][1] * (rule.count - 1);
								if (!onBoard(endColumn, endRow)) { continue; }
								int counts[2]{ 0, 0 };
								for (int i = 0; i < rule.count; ++i)
								{
									const int there = at(column + ways[w][0] * i, row + ways[w][1] * i);
									if (there == 0) { continue; }
									const int side = pieces[static_cast<std::size_t>(there - 1)].side;
									if (side >= 0) { ++counts[side]; }
								}
								if (counts[1] == 0 && counts[0] > 0) { total += static_cast<float>(counts[0] * counts[0]); }
								if (counts[0] == 0 && counts[1] > 0) { total -= static_cast<float>(counts[1] * counts[1]); }
							}
						}
					}
				}
				return total;
			}

			// Every put the side can make: a free cell, and the lines it traps.
			void putMoves(int side, std::vector<Move>& out)
			{
				const Object* kind = kinds[side];
				const Object::MoveSet* set = setOf(*kind, kind->look);
				for (int row = 1; row <= rows; ++row)
				{
					for (int column = 1; column <= columns; ++column)
					{
						if (at(column, row) != 0) { continue; }
						// Falling: onto the floor or a piece only.
						if (set->fall && row < rows && at(column, row + 1) == 0) { continue; }
						Move put;
						put.put = true;
						put.side = side;
						put.piece = pieces.size();
						put.cells.emplace_back(column, row);
						put.look = kind->look;
						if (set->flanks.empty())
						{
							out.push_back(std::move(put));
							continue;
						}
						for (const Object::Flank& flank : set->flanks)
						{
							for (int dy = -1; dy <= 1; ++dy)
							{
								for (int dx = -1; dx <= 1; ++dx)
								{
									if ((dx == 0 && dy == 0) || (flank.straight && dx != 0 && dy != 0)) { continue; }
									std::vector<std::size_t> line;
									for (int c = column + dx, r = row + dy; onBoard(c, r); c += dx, r += dy)
									{
										const int there = at(c, r);
										if (there == 0) { break; }
										const Piece& piece = pieces[static_cast<std::size_t>(there - 1)];
										if (piece.side != side && piece.object->objClass == flank.objClass)
										{
											line.push_back(static_cast<std::size_t>(there - 1));
											continue;
										}
										if (piece.side == side)
										{
											for (const std::size_t each : line)
											{
												if (std::find(put.turned.begin(), put.turned.end(), each) == put.turned.end()) { put.turned.push_back(each); }
											}
										}
										break;
									}
								}
							}
						}
						if (!put.turned.empty()) { out.push_back(std::move(put)); }
					}
				}
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
				if (puts(side))
				{
					std::vector<Move> putting;
					putMoves(side, putting);
					return putting;
				}
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
				if (move.put)
				{
					const int side = move.side;
					for (const std::size_t each : move.turned)
					{
						undo.taken.push_back(pieces[each]);
						pieces[each].object = kinds[side];
						pieces[each].look = kinds[side]->look;
						pieces[each].side = side;
					}
					Piece made;
					made.object = kinds[side];
					made.column = move.cells.front().first;
					made.row = move.cells.front().second;
					made.look = move.look;
					made.side = side;
					pieces.push_back(made);
					at(made.column, made.row) = static_cast<int>(pieces.size());
					return undo;
				}
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
				if (move.put)
				{
					at(move.cells.front().first, move.cells.front().second) = 0;
					pieces.pop_back();
					for (std::size_t i = 0; i < move.turned.size(); ++i) { pieces[move.turned[i]] = undo.taken[i]; }
					return;
				}
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
			float score()
			{
				if (!board.lines.empty()) { return board.lineScore(); }
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
				const float now = side == 0 ? score() : -score();
				if (moves.empty() && board.puts(side))
				{
					// No put: it passes; neither can: the game is over, won
					// by the side with more (sooner is better), or, when
					// pieces put turn nothing, drawn (a full board).
					if (board.moves(1 - side).empty())
					{
						if (!board.flanks(side)) { return 0.0f; }
						const float sooner = static_cast<float>(depth);
						return now > 0.0f ? decided + sooner + now : now < 0.0f ? -decided - sooner + now : 0.0f;
					}
					if (depth <= 0) { return now; }
					return -value(1 - side, depth - 1, -beta, -alpha);
				}
				if (moves.empty()) { return -(decided + static_cast<float>(depth)); }
				if (depth <= 0) { return now; }
				for (const Move& move : moves)
				{
					const Board::Undo undo = board.make(move);
					const float got = board.madeLine(move) ? decided + static_cast<float>(depth) : -value(1 - side, depth - 1, -beta, -alpha);
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
		// None on the board yet (pieces put): the one a put copies.
		if (!first) { first = putTemplate(think.objClass); }
		if (!first || first->moveSets.empty()) { return {}; }

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
			for (const Object::Flank& flank : set.flanks)
			{
				if (other.empty()) { other = flank.objClass; }
			}
		}
		// Neither: any other class of pieces with moves on the map.
		for (std::size_t i = 0; i < objects.size() && other.empty(); ++i)
		{
			const Object& object = objects[i];
			if (object.onCells && object.mapName == first->mapName && !object.moveSets.empty() && object.objClass != think.objClass) { other = object.objClass; }
		}
		board.kinds[0] = putTemplate(think.objClass);
		board.kinds[1] = other.empty() ? nullptr : putTemplate(other);
		// The map's checks, as its pieces written carry them (a copy may not).
		const Object* written = board.kinds[0] ? board.kinds[0] : first;
		for (const Object::CellCheck& check : written->cellChecks)
		{
			if (check.kind == "line") { board.lines.push_back({ check.objClass, check.count, check.straight }); }
		}
		if (!board.kinds[1]) { board.kinds[0] = nullptr; }
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
			const float got = board.madeLine(move) ? decided + static_cast<float>(think.depth)
				: -search.value(1, think.depth - 1, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
			board.unmake(move, undo);
			if (got > bestValue) { bestValue = got; best.clear(); }
			if (got == bestValue) { best.push_back(&move); }
		}
		if (best.empty()) { return {}; }
		const std::size_t pick = std::min(best.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(best.size()))));
		std::vector<std::pair<int, int>> cells = best[pick]->cells;
		// A piece that falls is dropped from the picker's own row.
		if (best[pick]->put)
		{
			const Object::MoveSet* set = Board::setOf(*board.kinds[0], board.kinds[0]->look);
			if (set && set->fall) { cells.front().second = whole(picker.cellRow); }
		}
		return cells;
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
