// routes.cpp
// XML Game Engine
// author: beefviper
// date: Nov 22, 2026
//
// Pieces that go along a <route> by the worth of what a holder holds (a
// race game's dice), and cells of several tiles that stack their pieces.

#include "game/cells.h"
#include "game/game.h"
#include "game/words.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	namespace
	{
		int whole(float number) noexcept { return static_cast<int>(std::lround(number)); }
		long long cellKey(int column, int row) noexcept { return static_cast<long long>(column) * 100000 + row; }

		// Where things stand along one class's route: its own pieces and
		// those it hits at each cell of it, and whether something else
		// stands there (which nothing passes into).
		struct Board
		{
			std::vector<int> mine;
			std::vector<int> theirs;
			std::vector<bool> other;
		};

		// A move from `from` by `steps`: where it ends (-1: it may not), and
		// whether it hits.
		struct Landing
		{
			int to{ -1 };
			bool hits{ false };
		};

		Landing landing(const Object::Route& route, const Board& board, int from, int steps)
		{
			const int last = static_cast<int>(route.cells.size()) - 1;
			if (board.mine[static_cast<std::size_t>(from)] == 0) { return {}; }
			// One waiting at the start comes in before any other moves.
			if (route.start && from != 0 && board.mine[0] > 0) { return {}; }
			int to = from + steps;
			if (route.end && to >= last)
			{
				if (from == last) { return {}; }
				// Every piece within `home` of the end first; past it only
				// from the one furthest back.
				const int home = route.home > 0 ? last - route.home : 0;
				for (int at = 0; at < home; ++at)
				{
					if (board.mine[static_cast<std::size_t>(at)] > 0) { return {}; }
				}
				if (to > last)
				{
					for (int at = 0; at < from; ++at)
					{
						if (board.mine[static_cast<std::size_t>(at)] > 0) { return {}; }
					}
				}
				return { last, false };
			}
			if (to > last) { return {}; }
			const auto cell = static_cast<std::size_t>(to);
			if (board.other[cell] || board.theirs[cell] > 1) { return {}; }
			return { to, board.theirs[cell] == 1 };
		}

		Board moved(Board board, int from, const Landing& land)
		{
			--board.mine[static_cast<std::size_t>(from)];
			++board.mine[static_cast<std::size_t>(land.to)];
			if (land.hits) { board.theirs[static_cast<std::size_t>(land.to)] = 0; }
			return board;
		}

		std::vector<int> without(std::vector<int> steps, int used)
		{
			steps.erase(std::find(steps.begin(), steps.end(), used));
			return steps;
		}

		// How many of `steps` can be used, one after another, from here;
		// what is worked out is kept in `known`, by the board and the steps.
		int mostUsed(const Object::Route& route, const Board& board, const std::vector<int>& steps, std::unordered_map<std::string, int>& known)
		{
			if (steps.empty()) { return 0; }
			std::string key;
			for (std::size_t i = 0; i < board.mine.size(); ++i)
			{
				key += static_cast<char>('a' + board.mine[i]);
				key += static_cast<char>('a' + board.theirs[i] + (board.other[i] ? 8 : 0));
			}
			for (const int each : steps) { key += static_cast<char>('0' + each); }
			if (const auto seen = known.find(key); seen != known.end()) { return seen->second; }
			if (known.size() > 100000) { known.clear(); }
			int most = 0;
			std::vector<int> tried;
			for (const int each : steps)
			{
				if (std::find(tried.begin(), tried.end(), each) != tried.end()) { continue; }
				tried.push_back(each);
				for (int from = 0; from < static_cast<int>(board.mine.size()); ++from)
				{
					const Landing land = landing(route, board, from, each);
					if (land.to < 0) { continue; }
					most = std::max(most, 1 + mostUsed(route, moved(board, from, land), without(steps, each), known));
					if (most == static_cast<int>(steps.size())) { break; }
				}
				if (most == static_cast<int>(steps.size())) { break; }
			}
			known.emplace(std::move(key), most);
			return most;
		}
	}

	std::vector<CellMaps::PieceMoveTo> CellMaps::routeMoves(Object& piece)
	{
		std::vector<PieceMoveTo> found;
		const Object::MoveSet* set = moveSetOf(piece);
		if (!set || set->route.cells.empty()) { return found; }
		const Object::Route& route = set->route;
		std::unordered_map<long long, int> indexOf;
		for (std::size_t i = 0; i < route.cells.size(); ++i) { indexOf.emplace(cellKey(route.cells[i].column, route.cells[i].row), static_cast<int>(i)); }

		const std::size_t length = route.cells.size();
		Board board{ std::vector<int>(length), std::vector<int>(length), std::vector<bool>(length) };
		const Name map = game.mapOf(piece).name;
		int at = -1;
		const auto count = [&](Object& object)
		{
			if (!object.exists || !object.onCells || game.mapOf(object).name != map || game.pieceMoves(object).sets.empty()) { return; }
			const auto cell = indexOf.find(cellKey(whole(game.place(object).column), whole(game.place(object).row)));
			if (cell == indexOf.end()) { return; }
			const auto index = static_cast<std::size_t>(cell->second);
			if (&object == &piece) { at = cell->second; }
			if (object.objClass == piece.objClass) { ++board.mine[index]; }
			else if (std::find(route.hits.begin(), route.hits.end(), object.objClass) != route.hits.end()) { ++board.theirs[index]; }
			else { board.other[index] = true; }
		};
		for (Object& object : game.getCurrentObjects())
		{
			if (game.isShown(object)) { count(object); }
		}
		for (Object& object : game.spawning()) { count(object); }
		if (Object::Moves* moves = game.parts().moves.find(piece.id)) { moves->along = static_cast<float>(at); }
		if (at < 0) { return found; }

		std::vector<int> steps;
		for (const Object* item : game.holders().heldIn(route.steps)) { steps.push_back(whole(item->variable.valueOr(words::worth, 0.0f))); }
		std::sort(steps.begin(), steps.end());

		// A turn uses as many steps as it can; when only one fits, the
		// biggest that does.
		const int most = mostUsed(route, board, steps, routeMemo);
		if (most == 0) { return found; }
		int biggest = 0;
		if (most == 1)
		{
			for (const int each : steps)
			{
				for (int from = 0; from < static_cast<int>(length); ++from)
				{
					if (landing(route, board, from, each).to >= 0) { biggest = std::max(biggest, each); }
				}
			}
		}
		std::vector<int> tried;
		for (const int each : steps)
		{
			if (std::find(tried.begin(), tried.end(), each) != tried.end() || (most == 1 && each != biggest)) { continue; }
			tried.push_back(each);
			const Landing land = landing(route, board, at, each);
			if (land.to < 0 || 1 + mostUsed(route, moved(board, at, land), without(steps, each), routeMemo) < most) { continue; }
			const Object::Route::Cell& cell = route.cells[static_cast<std::size_t>(land.to)];
			Object* hit = nullptr;
			if (land.hits)
			{
				for (Object& object : game.getCurrentObjects())
				{
					if (object.exists && object.onCells && game.isShown(object) && game.mapOf(object).name == map && &object != &piece
						&& whole(game.place(object).column) == cell.column && whole(game.place(object).row) == cell.row
						&& std::find(route.hits.begin(), route.hits.end(), object.objClass) != route.hits.end())
					{
						hit = &object;
						break;
					}
				}
			}
			found.push_back({ cell.column, cell.row, hit, each });
		}
		return found;
	}

	void CellMaps::placeAlong(Object& piece, const PieceMoveTo& move)
	{
		const Object::MoveSet* set = moveSetOf(piece);
		if (move.taken)
		{
			// Hit: to its own route's start, or out of play with none.
			const Object::MoveSet* theirs = moveSetOf(*move.taken);
			if (theirs && theirs->route.start) { putInCell(*move.taken, theirs->route.cells.front().column, theirs->route.cells.front().row); }
			else { game.despawn(*move.taken); }
		}
		putInCell(piece, move.column, move.row);
		for (Object* item : game.holders().heldIn(set->route.steps))
		{
			if (whole(item->variable.valueOr(words::worth, 0.0f)) == move.steps)
			{
				game.holders().sendItem(game.itemOf(*item).number, set->route.steps, set->route.into);
				break;
			}
		}
	}

	void CellMaps::layStacks()
	{
		std::map<std::tuple<Name, int, int>, std::vector<Object*>> stacks;
		for (Object& object : game.getCurrentObjects())
		{
			if (!object.exists || !object.onCells || game.mapOf(object).stack < 2 || !game.isShown(object)) { continue; }
			stacks[{ game.mapOf(object).name, whole(game.place(object).column), whole(game.place(object).row) }].push_back(&object);
		}
		for (auto& [where, pieces] : stacks)
		{
			const Object::MapPlace& map = game.mapOf(*pieces.front());
			const int column = std::get<1>(where);
			const int row = std::get<2>(where);
			const bool across = map.stackAcross;
			// From the end of the cell at the board's nearer edge.
			const bool fromFar = across ? column > (map.columns + 1) / 2 : row > (map.rows + 1) / 2;
			const Vector2f corner = map.origin + Vector2f{ static_cast<float>(column - 1) * map.tile.x, static_cast<float>(row - 1) * map.tile.y };
			const float length = across ? map.tile.x : map.tile.y;
			const float slot = length / static_cast<float>(map.stack);
			const auto along = [&](const Object& object) { return across ? object.position.x - corner.x : object.position.y - corner.y; };
			const auto fromBase = [&](const Object& object)
			{
				const float size = across ? object.size.x : object.size.y;
				return fromFar ? length - (along(object) + size) : along(object);
			};
			std::stable_sort(pieces.begin(), pieces.end(), [&](const Object* a, const Object* b) { return fromBase(*a) < fromBase(*b); });
			const std::size_t count = pieces.size();
			const float spacing = count <= static_cast<std::size_t>(map.stack) ? slot : (length - slot) / static_cast<float>(count - 1);
			for (std::size_t i = 0; i < count; ++i)
			{
				Object& piece = *pieces[i];
				const float size = across ? piece.size.x : piece.size.y;
				const float start = static_cast<float>(i) * spacing + (slot - size) / 2.0f;
				const float inCell = std::clamp(fromFar ? length - start - size : start, 0.0f, std::max(0.0f, length - size));
				const float side = ((across ? map.tile.y : map.tile.x) - (across ? piece.size.y : piece.size.x)) / 2.0f;
				piece.position = corner + (across ? Vector2f{ inCell, side } : Vector2f{ side, inCell });
			}
		}
	}
}
