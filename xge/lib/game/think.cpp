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
//
// The table is all the search reads: it holds its own copy of what it
// needs of each kind of piece, so the search can run beside the game, on a
// thread of its own, while the game goes on drawing and playing sounds.

#include "game/game.h"
#include "game/random.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <optional>
#include <thread>
#include <unordered_map>
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

		// What the search needs of a kind of piece: its class, its rules
		// and its looks (a copy, so the game may change while it thinks).
		struct Kind
		{
			std::string objClass;
			std::vector<Object::MoveSet> moveSets;
			std::size_t look{};
			std::vector<std::string> lookNames;

			// Worked out once the board is set (Board::prepare), so the
			// search compares no names: its place in the board's kinds, its
			// moves at its own look, the kinds that set goes <on>, and whether
			// it is a place pieces go on, or what a side closes or owns.
			std::size_t index{};
			const Object::MoveSet* ownSet{ nullptr };
			std::vector<char> goesOn;
			bool isPlace{ false };
			bool closedBy[2]{ false, false };
			bool ownedBy[2]{ false, false };
		};

		struct Piece
		{
			const Kind* object{ nullptr };
			int column{};
			int row{};
			std::size_t look{};
			int side{ -1 };       // 0 the computer's, 1 the other's, -1 neither (it only takes up room)
			bool alive{ true };
			int owner{ -1 };      // a piece <close>d in the search: the side it is claimed by
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
			const Kind* kind{ nullptr };  // a put: the piece it copies
			int under{ -1 };                // a put <on> a piece: that piece
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
			std::deque<Kind> types;   // every kind on the board (a deque: pointers to them stay good)
			std::map<const Object*, const Kind*> typeOf;
			std::vector<Piece> pieces;
			int columns{};
			int rows{};
			std::vector<int> grid;    // a piece's index + 1 in each cell, 0 for none
			std::vector<std::string> sides;
			// Each side's piece a put copies (none: its pieces are moved).
			const Kind* kinds[2]{ nullptr, nullptr };
			// Every piece each side may put (one of each group), the classes
			// they go <on>, and what a side's put <close>s and the class of
			// what it claims with.
			std::vector<const Kind*> allKinds[2];
			std::vector<std::string> places;
			std::string closeClass[2];
			std::string ownClass[2];
			std::vector<LineRule> lines;

			// The kind of a piece of the game, copied the first time.
			const Kind* kindOf(const Object* object)
			{
				if (!object) { return nullptr; }
				if (const auto found = typeOf.find(object); found != typeOf.end()) { return found->second; }
				Kind kind;
				kind.objClass = object->objClass;
				kind.moveSets = object->moveSets;
				kind.look = object->look;
				for (const auto& look : object->looks) { kind.lookNames.push_back(look.name); }
				types.push_back(std::move(kind));
				typeOf[object] = &types.back();
				return &types.back();
			}

			bool claims() const { return !closeClass[0].empty() || !closeClass[1].empty(); }

			// Each kind's names worked out against the others, once.
			void prepare()
			{
				std::size_t i = 0;
				for (Kind& kind : types)
				{
					kind.index = i++;
					kind.isPlace = std::find(places.begin(), places.end(), kind.objClass) != places.end();
					for (int side = 0; side < 2; ++side)
					{
						kind.closedBy[side] = !closeClass[side].empty() && kind.objClass == closeClass[side];
						kind.ownedBy[side] = !ownClass[side].empty() && kind.objClass == ownClass[side];
					}
				}
				for (Kind& kind : types)
				{
					kind.ownSet = setOf(kind, kind.look);
					kind.goesOn.assign(types.size(), 0);
					if (!kind.ownSet) { continue; }
					for (const Kind& other : types)
					{
						kind.goesOn[other.index] = std::find(kind.ownSet->on.begin(), kind.ownSet->on.end(), other.objClass) != kind.ownSet->on.end();
					}
				}
			}

			// What the board is, as one number: each piece in play by its
			// cell, kind, look, side and owner, in any order. Two orders of
			// the same moves come to the same number.
			std::uint64_t key() const
			{
				std::uint64_t all = 0;
				for (const Piece& piece : pieces)
				{
					if (!piece.alive) { continue; }
					std::uint64_t one = reinterpret_cast<std::uintptr_t>(piece.object);
					one = mix(one ^ (static_cast<std::uint64_t>(piece.column) << 40) ^ (static_cast<std::uint64_t>(piece.row) << 48));
					one = mix(one ^ (static_cast<std::uint64_t>(piece.look) << 8) ^ static_cast<std::uint64_t>(piece.side + 1) ^ (static_cast<std::uint64_t>(piece.owner + 1) << 4));
					all ^= one;
				}
				return all;
			}

			static std::uint64_t mix(std::uint64_t x)
			{
				// splitmix64's finish: every bit of x moves every bit out.
				x += 0x9e3779b97f4a7c15ull;
				x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
				x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
				return x ^ (x >> 31);
			}

			int& at(int column, int row) { return grid[static_cast<std::size_t>((row - 1) * columns + (column - 1))]; }
			bool onBoard(int column, int row) const { return column >= 1 && row >= 1 && column <= columns && row <= rows; }
			bool empty(int column, int row) { return onBoard(column, row) && at(column, row) == 0; }

			static const Object::MoveSet* setOf(const Kind& object, std::size_t look)
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
				const Kind* kind = kinds[side];
				const Object::MoveSet* set = kind ? setOf(*kind, kind->look) : nullptr;
				return set && set->puts();
			}

			// Whether the side's pieces turn others (reversi): then a game
			// that stops is won by more pieces; else it is drawn.
			bool flanks(int side) const
			{
				const Kind* kind = kinds[side];
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

			// Every put the side can make: a free cell (or one holding what
			// it goes <on>), and the lines it traps; the first of the side's
			// pieces that fits each cell.
			void putMoves(int side, std::vector<Move>& out, std::size_t enough = std::numeric_limits<std::size_t>::max())
			{
				std::vector<const Kind*> each = allKinds[side];
				if (each.empty()) { each.push_back(kinds[side]); }
				for (int row = 1; row <= rows; ++row)
				{
					for (int column = 1; column <= columns; ++column)
					{
						for (const Kind* kind : each)
						{
							const Object::MoveSet* set = kind->ownSet;
							if (!set) { continue; }
							const int there = at(column, row);
							int under = -1;
							if (!set->on.empty())
							{
								if (there == 0) { continue; }
								const Piece& piece = pieces[static_cast<std::size_t>(there - 1)];
								if (!kind->goesOn[piece.object->index]) { continue; }
								under = there - 1;
							}
							else if (there != 0) { continue; }
							// Falling: onto the floor or a piece only.
							if (set->fall && row < rows && at(column, row + 1) == 0) { continue; }
							Move put;
							put.put = true;
							put.side = side;
							put.kind = kind;
							put.under = under;
							put.piece = pieces.size();
							put.cells.emplace_back(column, row);
							put.look = kind->look;
							if (set->flanks.empty())
							{
								out.push_back(std::move(put));
								if (out.size() >= enough) { return; }
								break;
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
											const int other = at(c, r);
											if (other == 0) { break; }
											const Piece& piece = pieces[static_cast<std::size_t>(other - 1)];
											if (piece.side != side && piece.object->objClass == flank.objClass)
											{
												line.push_back(static_cast<std::size_t>(other - 1));
												continue;
											}
											if (piece.side == side)
											{
												for (const std::size_t one : line)
												{
													if (std::find(put.turned.begin(), put.turned.end(), one) == put.turned.end()) { put.turned.push_back(one); }
												}
											}
											break;
										}
									}
								}
							}
							if (!put.turned.empty())
							{
								out.push_back(std::move(put));
								if (out.size() >= enough) { return; }
								break;
							}
						}
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
			// Whether the side has any move at all: for a put, the first
			// one found is enough.
			bool canMove(int side)
			{
				if (!puts(side)) { return !moves(side).empty(); }
				std::vector<Move> one;
				putMoves(side, one, 1);
				return !one.empty();
			}

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
				std::vector<std::size_t> closed;   // pieces a put <close>d
			};

			// Walled in: no place left beside it that a piece goes <on>.
			bool walled(int column, int row)
			{
				const int around[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
				for (const auto& side : around)
				{
					const int c = column + side[0];
					const int r = row + side[1];
					if (!onBoard(c, r) || at(c, r) == 0) { return false; }
					const Piece& piece = pieces[static_cast<std::size_t>(at(c, r) - 1)];
					if (piece.object->isPlace) { return false; }
				}
				return true;
			}

			// Whether a put would close something: a piece of what its side
			// closes beside it, walled in but for this cell.
			bool closes(const Move& move)
			{
				if (!move.put || closeClass[move.side].empty()) { return false; }
				const int column = move.cells.front().first;
				const int row = move.cells.front().second;
				const int around[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
				for (const auto& way : around)
				{
					const int c = column + way[0];
					const int r = row + way[1];
					if (!onBoard(c, r) || at(c, r) == 0) { continue; }
					const Piece& box = pieces[static_cast<std::size_t>(at(c, r) - 1)];
					if (box.owner >= 0 || !box.alive || !box.object->closedBy[move.side]) { continue; }
					int open = 0;
					for (const auto& side : around)
					{
						const int bc = c + side[0];
						const int br = r + side[1];
						if (!onBoard(bc, br) || at(bc, br) == 0) { open = 2; break; }
						if (pieces[static_cast<std::size_t>(at(bc, br) - 1)].object->isPlace) { ++open; }
					}
					// Only the place this put goes on is still open.
					if (open == 1 && move.under >= 0) { return true; }
				}
				return false;
			}

			// A put that touches nothing yet: every piece of what its side
			// closes beside it has all its other places still free (a line
			// on an empty stretch of Dots and Boxes). Such moves are all
			// about as good as one another, so one of them stands for all.
			bool quiet(const Move& move)
			{
				if (!move.put || closeClass[move.side].empty()) { return false; }
				const int column = move.cells.front().first;
				const int row = move.cells.front().second;
				const int around[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
				for (const auto& way : around)
				{
					const int c = column + way[0];
					const int r = row + way[1];
					if (!onBoard(c, r) || at(c, r) == 0) { continue; }
					const Piece& box = pieces[static_cast<std::size_t>(at(c, r) - 1)];
					if (!box.alive || !box.object->closedBy[move.side]) { continue; }
					for (const auto& side : around)
					{
						const int bc = c + side[0];
						const int br = r + side[1];
						if (bc == column && br == row) { continue; }
						if (!onBoard(bc, br) || at(bc, br) == 0) { continue; }
						if (!pieces[static_cast<std::size_t>(at(bc, br) - 1)].object->isPlace) { return false; }
					}
				}
				return true;
			}

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
					if (move.under >= 0) { pieces[static_cast<std::size_t>(move.under)].alive = false; }
					Piece made;
					made.object = move.kind ? move.kind : kinds[side];
					made.column = move.cells.front().first;
					made.row = move.cells.front().second;
					made.look = move.look;
					made.side = side;
					pieces.push_back(made);
					at(made.column, made.row) = static_cast<int>(pieces.size());
					// What it walls in beside it is the side's.
					if (!closeClass[side].empty())
					{
						const int around[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
						for (const auto& way : around)
						{
							const int c = made.column + way[0];
							const int r = made.row + way[1];
							if (!onBoard(c, r) || at(c, r) == 0) { continue; }
							const std::size_t box = static_cast<std::size_t>(at(c, r) - 1);
							if (pieces[box].owner >= 0 || !pieces[box].alive || !pieces[box].object->closedBy[side] || !walled(c, r)) { continue; }
							pieces[box].owner = side;
							undo.closed.push_back(box);
						}
					}
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
					for (const std::size_t box : undo.closed) { pieces[box].owner = -1; }
					at(move.cells.front().first, move.cells.front().second) = 0;
					pieces.pop_back();
					if (move.under >= 0)
					{
						pieces[static_cast<std::size_t>(move.under)].alive = true;
						at(move.cells.front().first, move.cells.front().second) = move.under + 1;
					}
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
			const std::atomic<bool>& stop;

			// Boards already worked out, by their key, whose turn and how
			// deep: the same lines drawn in another order are not looked at
			// again. What was found may be only a bound (the search of it
			// stopped early at alpha or beta).
			struct Known
			{
				float value{};
				enum class Bound { exact, atMost, atLeast } bound{ Bound::exact };
			};
			std::unordered_map<std::uint64_t, Known> known;

			float worthOf(const Piece& piece) const
			{
				const std::string& look = piece.look < piece.object->lookNames.size() ? piece.object->lookNames[piece.look] : std::string();
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
				// What each side has claimed, nothing else.
				if (board.claims())
				{
					for (const Piece& piece : board.pieces)
					{
						if (!piece.alive) { continue; }
						if (piece.owner == 0 || piece.object->ownedBy[0]) { total += 1.0f; }
						else if (piece.owner == 1 || piece.object->ownedBy[1]) { total -= 1.0f; }
					}
					return total;
				}
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
				// Its turn is over or the game was put back: nothing is wanted.
				if (stop.load(std::memory_order_relaxed)) { return 0.0f; }
				const float now = side == 0 ? score() : -score();
				// The last of the moves looked at: only whether the game is
				// over is wanted, not every move there is.
				const bool any = depth > 0 || board.canMove(side);
				const std::vector<Move> moves = depth > 0 ? board.moves(side) : std::vector<Move>{};
				if (depth <= 0 && any) { return now; }
				if (moves.empty() && board.puts(side))
				{
					// No put: it passes; neither can: the game is over, won
					// by the side with more (sooner is better), or, when
					// pieces put turn nothing, drawn (a full board).
					if (!board.canMove(1 - side))
					{
						if (!board.flanks(side) && !board.claims()) { return 0.0f; }
						const float sooner = static_cast<float>(depth);
						return now > 0.0f ? decided + sooner + now : now < 0.0f ? -decided - sooner + now : 0.0f;
					}
					if (depth <= 0) { return now; }
					return -value(1 - side, depth - 1, -beta, -alpha);
				}
				if (moves.empty()) { return -(decided + static_cast<float>(depth)); }
				if (depth <= 0) { return now; }

				const std::uint64_t key = board.key() ^ Board::mix(static_cast<std::uint64_t>(side) * std::uint64_t{ 1000003 } + static_cast<std::uint64_t>(depth));
				if (const auto found = known.find(key); found != known.end())
				{
					const Known& seen = found->second;
					if (seen.bound == Known::Bound::exact) { return seen.value; }
					if (seen.bound == Known::Bound::atLeast) { alpha = std::max(alpha, seen.value); }
					else { beta = std::min(beta, seen.value); }
					if (alpha >= beta) { return seen.value; }
				}
				const float from = alpha;

				// Moves that close something first: they are most often the
				// best, and a good move found early cuts the rest short.
				std::vector<const Move*> order;
				order.reserve(moves.size());
				bool quietSeen = false;
				for (const Move& move : moves)
				{
					// One quiet move stands for them all.
					if (board.claims() && board.quiet(move))
					{
						if (quietSeen) { continue; }
						quietSeen = true;
					}
					order.push_back(&move);
				}
				// (Not worth it just above the last step, where every move is
				// only scored.)
				if (board.claims() && depth >= 2)
				{
					std::stable_partition(order.begin(), order.end(), [&](const Move* move) { return board.closes(*move); });
				}

				for (const Move* move : order)
				{
					const Board::Undo undo = board.make(*move);
					// Something closed: the same side goes again.
					const float got = board.madeLine(*move) ? decided + static_cast<float>(depth)
						: !undo.closed.empty() ? value(side, depth - 1, alpha, beta)
						: -value(1 - side, depth - 1, -beta, -alpha);
					board.unmake(*move, undo);
					alpha = std::max(alpha, got);
					if (alpha >= beta) { break; }
				}
				if (!stop.load(std::memory_order_relaxed))
				{
					known[key] = { alpha, alpha <= from ? Known::Bound::atMost : alpha >= beta ? Known::Bound::atLeast : Known::Bound::exact };
				}
				return alpha;
			}
		};
	}

	// A search: the table it is made on, what it is asked, and, once it is
	// done, the best moves. `stop` tells it to give up (its turn is over).
	struct ThinkJob
	{
		Board board;
		CmdThink think;
		int pickerRow{};
		std::atomic<bool> stop{ false };
		std::atomic<bool> done{ false };
		std::vector<std::vector<std::pair<int, int>>> best;
	};

	// The table of the board as it is now, and the search to be made on it.
	std::shared_ptr<ThinkJob> Game::startThinking(const CmdThink& think, const Object& picker)
	{
		auto job = std::make_shared<ThinkJob>();
		job->think = think;
		job->pickerRow = whole(picker.cellRow);
		Board& board = job->board;
		// The board: the map of cells its pieces are on (the first piece of
		// its class with moves), and everything on that map.
		const Object* first = nullptr;
		for (const Object& object : objects)
		{
			if (object.objClass == think.objClass && !object.moveSets.empty() && object.onCells && isShown(object)) { first = &object; break; }
		}
		// None on the board yet (pieces put): the one a put copies.
		if (!first) { first = putTemplate(think.objClass); }
		if (!first || first->moveSets.empty())
		{
			job->done = true;
			return job;
		}

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
		board.kinds[0] = board.kindOf(putTemplate(think.objClass));
		board.kinds[1] = other.empty() ? nullptr : board.kindOf(putTemplate(other));
		for (int side = 0; side < 2; ++side)
		{
			if (side == 1 && other.empty()) { break; }
			for (const Object* each : putTemplates(side == 0 ? think.objClass : other))
			{
				const Kind* kind = board.kindOf(each);
				board.allKinds[side].push_back(kind);
				if (const Object::MoveSet* set = Board::setOf(*kind, kind->look))
				{
					board.places.insert(board.places.end(), set->on.begin(), set->on.end());
					// What a side's put closes, and the class of what claims it.
					if (!set->closeClass.empty() && board.closeClass[side].empty())
					{
						board.closeClass[side] = set->closeClass;
						const std::string owned = fullName(set->closeObject);
						for (const Object& object : objects)
						{
							if (object.named(owned)) { board.ownClass[side] = object.objClass; break; }
						}
					}
				}
			}
		}
		// The map's checks, as its pieces written carry them (a copy may not).
		const Object* written = putTemplate(think.objClass);
		if (!written) { written = first; }
		for (const Object::CellCheck& check : written->cellChecks)
		{
			if (check.kind == "line") { board.lines.push_back({ check.objClass, check.count, check.straight }); }
		}
		if (!board.kinds[1]) { board.kinds[0] = nullptr; }
		// What is on the board: in play, and put this frame (a piece put
		// joins the game at the end of the frame its put was made in).
		std::vector<const Object*> there;
		for (const Object& object : objects)
		{
			if (isShown(object)) { there.push_back(&object); }
		}
		for (const Object& object : spawned)
		{
			if (object.exists) { there.push_back(&object); }
		}
		for (const Object* each : there)
		{
			const Object& object = *each;
			if (!object.onCells || object.mapName != first->mapName || &object == &picker) { continue; }
			const int column = whole(object.cellColumn);
			const int row = whole(object.cellRow);
			if (!board.onBoard(column, row)) { continue; }
			Piece piece;
			piece.object = board.kindOf(&object);
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

		board.prepare();

		// Thought about beside the game, or at once.
		if (thinkAside)
		{
			std::thread([job] { runThinking(*job); }).detach();
		}
		else
		{
			runThinking(*job);
		}
		return job;
	}

	// Every move it can make now, each played out: the best, all of them
	// that are as good.
	void Game::runThinking(ThinkJob& job)
	{
		try
		{
			Board& board = job.board;
			const CmdThink& think = job.think;
			Search search{ board, think, job.stop, {} };
			const std::vector<Move> moves = board.moves(0);
			std::vector<const Move*> best;
			const float infinity = std::numeric_limits<float>::infinity();
			float bestValue = -infinity;
			// The quiet moves (touching nothing yet): the first is looked
			// at, and the others are as good as it.
			std::vector<const Move*> quiet;
			if (board.claims())
			{
				for (const Move& move : moves)
				{
					if (board.quiet(move)) { quiet.push_back(&move); }
				}
			}
			for (const Move& move : moves)
			{
				if (!quiet.empty() && &move != quiet.front() && std::find(quiet.begin(), quiet.end(), &move) != quiet.end()) { continue; }
				const Board::Undo undo = board.make(move);
				// Only whether it is as good as the best so far matters
				// (the many moves that come to the same are all kept), so a
				// move is looked at only until it is plainly worse. Values
				// differ by whole boxes, pieces or moves.
				const float low = bestValue - 0.5f;
				const float got = board.madeLine(move) ? decided + static_cast<float>(think.depth)
					: !undo.closed.empty() ? search.value(0, think.depth - 1, low, infinity)
					: -search.value(1, think.depth - 1, -infinity, -low);
				board.unmake(move, undo);
				if (got > bestValue) { bestValue = got; best.clear(); }
				if (got == bestValue)
				{
					if (!quiet.empty() && &move == quiet.front()) { best.insert(best.end(), quiet.begin(), quiet.end()); }
					else { best.push_back(&move); }
				}
			}
			for (const Move* move : best)
			{
				std::vector<std::pair<int, int>> cells = move->cells;
				// A piece that falls is dropped from the picker's own row.
				if (move->put)
				{
					const Object::MoveSet* set = Board::setOf(*board.kinds[0], board.kinds[0]->look);
					if (set && set->fall) { cells.front().second = job.pickerRow; }
				}
				job.best.push_back(std::move(cells));
			}
		}
		catch (...)
		{
			job.best.clear();
		}
		job.done = true;
	}

	void Game::stopThinking()
	{
		if (thinking.job) { thinking.job->stop = true; }
		thinking = {};
	}

	void Game::thinkInBackground(bool aside) noexcept
	{
		thinkAside = aside;
	}

	std::optional<KeyCode> Game::think(const CmdThink& think)
	{
		Object* picker = nullptr;
		const std::string name = fullName(think.picker);
		for (Object& object : objects)
		{
			if (object.exists && object.named(name)) { picker = &object; break; }
		}
		if (picker && picker->hand.is) { return thinkCards(think, *picker); }
		if (!picker || !picker->onCells || think.objClass.empty()) { return std::nullopt; }

		// Its move is thought about first (the frame its turn came is drawn
		// before, the sounds of the move before it heard); then made.
		if (!thinking.planned)
		{
			if (!thinking.job) { thinking.job = startThinking(think, *picker); }
			if (!thinking.job->done) { return std::nullopt; }
			const std::vector<std::vector<std::pair<int, int>>> best = std::move(thinking.job->best);
			thinking = {};
			thinking.planned = true;
			if (!best.empty())
			{
				const std::size_t pick = std::min(best.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(best.size()))));
				thinking.cells = best[pick];
			}
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
