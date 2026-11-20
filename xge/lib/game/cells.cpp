// cells.cpp
// XML Game Engine
// author: beefviper
// date: Oct 30, 2026

#include "game/game.h"
#include "play/executor.h"
#include "play/geometry.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace xge
{
	namespace
	{
		// A cell's column and row as whole numbers.
		int whole(float number) noexcept { return static_cast<int>(std::lround(number)); }

		// The edge of a mover that a step that way meets things with.
		Edge edgeOfStep(int across, int down) noexcept
		{
			if (down > 0) { return Edge::Bottom; }
			if (down < 0) { return Edge::Top; }
			return across < 0 ? Edge::Left : Edge::Right;
		}

		// The standard kicks of a quarter turn (the Super Rotation System's):
		// where else to try, across and up, after where it is, by the square
		// the shape turns in and the turn (from quarter `from`, clockwise
		// `clockwise`). A square of 3 takes the first table, of 4 the second;
		// a shape of two cells (a capsule) the third, which turns it about
		// its bottom left cell, lying along the same row and standing up from
		// it; any other turns where it is or not at all.
		using Kicks = std::array<std::pair<int, int>, 5>;
		const Kicks& kicksFor(int size, int cells, int from, bool clockwise)
		{
			// By the turn: 0 to 1, 1 to 0, 1 to 2, 2 to 1, 2 to 3, 3 to 2, 3 to 0, 0 to 3.
			static const Kicks three[8] = {
				Kicks{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { 1, -1 }, { 0, 2 }, { 1, 2 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { -1, 1 }, { 0, -2 }, { -1, -2 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, 2 }, { -1, 2 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, -2 }, { 1, -2 } } },
			};
			static const Kicks four[8] = {
				Kicks{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, -1 }, { 1, 2 } } },
				Kicks{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, 1 }, { -1, -2 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, 2 }, { 2, -1 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, -2 }, { -2, 1 } } },
				Kicks{ { { 0, 0 }, { 2, 0 }, { -1, 0 }, { 2, 1 }, { -1, -2 } } },
				Kicks{ { { 0, 0 }, { -2, 0 }, { 1, 0 }, { -2, -1 }, { 1, 2 } } },
				Kicks{ { { 0, 0 }, { 1, 0 }, { -2, 0 }, { 1, -2 }, { -2, 1 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { 2, 0 }, { -1, 2 }, { 2, -1 } } },
			};
			// Lying, it stands up from its left cell, else down from it (the
			// top row); standing, it lies down along the row of its bottom
			// cell, else one to the left (a wall).
			static const Kicks two[8] = {
				Kicks{ { { -1, 1 }, { -1, 0 }, { -1, 1 }, { -1, 1 }, { -1, 1 } } },
				Kicks{ { { 1, -1 }, { 0, -1 }, { 1, -1 }, { 1, -1 }, { 1, -1 } } },
				Kicks{ { { 1, 0 }, { 0, 0 }, { 1, 0 }, { 1, 0 }, { 1, 0 } } },
				Kicks{ { { -1, 0 }, { -1, -1 }, { -1, 0 }, { -1, 0 }, { -1, 0 } } },
				Kicks{ { { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } },
				Kicks{ { { 0, 0 }, { -1, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } },
				Kicks{ { { 0, -1 }, { -1, -1 }, { 0, -1 }, { 0, -1 }, { 0, -1 } } },
				Kicks{ { { 0, 1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 } } },
			};
			static const Kicks none{ { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };

			const int turn = clockwise ? from * 2 : (from + 3) % 4 * 2 + 1;
			if (size == 2 && cells == 2) { return two[turn]; }
			if (size == 3) { return three[turn]; }
			if (size == 4) { return four[turn]; }
			return none;
		}
	}

	Object* Game::findSpawning(Name name) noexcept
	{
		for (Object& object : objects)
		{
			if (object.name == name) { return &object; }
		}
		for (Object& object : spawned)
		{
			if (object.name == name) { return &object; }
		}
		return nullptr;
	}

	Object* Game::cellHolder(Name map, int column, int row, const std::vector<Object*>& mover)
	{
		const auto cellKey = [](int c, int r) { return static_cast<long long>(c) * 100000 + r; };
		if (heldCells.on && mover.empty())
		{
			auto found = heldCells.maps.find(map);
			if (found == heldCells.maps.end())
			{
				// The first piece in each cell, as the scan below finds it.
				std::unordered_map<long long, Object*> cells;
				for (Object& object : objects)
				{
					if (object.exists && object.onCells && !isShape(object) && mapOf(object).name == map && isShown(object))
					{
						cells.emplace(cellKey(whole(place(object).column), whole(place(object).row)), &object);
					}
				}
				for (Object& object : spawned)
				{
					if (object.exists && object.onCells && !isShape(object) && mapOf(object).name == map)
					{
						cells.emplace(cellKey(whole(place(object).column), whole(place(object).row)), &object);
					}
				}
				found = heldCells.maps.emplace(map, std::move(cells)).first;
			}
			const auto cell = found->second.find(cellKey(column, row));
			return cell == found->second.end() ? nullptr : cell->second;
		}
		const auto holds = [&](Object& object)
		{
			return object.exists && object.onCells && !isShape(object) && mapOf(object).name == map
				&& whole(place(object).column) == column && whole(place(object).row) == row
				&& std::find(mover.begin(), mover.end(), &object) == mover.end();
		};
		for (Object& object : objects)
		{
			if (holds(object) && isShown(object)) { return &object; }
		}
		// Spawned this frame: in play once the frame is over.
		for (Object& object : spawned)
		{
			if (holds(object)) { return &object; }
		}
		return nullptr;
	}

	std::vector<Object*> Game::cellsOf(Object& mover)
	{
		if (!isShape(mover)) { return { &mover }; }
		std::vector<Object*> cells;
		for (const Name part : cellPart(mover).shapeParts)
		{
			if (Object* cell = findSpawning(part)) { cells.push_back(cell); }
		}
		return cells;
	}

	void Game::putInCell(Object& object, int column, int row)
	{
		placeFor(object).column = static_cast<float>(column);
		placeFor(object).row = static_cast<float>(row);
		mapFor(object).column = column - 1;
		mapFor(object).row = row - 1;
		const Vector2f size = sizeOf(object);
		object.position = mapOf(object).origin + Vector2f{ (static_cast<float>(column) - 0.5f) * mapOf(object).tile.x - size.x / 2.0f,
			(static_cast<float>(row) - 0.5f) * mapOf(object).tile.y - size.y / 2.0f };
	}

	bool Game::moveToMap(Object& object, Name map)
	{
		if (mapOf(object).name == map) { return true; }
		const auto onIt = [&](const Object& each) { return each.onCells && mapOf(each).name == map; };
		const Object* like = nullptr;
		for (const Object& each : objects)
		{
			if (onIt(each)) { like = &each; break; }
		}
		if (!like)
		{
			for (const Object& each : spawned)
			{
				if (onIt(each)) { like = &each; break; }
			}
		}
		if (!like) { return false; }
		// Copied out first: `like` may sit in a list that grows.
		const Vector2f origin = mapOf(*like).origin;
		const Vector2f tile = mapOf(*like).tile;
		const int columns = mapOf(*like).columns;
		const int rows = mapOf(*like).rows;
		std::vector<Object::CellCheck> checks = cellPart(*like).checks;
		mapFor(object).name = map;
		mapFor(object).origin = origin;
		mapFor(object).tile = tile;
		mapFor(object).columns = columns;
		mapFor(object).rows = rows;
		cellPartFor(object).checks = std::move(checks);
		cellPartFor(object).held = false;
		return true;
	}

	bool Game::putOnCells(Object& made, int column, int row, Name map)
	{
		const auto fits = [&](int c, int r)
		{
			return c >= 1 && r >= 1 && c <= mapOf(made).columns && r <= mapOf(made).rows && !cellHolder(mapOf(made).name, c, r, { &made });
		};

		const auto takeBack = [&]()
		{
			writtenCopies.erase(made.name);
			spawned.pop_back();
			return false;
		};

		if (!map.empty() && !moveToMap(made, map)) { return takeBack(); }

		if (!isShape(made))
		{
			if (!fits(column, row)) { return takeBack(); }
			putInCell(made, column, row);
			return true;
		}

		for (const Object::ShapeCell& cell : cellPart(made).shape)
		{
			if (!fits(column + cell.column, row + cell.row)) { return takeBack(); }
		}
		putInCell(made, column, row);
		for (const Object::ShapeCell& cell : cellPart(made).shape)
		{
			Object* part = spawn(cell.piece, true);
			if (!part) { continue; }
			moveToMap(*part, mapOf(made).name);
			cellPartFor(*part).shapeOwner = made.name;
			putInCell(*part, column + cell.column, row + cell.row);
			cellPartFor(made).shapeParts.push_back(part->name);
		}
		return true;
	}

	bool Game::sendCells(Name target, Name from, Name into, std::optional<int> column, std::optional<int> row)
	{
		// The first of that name on `from` that moves as one: a shape, or a
		// piece that is no shape's cell.
		Object* piece = nullptr;
		for (Object& object : objects)
		{
			if (object.exists && object.onCells && mapOf(object).name == from && cellPart(object).shapeOwner.empty()
				&& (object.name == target || object.inGroup(target)) && isShown(object))
			{
				piece = &object;
				break;
			}
		}
		if (!piece) { return false; }

		const std::vector<Object*> cells = cellsOf(*piece);
		const int left = column.value_or(whole(place(*piece).column));
		const int top = row.value_or(whole(place(*piece).row));
		const int fromLeft = whole(place(*piece).column);
		const int fromTop = whole(place(*piece).row);

		// Where each cell goes, kept as it lies from the shape's top left.
		Object probe = *piece;
		if (!moveToMap(probe, into)) { return false; }
		for (Object* cell : cells)
		{
			const int c = left + whole(place(*cell).column) - fromLeft;
			const int r = top + whole(place(*cell).row) - fromTop;
			if (c < 1 || r < 1 || c > mapOf(probe).columns || r > mapOf(probe).rows || cellHolder(into, c, r, cells)) { return false; }
		}

		for (Object* cell : cells)
		{
			const int c = left + whole(place(*cell).column) - fromLeft;
			const int r = top + whole(place(*cell).row) - fromTop;
			moveToMap(*cell, into);
			putInCell(*cell, c, r);
		}
		if (isShape(*piece))
		{
			moveToMap(*piece, into);
			putInCell(*piece, left, top);
		}
		return true;
	}

	bool Game::stepCells(Object& mover, int across, int down)
	{
		if (!mover.onCells || (across == 0 && down == 0)) { return false; }
		const std::vector<Object*> cells = cellsOf(mover);
		const Edge edge = edgeOfStep(across, down);

		// A chain's tail leaves its cell as the head comes in, unless it is
		// growing there: it is no blocker.
		std::vector<Object*> passing = cells;
		const std::vector<Object*> links = chainOf(mover);
		if (!links.empty() && place(mover).chainGrow == 0) { passing.push_back(links.back()); }

		// The first thing in the way, or the map's edge.
		const auto inTheWay = [&](bool& offMap) -> Object*
		{
			offMap = false;
			for (Object* cell : cells)
			{
				const int column = whole(place(*cell).column) + across;
				const int row = whole(place(*cell).row) + down;
				if (column < 1 || row < 1 || column > mapOf(mover).columns || row > mapOf(mover).rows)
				{
					offMap = true;
					return nullptr;
				}
				if (Object* holder = cellHolder(mapOf(mover).name, column, row, passing)) { return holder; }
			}
			return nullptr;
		};
		bool offMap = false;
		Object* blocker = inTheWay(offMap);

		if (offMap)
		{
			atEdge(mover, edge);
			return false;
		}
		if (blocker)
		{
			// While the rules run, the cells it steps into are not free for
			// a <spawn><free /> (Snake's next food).
			const std::size_t steppingBefore = stepping.size();
			for (Object* cell : cells) { stepping.push_back({ mapOf(mover).name, whole(place(*cell).column) + across, whole(place(*cell).row) + down }); }
			Touch touch = touchAcross(edge);
			touch.point = middleOf(*blocker);
			const int beforeColumn = whole(place(mover).column);
			const int beforeRow = whole(place(mover).row);
			const Reaction moverAsked = reactTo(mover, *blocker, touch);
			Touch back = touch;
			back.edge = opposite(edge);
			back.normal = outOf(edge);
			const Reaction blockerAsked = reactTo(*blocker, mover, back);
			// A shoved piece's own step is asked inside these rules.
			stepping.resize(steppingBefore);

			// A touch that took what was in the way away (food eaten) lets
			// the step go on; one that moved the mover (a centipede turning)
			// has made its own step instead.
			// A <through /> in the rules makes it anyway.
			const bool moved = whole(place(mover).column) != beforeColumn || whole(place(mover).row) != beforeRow;
			const bool through = moverAsked.through || blockerAsked.through;
			if (moved || !mover.exists) { return false; }
			if (!through && (inTheWay(offMap) || offMap)) { return false; }
		}

		const int fromColumn = whole(place(mover).column);
		const int fromRow = whole(place(mover).row);
		for (Object* cell : cells) { putInCell(*cell, whole(place(*cell).column) + across, whole(place(*cell).row) + down); }
		if (isShape(mover)) { putInCell(mover, whole(place(mover).column) + across, whole(place(mover).row) + down); }
		if (!cellPart(mover).chain.empty()) { followChain(mover, links, fromColumn, fromRow); }
		return true;
	}

	bool Game::isLink(const Object& piece, Name chain)
	{
		if (piece.named(chain)) { return true; }
		if (!piece.isCopy()) { return false; }
		const Object* of = findSpawning(piece.copyOf);
		return piece.copyOf == chain || (of && of != &piece && of->named(chain));
	}

	std::vector<Object*> Game::chainOf(Object& head)
	{
		if (cellPart(head).chain.empty()) { return {}; }

		// As kept: a link no longer there is dropped.
		std::vector<Object*> links;
		if (place(head).chainKnown)
		{
			std::vector<Name> kept;
			for (const Name name : place(head).chainLinks)
			{
				Object* link = findSpawning(name);
				if (!link || !link->exists) { continue; }
				links.push_back(link);
				kept.push_back(name);
			}
			placeFor(head).chainLinks = std::move(kept);
			return links;
		}

		// Found from where they lie: from the head, the next one beside
		// the last, until there is none.
		placeFor(head).chainLinks.clear();
		int column = whole(place(head).column);
		int row = whole(place(head).row);
		for (;;)
		{
			Object* next = nullptr;
			const auto look = [&](auto& list)
			{
				for (Object& object : list)
				{
					if (next) { return; }
					if (!object.exists || !object.onCells || mapOf(object).name != mapOf(head).name || &object == &head || !isLink(object, cellPart(head).chain)) { continue; }
					if (std::find(links.begin(), links.end(), &object) != links.end()) { continue; }
					if (std::abs(whole(place(object).column) - column) + std::abs(whole(place(object).row) - row) == 1) { next = &object; }
				}
			};
			look(objects);
			look(spawned);
			if (!next) { break; }
			links.push_back(next);
			placeFor(head).chainLinks.push_back(next->name);
			column = whole(place(*next).column);
			row = whole(place(*next).row);
		}
		placeFor(head).chainKnown = true;
		return links;
	}

	void Game::followChain(Object& head, const std::vector<Object*>& links, int column, int row)
	{
		// Each link into the cell of the one ahead of it.
		for (Object* link : links)
		{
			const int nowColumn = whole(place(*link).column);
			const int nowRow = whole(place(*link).row);
			putInCell(*link, column, row);
			column = nowColumn;
			row = nowRow;
		}
		if (place(head).chainGrow <= 0) { return; }

		// Growing: a new link where the tail was.
		--placeFor(head).chainGrow;
		Object* grown = spawn(cellPart(head).chain, true);
		if (!grown) { return; }
		moveToMap(*grown, mapOf(head).name);
		putInCell(*grown, column, row);
		placeFor(head).chainLinks.push_back(grown->name);
	}

	void Game::splitChain(Object& piece)
	{
		if (cellPart(piece).chain.empty()) { return; }

		// The head it is, or the head it follows.
		Object* head = nullptr;
		std::size_t at = 0;
		const auto lead = [&](auto& list)
		{
			for (Object& object : list)
			{
				if (head || !object.exists || cellPart(object).chain.empty()) { continue; }
				if (&object == &piece && (place(object).chainKnown || place(object).going)) { head = &object; return; }
				const auto found = std::find(place(object).chainLinks.begin(), place(object).chainLinks.end(), piece.name);
				if (found != place(object).chainLinks.end())
				{
					head = &object;
					at = static_cast<std::size_t>(found - place(object).chainLinks.begin()) + 1;
					return;
				}
			}
		};
		lead(objects);
		lead(spawned);
		if (!head) { return; }
		chainOf(*head);

		// at: where the piece is, 0 for the head itself.
		const std::vector<Name> links = place(*head).chainLinks;
		std::vector<Name> behind(links.begin() + static_cast<std::ptrdiff_t>(std::min(at, links.size())), links.end());
		placeFor(*head).chainLinks.resize(at == 0 ? 0 : at - 1);
		placeFor(*head).chainKnown = true;

		Object* next = behind.empty() ? nullptr : findSpawning(behind.front());
		if (next)
		{
			placeFor(*next).chainLinks.assign(behind.begin() + 1, behind.end());
			placeFor(*next).chainKnown = true;
			placeFor(*next).chainGrow = 0;
			placeFor(*next).going = place(*head).going;
			placeFor(*next).goEvery = place(*head).goEvery;
			placeFor(*next).goWait = place(*head).goWait;
			// The way it was going: toward the cell of the piece cut out,
			// across; or, just turning down or up, the head's way.
			const int across = whole(place(piece).column) - whole(place(*next).column);
			placeFor(*next).goWay = across > 0 ? Direction::Right : across < 0 ? Direction::Left : place(*head).goWay;
			next->facing = place(*next).goWay;
			next->variable = head->variable;
			if (const Name look = head->lookName(); !look.empty()) { showLook(*next, look); }
		}
		if (&piece == head)
		{
			placeFor(piece).going = false;
			placeFor(piece).chainLinks.clear();
		}
	}

	bool Game::goCells(Object& mover, Direction way, float speed)
	{
		// Never straight back into its own first link.
		const std::vector<Object*> links = chainOf(mover);
		const Vector2f step = stepToward(way, 1.0f);
		if (!links.empty() && whole(place(*links.front()).column) == whole(place(mover).column) + whole(step.x)
			&& whole(place(*links.front()).row) == whole(place(mover).row) + whole(step.y))
		{
			return false;
		}

		placeFor(mover).goEvery = speed > 0.0f ? 1.0f / speed : 0.0f;
		if (!place(mover).going || place(mover).goWait > place(mover).goEvery) { placeFor(mover).goWait = place(mover).goEvery; }
		placeFor(mover).going = speed > 0.0f;
		placeFor(mover).goWay = way;
		mover.facing = way;
		return true;
	}

	bool Game::putInFreeCell(Object& made, Name map, int below)
	{
		if (!map.empty() && !moveToMap(made, map))
		{
			writtenCopies.erase(made.name);
			spawned.pop_back();
			return false;
		}
		// Every cell the whole of it fits in, one of them at random.
		std::vector<std::pair<int, int>> free;
		for (int row = below + 1; row <= mapOf(made).rows; ++row)
		{
			for (int column = 1; column <= mapOf(made).columns; ++column)
			{
				const bool taken = cellHolder(mapOf(made).name, column, row, { &made })
					|| std::any_of(stepping.begin(), stepping.end(), [&](const Stepping& each) { return each.map == mapOf(made).name && each.column == column && each.row == row; });
				if (!taken) { free.emplace_back(column, row); }
			}
		}
		if (free.empty())
		{
			writtenCopies.erase(made.name);
			spawned.pop_back();
			return false;
		}
		const std::size_t pick = std::min(free.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(free.size()))));
		return putOnCells(made, free[pick].first, free[pick].second);
	}

	void Game::slideCells(Object& mover, Direction way)
	{
		const Vector2f step = stepToward(way, 1.0f);
		for (int most = mapOf(mover).columns + mapOf(mover).rows; most > 0; --most)
		{
			if (!stepCells(mover, whole(step.x), whole(step.y))) { return; }
		}
	}

	bool Game::turnCells(Object& shape, bool clockwise)
	{
		if (!isShape(shape)) { return false; }
		const int size = cellPart(shape).shapeSize;
		std::vector<Object::ShapeCell> turned = cellPart(shape).shape;
		for (Object::ShapeCell& cell : turned)
		{
			const int column = cell.column;
			cell.column = clockwise ? size - 1 - cell.row : cell.row;
			cell.row = clockwise ? column : size - 1 - column;
		}

		const std::vector<Object*> cells = cellsOf(shape);
		if (cells.size() != turned.size()) { return false; }
		const int column = whole(place(shape).column);
		const int row = whole(place(shape).row);
		for (const auto& [kickAcross, kickUp] : kicksFor(size, static_cast<int>(cellPart(shape).shape.size()), cellPart(shape).quarter, clockwise))
		{
			const int left = column + kickAcross;
			const int top = row - kickUp;
			const bool fits = std::all_of(turned.begin(), turned.end(), [&](const Object::ShapeCell& cell)
				{
					const int c = left + cell.column;
					const int r = top + cell.row;
					return c >= 1 && r >= 1 && c <= mapOf(shape).columns && r <= mapOf(shape).rows && !cellHolder(mapOf(shape).name, c, r, cells);
				});
			if (!fits) { continue; }

			cellPartFor(shape).shape = turned;
			cellPartFor(shape).quarter = (cellPartFor(shape).quarter + (clockwise ? 1 : 3)) % 4;
			putInCell(shape, left, top);
			for (std::size_t i = 0; i < cells.size(); ++i) { putInCell(*cells[i], left + turned[i].column, top + turned[i].row); }
			return true;
		}
		return false;
	}

	void Game::settle(Object& mover)
	{
		if (!mover.onCells) { return; }
		const Name map = mapOf(mover).name;
		if (isShape(mover))
		{
			// Cells with <joins> stay joined to one another.
			const std::vector<Object*> cells = cellsOf(mover);
			for (Object* cell : cells)
			{
				cellPartFor(*cell).shapeOwner.clear();
				if (!cell->looks.set->hasJoins) { continue; }
				placeFor(*cell).joinedWith.clear();
				for (const Object* other : cells)
				{
					if (other != cell) { placeFor(*cell).joinedWith.push_back(other->name); }
				}
			}
			cellPartFor(mover).shapeParts.clear();
			despawn(mover);
		}
		askChecks(map);
	}

	void Game::askChecks(Name map)
	{
		// Every piece of a map carries its checks; any one says them.
		std::vector<Object::CellCheck> checks;
		int columns = 0;
		int rows = 0;
		for (const Object& object : objects)
		{
			if (mapOf(object).name == map && object.onCells)
			{
				checks = cellPart(object).checks;
				columns = mapOf(object).columns;
				rows = mapOf(object).rows;
				break;
			}
		}

		CommandExecutor executor(*this);
		for (const Object::CellCheck& check : checks)
		{
			if (check.kind == "line")
			{
				askLines(check, map, columns, rows, executor);
				continue;
			}
			// Full rows, from the bottom up; a row brought down by a
			// <collapse /> is asked about again.
			for (int row = rows; row >= 1;)
			{
				Object::CellMatch match{ map, row, {} };
				for (int column = 1; column <= columns; ++column)
				{
					Object* holder = cellHolder(map, column, row, {});
					if (!holder || !cellPart(*holder).shapeOwner.empty() || (!check.objClass.empty() && holder->objClass != check.objClass)) { break; }
					match.pieces.push_back(holder->name);
				}
				if (static_cast<int>(match.pieces.size()) != columns)
				{
					--row;
					continue;
				}

				for (const Command& command : check.commands)
				{
					if (!executor.run(command, Doer::finding(match))) { break; }
				}
				// Still full (nothing cleared it): on up.
				const bool stillFull = std::all_of(match.pieces.begin(), match.pieces.end(), [&](const Name name)
					{
						const Object* piece = findSpawning(name);
						return piece && piece->exists && whole(place(*piece).row) == row;
					});
				if (stillFull) { --row; }
			}
		}
	}

	std::string Game::kindOf(const Object& piece) noexcept
	{
		// A copy's group is what it copies; that one's group is the kind.
		if (piece.isCopy())
		{
			for (const Object& object : objects)
			{
				if (object.name == piece.copyOf) { return object.groupName.empty() ? object.name : object.groupName; }
			}
		}
		return piece.groupName.empty() ? piece.name : piece.groupName;
	}

	void Game::askLines(const Object::CellCheck& check, Name map, int columns, int rows, CommandExecutor& executor)
	{
		// Every line of so many alike at once, then again on what its
		// commands leave (a fall makes new lines: Columns' chains), until
		// there is none or nothing is taken away.
		for (int round = 0; round < columns * rows; ++round)
		{
			const auto loose = [&](int column, int row) -> Object*
			{
				if (column < 1 || row < 1 || column > columns || row > rows) { return nullptr; }
				Object* holder = cellHolder(map, column, row, {});
				if (!holder || !cellPart(*holder).shapeOwner.empty() || (!check.objClass.empty() && holder->objClass != check.objClass)) { return nullptr; }
				return holder;
			};
			// By group, or by the value of a variable they share (a germ and a
			// capsule half of one colour).
			const auto alike = [&](const Object* a, const Object* b)
			{
				if (!a || !b) { return false; }
				if (check.alike.empty()) { return kindOf(*a) == kindOf(*b); }
				const auto one = a->variable.find(check.alike);
				const auto two = b->variable.find(check.alike);
				return one != a->variable.end() && two != b->variable.end() && one->second == two->second;
			};

			std::vector<Object*> found;
			const int ways[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { -1, 1 } };
			const int wayCount = check.straight ? 2 : 4;
			for (int row = 1; row <= rows; ++row)
			{
				for (int column = 1; column <= columns; ++column)
				{
					Object* start = loose(column, row);
					if (!start) { continue; }
					for (int w = 0; w < wayCount; ++w)
					{
						const auto& way = ways[w];
						// Only from where a line begins.
						if (alike(loose(column - way[0], row - way[1]), start)) { continue; }
						std::vector<Object*> line{ start };
						while (Object* next = loose(column + way[0] * static_cast<int>(line.size()), row + way[1] * static_cast<int>(line.size())))
						{
							if (!alike(next, start)) { break; }
							line.push_back(next);
						}
						if (static_cast<int>(line.size()) < check.count) { continue; }
						for (Object* piece : line)
						{
							if (std::find(found.begin(), found.end(), piece) == found.end()) { found.push_back(piece); }
						}
					}
				}
			}
			if (found.empty()) { return; }

			Object::CellMatch match{ map, 0, {} };
			for (const Object* piece : found) { match.pieces.push_back(piece->name); }
			for (const Command& command : check.commands)
			{
				if (!executor.run(command, Doer::finding(match))) { break; }
			}
			const bool taken = std::any_of(match.pieces.begin(), match.pieces.end(), [&](const Name name)
				{
					const Object* piece = findSpawning(name);
					return !piece || !piece->exists;
				});
			if (!taken) { return; }
		}
	}

	void Game::fallCells(Name map, Name objClass)
	{
		const Object* any = nullptr;
		for (const Object& object : objects)
		{
			if (object.onCells && mapOf(object).name == map) { any = &object; break; }
		}
		if (!any) { return; }
		const int columns = mapOf(*any).columns;
		const int rows = mapOf(*any).rows;

		// From the bottom up, each loose piece, with the pieces joined to it,
		// drops as far as all of them can; anything else (a shape's cell; with a class
		// named, a piece of another class) stays, and stops them. Again, until nothing moves: a capsule half
		// held up by its other half falls once that one has.
		for (int pass = 0; pass < rows; ++pass)
		{
			bool moved = false;
			std::vector<Object*> done;
			for (int row = rows; row >= 1; --row)
			{
				for (int column = 1; column <= columns; ++column)
				{
					Object* holder = cellHolder(map, column, row, {});
					if (!holder || !cellPart(*holder).shapeOwner.empty() || (!objClass.empty() && holder->objClass != objClass) || std::find(done.begin(), done.end(), holder) != done.end()) { continue; }
					std::vector<Object*> together{ holder };
					for (const Name name : place(*holder).joinedWith)
					{
						Object* other = findSpawning(name);
						if (other && other->exists && other->onCells && mapOf(*other).name == map && cellPart(*other).shapeOwner.empty() && (objClass.empty() || other->objClass == objClass)
							&& std::find(together.begin(), together.end(), other) == together.end())
						{
							together.push_back(other);
						}
					}
					done.insert(done.end(), together.begin(), together.end());

					int drop = 0;
					const auto fits = [&](int by)
					{
						return std::all_of(together.begin(), together.end(), [&](const Object* piece)
							{
								const int below = whole(place(*piece).row) + by;
								return below <= rows && !cellHolder(map, whole(place(*piece).column), below, together);
							});
					};
					while (fits(drop + 1)) { ++drop; }
					if (drop == 0) { continue; }
					for (Object* piece : together) { putInCell(*piece, whole(place(*piece).column), whole(place(*piece).row) + drop); }
					moved = true;
				}
			}
			if (!moved) { return; }
		}
	}

	void Game::showJoins()
	{
		const auto join = [&](Object& piece)
		{
			if (!piece.looks.set->hasJoins || !piece.exists || !piece.onCells) { return; }
			const auto& looks = piece.looks.set->joins;
			if (std::find(looks.begin(), looks.end(), piece.looks.shown) == looks.end()) { return; }

			// Falling, the other cells of its shape; settled, those it came with.
			std::vector<Name> partners = place(piece).joinedWith;
			if (!cellPart(piece).shapeOwner.empty())
			{
				const Object* shape = findSpawning(cellPart(piece).shapeOwner);
				partners = shape ? cellPart(*shape).shapeParts : std::vector<Name>{};
			}
			Object::Join way = Object::Join::Alone;
			for (const Name name : partners)
			{
				const Object* other = findSpawning(name);
				if (!other || other == &piece || !other->exists || mapOf(*other).name != mapOf(piece).name) { continue; }
				const int across = whole(place(*other).column) - whole(place(piece).column);
				const int down = whole(place(*other).row) - whole(place(piece).row);
				if (std::abs(across) + std::abs(down) != 1) { continue; }
				way = across < 0 ? Object::Join::Left : across > 0 ? Object::Join::Right : down < 0 ? Object::Join::Up : Object::Join::Down;
				break;
			}
			const std::size_t look = looks[static_cast<std::size_t>(way)];
			if (look != piece.looks.shown) { piece.showLook(look); }
		};
		for (Object& object : objects) { join(object); }
	}

	void Game::cycleCells(Object& shape, bool backward)
	{
		if (!isShape(shape)) { return; }
		const std::vector<Object*> cells = cellsOf(shape);
		const std::size_t count = cells.size();
		if (count < 2 || count != cellPart(shape).shape.size()) { return; }

		// The cell at place i goes to place i + 1 (backward: i - 1); the places stay.
		std::vector<Name> parts(count);
		std::vector<Name> pieces(count);
		for (std::size_t i = 0; i < count; ++i)
		{
			const std::size_t to = backward ? (i + count - 1) % count : (i + 1) % count;
			parts[to] = cellPart(shape).shapeParts[i];
			pieces[to] = cellPart(shape).shape[i].piece;
		}
		const int column = whole(place(shape).column);
		const int row = whole(place(shape).row);
		for (std::size_t i = 0; i < count; ++i)
		{
			cellPartFor(shape).shapeParts[i] = parts[i];
			cellPartFor(shape).shape[i].piece = pieces[i];
			if (Object* cell = findSpawning(parts[i])) { putInCell(*cell, column + cellPart(shape).shape[i].column, row + cellPart(shape).shape[i].row); }
		}
	}

	void Game::clearCells(const Object::CellMatch& match)
	{
		for (const Name name : match.pieces)
		{
			if (Object* piece = findSpawning(name)) { despawn(*piece); }
		}
	}

	void Game::collapseCells(const Object::CellMatch& match)
	{
		const auto down = [&](Object& object)
		{
			if (object.exists && object.onCells && !isShape(object) && cellPart(object).shapeOwner.empty() && mapOf(object).name == match.map && whole(place(object).row) < match.row)
			{
				putInCell(object, whole(place(object).column), whole(place(object).row) + 1);
			}
		};
		for (Object& object : objects) { down(object); }
		for (Object& object : spawned) { down(object); }
	}

	void Game::holdCellMove(Object& object, const CmdMove& move, bool pressed)
	{
		if (!pressed)
		{
			if (cellPartFor(object).held && cellPartFor(object).heldWay == move.direction) { cellPartFor(object).held = false; }
			return;
		}

		cellPartFor(object).held = move.step > 0.0f;
		cellPartFor(object).heldWay = move.direction;
		cellPartFor(object).heldEvery = move.step > 0.0f ? 1.0f / move.step : 0.0f;
		cellPartFor(object).heldWait = move.wait > 0.0f ? move.wait : cellPartFor(object).heldEvery;

		const Vector2f step = stepToward(move.direction, 1.0f);
		stepCells(object, whole(step.x), whole(step.y));
	}

	void Game::cellMoves(float seconds)
	{
		// Each tick the step owes is a move of its own, with its own touches,
		// one after another. By index: a move can spawn, and the list grow.
		const auto counts = [&](std::size_t i) { return objects[i].exists && isShown(objects[i]) && !awayFromHome(objects[i]); };
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (!counts(i)) { continue; }
			if (place(objects[i]).going)
			{
				placeFor(objects[i]).goWait -= seconds;
				while (counts(i) && place(objects[i]).going && place(objects[i]).goWait <= kTimeSlack)
				{
					placeFor(objects[i]).goWait += place(objects[i]).goEvery;
					const Vector2f step = stepToward(place(objects[i]).goWay, 1.0f);
					stepCells(objects[i], whole(step.x), whole(step.y));
				}
			}
			if (!counts(i) || !cellPart(objects[i]).held) { continue; }
			cellPartFor(objects[i]).heldWait -= seconds;
			while (counts(i) && cellPartFor(objects[i]).held && cellPartFor(objects[i]).heldWait <= kTimeSlack)
			{
				cellPartFor(objects[i]).heldWait += cellPartFor(objects[i]).heldEvery;
				const Vector2f step = stepToward(cellPart(objects[i]).heldWay, 1.0f);
				stepCells(objects[i], whole(step.x), whole(step.y));
			}
		}
	}

	const Object::MoveSet* Game::moveSetOf(const Object& piece) const noexcept
	{
		const Object::MoveSet* any = nullptr;
		for (const Object::MoveSet& set : pieceMoves(piece).sets)
		{
			if (set.look && *set.look == piece.looks.shown) { return &set; }
			if (!set.look && !any) { any = &set; }
		}
		return any;
	}

	std::vector<Game::PieceMoveTo> Game::movesOf(Object& piece, bool jumpsOnly)
	{
		std::vector<PieceMoveTo> found;
		const Object::MoveSet* set = moveSetOf(piece);
		if (!set || !piece.onCells || !piece.exists) { return found; }
		const int column = whole(place(piece).column);
		const int row = whole(place(piece).row);
		const auto onMap = [&](int c, int r) { return c >= 1 && r >= 1 && c <= mapOf(piece).columns && r <= mapOf(piece).rows; };
		for (const Object::PieceMove& move : set->moves)
		{
			if (jumpsOnly && !move.jump) { continue; }
			const int by = move.jump ? 2 : 1;
			const int toColumn = column + move.x * by;
			const int toRow = row + move.y * by;
			if (!onMap(toColumn, toRow) || cellHolder(mapOf(piece).name, toColumn, toRow, { &piece })) { continue; }
			if (!move.jump)
			{
				found.push_back({ toColumn, toRow, nullptr });
				continue;
			}
			Object* over = cellHolder(mapOf(piece).name, column + move.x, row + move.y, { &piece });
			if (over && over->objClass == move.objClass) { found.push_back({ toColumn, toRow, over }); }
		}
		return found;
	}

	std::vector<Game::PieceMoveTo> Game::legalMoves(Object& piece, bool jumpsOnly)
	{
		const Object::MoveSet* set = moveSetOf(piece);
		if (!set) { return {}; }
		if (!jumpsOnly && set->forced)
		{
			// Any piece of its side that can jump makes jumps the only moves.
			for (Object& other : objects)
			{
				if (pieceMoves(other).sets.empty() || other.objClass != piece.objClass || mapOf(other).name != mapOf(piece).name || !isShown(other)) { continue; }
				if (!movesOf(other, true).empty()) { jumpsOnly = true; break; }
			}
		}
		return movesOf(piece, jumpsOnly);
	}

	void Game::countMoves()
	{
		// Nothing moves while they are counted.
		const CellsStill still(*this);
		// The cells a class may put a piece in are the same for each of its
		// pieces: worked out once.
		std::vector<std::pair<std::string, float>> puts;
		for (Object& piece : objects)
		{
			Object::Moves* moves = objectParts.moves.find(piece.id);
			if (!moves || moves->sets.empty()) { continue; }
			if (!isShown(piece)) { moves->count = 0.0f; continue; }
			float count = static_cast<float>(movesOf(piece, false).size());
			const Object::MoveSet* set = moveSetOf(piece);
			if (set && set->puts())
			{
				auto known = std::find_if(puts.begin(), puts.end(), [&](const auto& each) { return each.first == piece.objClass; });
				if (known == puts.end())
				{
					puts.emplace_back(piece.objClass, static_cast<float>(putCells(piece).size()));
					known = puts.end() - 1;
				}
				count += known->second;
			}
			moves->count = count;
		}
	}

	Object* Game::putTemplate(Name objClass)
	{
		const std::vector<Object*> all = putTemplates(objClass);
		return all.empty() ? nullptr : all.front();
	}

	std::vector<Object*> Game::putTemplates(Name objClass)
	{
		// One of each group: pieces of a group are alike.
		std::vector<Object*> found;
		std::vector<std::string> groups;
		for (std::size_t i = 0; i < loadedCount && i < objects.size(); ++i)
		{
			Object& object = objects[i];
			if (object.objClass != objClass || !object.onCells || isShape(object)) { continue; }
			const std::string group = object.groupName.empty() ? object.name : object.groupName;
			if (std::find(groups.begin(), groups.end(), group) != groups.end()) { continue; }
			groups.push_back(group);
			found.push_back(&object);
		}
		return found;
	}

	std::vector<Object*> Game::flanked(const Object& kind, int column, int row, bool& can)
	{
		std::vector<Object*> trapped;
		can = false;
		const auto onMap = [&](int c, int r) { return c >= 1 && r >= 1 && c <= mapOf(kind).columns && r <= mapOf(kind).rows; };
		if (!onMap(column, row)) { return trapped; }
		const Object::MoveSet* set = moveSetOf(kind);
		// On a piece of a class it goes on (which it takes the place of), or
		// else in a free cell.
		Object* under = cellHolder(mapOf(kind).name, column, row, {});
		if (set && !set->on.empty())
		{
			if (!under || std::find(set->on.begin(), set->on.end(), under->objClass) == set->on.end()) { return trapped; }
		}
		else if (under) { return trapped; }
		// Falling: only onto the floor or onto a piece.
		if (set && set->fall && row < mapOf(kind).rows && !cellHolder(mapOf(kind).name, column, row + 1, {})) { return trapped; }
		if (!set || set->flanks.empty())
		{
			can = true;
			return trapped;
		}
		for (const Object::Flank& flank : set->flanks)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					if ((dx == 0 && dy == 0) || (flank.straight && dx != 0 && dy != 0)) { continue; }
					// A line of the flank's class, then one of its own.
					std::vector<Object*> line;
					for (int c = column + dx, r = row + dy; onMap(c, r); c += dx, r += dy)
					{
						Object* there = cellHolder(mapOf(kind).name, c, r, {});
						if (!there) { break; }
						if (there->objClass == flank.objClass && there->objClass != kind.objClass)
						{
							line.push_back(there);
							continue;
						}
						if (there->objClass == kind.objClass)
						{
							for (Object* each : line)
							{
								if (std::find(trapped.begin(), trapped.end(), each) == trapped.end()) { trapped.push_back(each); }
							}
						}
						break;
					}
				}
			}
		}
		can = !trapped.empty();
		return trapped;
	}

	std::vector<std::pair<int, int>> Game::putCells(const Object& kind)
	{
		// Any piece of its class that fits the cell.
		std::vector<const Object*> kinds;
		for (const Object* each : putTemplates(kind.objClass)) { kinds.push_back(each); }
		if (kinds.empty()) { kinds.push_back(&kind); }
		std::vector<std::pair<int, int>> cells;
		for (int row = 1; row <= mapOf(kind).rows; ++row)
		{
			for (int column = 1; column <= mapOf(kind).columns; ++column)
			{
				for (const Object* each : kinds)
				{
					bool can = false;
					flanked(*each, column, row, can);
					if (can)
					{
						cells.emplace_back(column, row);
						break;
					}
				}
			}
		}
		return cells;
	}

	bool Game::putPiece(Object& picker, const CmdPut& put)
	{
		if (!picker.onCells || !pickerOf(picker).picked.empty()) { return false; }
		const int column = whole(place(picker).column);
		// The first piece of the class that may go there.
		Object* kind = nullptr;
		int row = 0;
		bool can = false;
		std::vector<Object*> trapped;
		for (Object* each : putTemplates(put.objClass))
		{
			row = whole(place(picker).row);
			// Falling: wherever the picker is in its column, to the lowest free cell.
			if (const Object::MoveSet* set = moveSetOf(*each); set && set->fall)
			{
				row = 0;
				for (int at = mapOf(*each).rows; at >= 1 && row == 0; --at)
				{
					if (!cellHolder(mapOf(*each).name, column, at, {})) { row = at; }
				}
				if (row == 0) { continue; }
			}
			trapped = flanked(*each, column, row, can);
			if (can)
			{
				kind = each;
				break;
			}
		}
		if (!kind) { return false; }

		// Turned: each trapped piece goes, and one of the new kind comes in
		// its cell; what it is put on goes too. (Names and cells first: a
		// spawn may move the pieces spawned this frame.)
		const Object::MoveSet* kindSet = moveSetOf(*kind);
		const Name closeClass = kindSet ? kindSet->closeClass : Name();
		const Name closeObject = kindSet ? kindSet->closeObject : Name();
		const Name name = kind->name;
		const Name map = mapOf(*kind).name;
		const int columns = mapOf(*kind).columns;
		const int rows = mapOf(*kind).rows;
		if (Object* under = cellHolder(map, column, row, {})) { despawn(*under); }
		std::vector<std::pair<int, int>> cells{ { column, row } };
		for (Object* each : trapped)
		{
			cells.emplace_back(whole(place(*each).column), whole(place(*each).row));
			despawn(*each);
		}
		for (const auto& [c, r] : cells)
		{
			if (Object* made = spawn(name)) { putInCell(*made, c, r); }
		}

		// <close> in its moves: each piece of that class beside it now walled in, no place
		// left beside it that a piece could be put <on> (lines of either
		// side: the slots are gone), is taken by a copy of the object named.
		bool closed = false;
		if (!closeClass.empty())
		{
			std::vector<std::string> places;
			for (const Object* each : putTemplates(put.objClass))
			{
				if (const Object::MoveSet* set = moveSetOf(*each))
				{
					places.insert(places.end(), set->on.begin(), set->on.end());
				}
			}
			const auto onMap = [&](int c, int r) { return c >= 1 && r >= 1 && c <= columns && r <= rows; };
			const auto put_ = [&](int c, int r)
			{
				const Object* there = onMap(c, r) ? cellHolder(map, c, r, {}) : nullptr;
				return there && std::find(places.begin(), places.end(), there->objClass) == places.end();
			};
			const int sides[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
			for (const auto& side : sides)
			{
				const int c = column + side[0];
				const int r = row + side[1];
				Object* box = onMap(c, r) ? cellHolder(map, c, r, {}) : nullptr;
				if (!box || box->objClass != closeClass) { continue; }
				if (!put_(c + 1, r) || !put_(c - 1, r) || !put_(c, r + 1) || !put_(c, r - 1)) { continue; }
				despawn(*box);
				if (Object* made = spawn(closeObject)) { putInCell(*made, c, r); }
				closed = true;
			}
		}
		pickerFor(picker).fromColumn = column;
		pickerFor(picker).fromRow = row;
		pickerFor(picker).toColumn = column;
		pickerFor(picker).toRow = row;
		// The map's checks asked about what it has now (a line made).
		askChecks(map);
		countMoves();
		// Something closed: the turn goes on, as after a jump that must be
		// followed; what is written after the put is not done.
		return !closed;
	}

	float Game::countClass(Name objClass)
	{
		float count = 0.0f;
		for (Object& object : objects)
		{
			if (object.objClass == objClass && object.exists && isShown(object)) { count += 1.0f; }
		}
		for (const Object& object : spawned)
		{
			if (object.objClass == objClass && object.exists) { count += 1.0f; }
		}
		return count;
	}

	bool Game::pickPiece(Object& picker, Name objClass, bool only)
	{
		if (pickerOf(picker).inChain || !picker.onCells) { return false; }
		if (only)
		{
			Object* one = nullptr;
			for (Object& piece : objects)
			{
				if (pieceMoves(piece).sets.empty() || piece.objClass != objClass || !piece.onCells || !isShown(piece) || legalMoves(piece, false).empty()) { continue; }
				if (one) { return false; }
				one = &piece;
			}
			if (!one || pickerOf(picker).picked == one->name) { return false; }
			pickerFor(picker).picked = one->name;
			return true;
		}
		const int column = whole(place(picker).column);
		const int row = whole(place(picker).row);
		for (Object& piece : objects)
		{
			if (&piece == &picker || pieceMoves(piece).sets.empty() || piece.objClass != objClass || !piece.onCells || !isShown(piece)
				|| whole(place(piece).column) != column || whole(place(piece).row) != row)
			{
				continue;
			}
			if (legalMoves(piece, false).empty()) { return false; }
			pickerFor(picker).picked = piece.name;
			return true;
		}
		return false;
	}

	bool Game::placePiece(Object& picker)
	{
		if (pickerOf(picker).picked.empty() || !picker.onCells) { return false; }
		Object* piece = findSpawning(pickerOf(picker).picked);
		if (!piece || !piece->exists || !isShown(*piece))
		{
			pickerFor(picker).picked.clear();
			pickerFor(picker).inChain = false;
			return false;
		}
		const int column = whole(place(picker).column);
		const int row = whole(place(picker).row);
		const std::vector<PieceMoveTo> moves = legalMoves(*piece, pickerOf(picker).inChain);
		const auto move = std::find_if(moves.begin(), moves.end(), [&](const PieceMoveTo& each) { return each.column == column && each.row == row; });
		if (move == moves.end()) { return false; }

		const Object::MoveSet* set = moveSetOf(*piece);
		const bool jumped = move->taken != nullptr;
		if (!pickerOf(picker).inChain)
		{
			pickerFor(picker).fromColumn = whole(place(*piece).column);
			pickerFor(picker).fromRow = whole(place(*piece).row);
		}
		pickerFor(picker).toColumn = column;
		pickerFor(picker).toRow = row;
		if (move->taken) { despawn(*move->taken); }
		putInCell(*piece, column, row);

		// Promoted on reaching its row: that ends its move.
		bool promoted = false;
		if (set && set->promoteLook && row == set->promoteRow && piece->looks.shown != *set->promoteLook)
		{
			piece->showLook(*set->promoteLook);
			promoted = true;
		}

		// A jump that can be followed by another must be: the turn goes on.
		if (jumped && !promoted && !movesOf(*piece, true).empty())
		{
			pickerFor(picker).inChain = true;
			countMoves();
			return false;
		}
		pickerFor(picker).picked.clear();
		pickerFor(picker).inChain = false;
		countMoves();
		return true;
	}

	bool Game::dropPiece(Object& picker)
	{
		if (pickerOf(picker).picked.empty() || pickerOf(picker).inChain) { return false; }
		pickerFor(picker).picked.clear();
		return true;
	}

	// The cells holding a piece in play of one of these classes.
	std::vector<std::pair<int, int>> Game::ontoCells(const std::vector<Name>& onto)
	{
		std::vector<std::pair<int, int>> cells;
		for (const Object& object : objects)
		{
			if (!object.onCells || !isShown(object)) { continue; }
			if (std::find(onto.begin(), onto.end(), object.objClass) == onto.end()) { continue; }
			cells.emplace_back(whole(place(object).column), whole(place(object).row));
		}
		return cells;
	}

	// <hop direction="right"><onto class="hslot" /></hop>: left and right,
	// to the nearest such cell along the row; up and down, to the nearest
	// row ahead that has one, the cell in it nearest the column kept (the
	// one the first of these hops started from, as a text cursor keeps
	// its column). Refused when there is none that way.
	bool Game::hopOnto(Object& mover, Direction direction, const std::vector<Name>& onto)
	{
		const int column = whole(place(mover).column);
		const int row = whole(place(mover).row);
		const bool upDown = direction == Direction::Up || direction == Direction::Down;
		const int ahead = direction == Direction::Down || direction == Direction::Right ? 1 : -1;
		if (!upDown || pickerFor(mover).keptAt != std::pair<int, int>{ column, row }) { pickerFor(mover).keptColumn = column; }

		std::optional<std::pair<int, int>> best;
		int bestAlong = 0;
		int bestAcross = 0;
		for (const auto& [c, r] : ontoCells(onto))
		{
			if (!upDown && r != row) { continue; }
			const int along = upDown ? (r - row) * ahead : (c - column) * ahead;
			const int across = upDown ? std::abs(c - pickerOf(mover).keptColumn) : 0;
			if (along <= 0) { continue; }
			if (!best || along < bestAlong || (along == bestAlong && (across < bestAcross || (across == bestAcross && c < best->first))))
			{
				best = std::pair<int, int>{ c, r };
				bestAlong = along;
				bestAcross = across;
			}
		}
		if (!best) { return false; }
		putInCell(mover, best->first, best->second);
		mover.facing = direction;
		pickerFor(mover).keptAt = upDown ? *best : std::pair<int, int>{};
		return true;
	}

	void Game::trackCells(Object& object, bool horizontal, float at, const std::vector<Name>& onto)
	{
		// <onto>: the cell nearest where it points, of those holding such a
		// piece, while it points over the map.
		if (!onto.empty())
		{
			(horizontal ? pickerFor(object).pointer.x : pickerFor(object).pointer.y) = at;
			const auto middle = [&](int column, int row)
			{
				return Vector2f{ mapOf(object).origin.x + (static_cast<float>(column) - 0.5f) * mapOf(object).tile.x,
					mapOf(object).origin.y + (static_cast<float>(row) - 0.5f) * mapOf(object).tile.y };
			};
			Vector2f point = pickerOf(object).pointer;
			const Vector2f here = middle(whole(place(object).column), whole(place(object).row));
			if (point.x < 0.0f) { point.x = here.x; }
			if (point.y < 0.0f) { point.y = here.y; }
			if (point.x < mapOf(object).origin.x || point.y < mapOf(object).origin.y
				|| point.x > mapOf(object).origin.x + mapOf(object).tile.x * static_cast<float>(mapOf(object).columns)
				|| point.y > mapOf(object).origin.y + mapOf(object).tile.y * static_cast<float>(mapOf(object).rows)) { return; }
			std::optional<std::pair<int, int>> best;
			float bestDistance = 0.0f;
			for (const auto& [c, r] : ontoCells(onto))
			{
				const Vector2f to = middle(c, r);
				const float distance = (to.x - point.x) * (to.x - point.x) + (to.y - point.y) * (to.y - point.y);
				if (!best || distance < bestDistance)
				{
					best = std::pair<int, int>{ c, r };
					bestDistance = distance;
				}
			}
			if (best && (best->first != whole(place(object).column) || best->second != whole(place(object).row))) { putInCell(object, best->first, best->second); }
			return;
		}
		const float from = horizontal ? mapOf(object).origin.x : mapOf(object).origin.y;
		const float tile = horizontal ? mapOf(object).tile.x : mapOf(object).tile.y;
		if (tile <= 0.0f || at < from) { return; }
		const int cell = static_cast<int>((at - from) / tile) + 1;
		if (cell > (horizontal ? mapOf(object).columns : mapOf(object).rows)) { return; }
		const int column = horizontal ? cell : whole(place(object).column);
		const int row = horizontal ? whole(place(object).row) : cell;
		if (column != whole(place(object).column) || row != whole(place(object).row)) { putInCell(object, column, row); }
	}

	void Game::showMarks()
	{
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			// Not in play now (a pause over it): its marks stay as they are.
			if (pickerOf(objects[i]).marks.sets.empty() || !isShown(objects[i])) { continue; }
			Object& picker = objects[i];

			// What is marked now: the set for its look, and the one for any.
			std::vector<Object::Marked> want;
			{
				const auto on = [&](const std::string& variable)
				{
					if (variable.empty()) { return true; }
					VariableName name = VariableName::of(variable);
					name.owner = fullName(name.owner);
					const float* value = variableOf(name);
					return value && *value != 0.0f;
				};
				Object* held = pickerOf(picker).picked.empty() ? nullptr : findSpawning(pickerOf(picker).picked);
				for (const Object::MarkSet& set : pickerOf(picker).marks.sets)
				{
					if (set.look && *set.look != picker.looks.shown) { continue; }
					for (const Object::Mark& mark : set.marks)
					{
						if (!on(mark.variable)) { continue; }
						// No class=: the pieces of the player on its turn.
						const Name markClass = mark.objClass.empty() ? turnClass() : mark.objClass;
						switch (mark.kind)
						{
						case Object::Mark::Kind::Held:
							if (held) { want.push_back({ mark.object, whole(place(*held).column), whole(place(*held).row) }); }
							break;
						case Object::Mark::Kind::Moves:
							if (held)
							{
								for (const PieceMoveTo& move : legalMoves(*held, pickerOf(picker).inChain)) { want.push_back({ mark.object, move.column, move.row }); }
							}
							break;
						case Object::Mark::Kind::Movable:
							for (Object& piece : objects)
							{
								if (pieceMoves(piece).sets.empty() || piece.objClass != markClass || !piece.onCells || !isShown(piece) || legalMoves(piece, false).empty()) { continue; }
								want.push_back({ mark.object, whole(place(piece).column), whole(place(piece).row) });
							}
							break;
						case Object::Mark::Kind::Put:
							if (const Object* kind = markClass.empty() ? nullptr : putTemplate(markClass))
							{
								const CellsStill still(*this);
								for (const auto& [column, row] : putCells(*kind)) { want.push_back({ mark.object, column, row }); }
							}
							break;
						case Object::Mark::Kind::Last:
							if (pickerOf(picker).toColumn > 0)
							{
								want.push_back({ mark.object, pickerOf(picker).fromColumn, pickerOf(picker).fromRow });
								// A piece put: one cell.
								if (pickerOf(picker).toColumn != pickerOf(picker).fromColumn || pickerOf(picker).toRow != pickerOf(picker).fromRow)
								{
									want.push_back({ mark.object, pickerOf(picker).toColumn, pickerOf(picker).toRow });
								}
							}
							break;
						}
					}
				}
			}
			if (want == pickerOf(objects[i]).marks.marked) { continue; }

			// Made again: the old copies out, a new one in each cell marked.
			std::vector<Name> old = std::move(pickerFor(objects[i]).marks.copies);
			pickerFor(objects[i]).marks.copies.clear();
			pickerFor(objects[i]).marks.marked = want;
			for (const Name copy : old) { despawn(copy); }
			std::vector<Name> made;
			for (const Object::Marked& mark : want)
			{
				Object* copy = spawn(mark.object);
				if (!copy || !copy->onCells) { continue; }
				putInCell(*copy, mark.column, mark.row);
				made.push_back(copy->name);
			}
			pickerFor(objects[i]).marks.copies = std::move(made);
		}
	}

	bool Game::shoveCells(Object& piece, Edge way)
	{
		if (!piece.onCells || !piece.exists) { return false; }
		// A shape's cell is shoved with its shape.
		Object* shoved = &piece;
		if (!cellPart(piece).shapeOwner.empty())
		{
			if (Object* owner = find(cellPart(piece).shapeOwner)) { shoved = owner; }
		}
		const int across = way == Edge::Left ? -1 : way == Edge::Right ? 1 : 0;
		const int down = way == Edge::Top ? -1 : way == Edge::Bottom ? 1 : 0;
		return stepCells(*shoved, across, down);
	}

	bool Game::mergeCells(Object& mover, Object& other, Name variable)
	{
		if (!mover.exists || !other.exists || !mover.onCells || !other.onCells) { return false; }
		if (std::find(merged.begin(), merged.end(), &mover) != merged.end() || std::find(merged.begin(), merged.end(), &other) != merged.end()) { return false; }
		const auto mine = mover.variable.find(variable);
		const auto theirs = other.variable.find(variable);
		if (mine == mover.variable.end() || theirs == other.variable.end() || mine->second != theirs->second) { return false; }

		const float sum = mine->second + theirs->second;
		despawn(other);
		setVariable(mover.name, variable, sum);
		merged.push_back(&mover);
		return true;
	}

	bool Game::slideClass(Name objClass, Direction way)
	{
		std::vector<Object*> pieces;
		for (Object& object : objects)
		{
			if (object.objClass == objClass && object.exists && object.onCells && !isShape(object) && !awayFromHome(object) && isShown(object)) { pieces.push_back(&object); }
		}
		// The one nearest that way first, so the rest slide up behind it.
		const auto key = [this, way](const Object* piece)
		{
			const int column = whole(place(*piece).column);
			const int row = whole(place(*piece).row);
			switch (way)
			{
			case Direction::Left: return column * 1000 + row;
			case Direction::Right: return -column * 1000 + row;
			case Direction::Up: return row * 1000 + column;
			default: return -row * 1000 + column;
			}
		};
		std::stable_sort(pieces.begin(), pieces.end(), [&](const Object* a, const Object* b) { return key(a) < key(b); });

		merged.clear();
		bool moved = false;
		for (Object* piece : pieces)
		{
			if (!piece->exists) { continue; }
			const int column = whole(place(*piece).column);
			const int row = whole(place(*piece).row);
			slideCells(*piece, way);
			moved = moved || !piece->exists || whole(place(*piece).column) != column || whole(place(*piece).row) != row;
		}
		merged.clear();
		return moved;
	}

	std::vector<Object*> Game::piecesAbout(const Object& piece, Name objClass, const CellPlace& place)
	{
		std::vector<Object*> found;
		if (!piece.onCells || !place.any()) { return found; }

		// The cells, in reading order.
		std::vector<std::pair<int, int>> offsets;
		if (place.where == CellPlace::Where::Here) { offsets.push_back({ 0, 0 }); }
		else
		{
			for (int down = -1; down <= 1; ++down)
			{
				for (int across = -1; across <= 1; ++across)
				{
					if (across == 0 && down == 0) { continue; }
					if (place.straight && across != 0 && down != 0) { continue; }
					offsets.push_back({ across, down });
				}
			}
		}

		const auto sameLayout = [&](const Object& other)
		{
			return std::abs(mapOf(other).origin.x - mapOf(piece).origin.x) < 0.5f && std::abs(mapOf(other).origin.y - mapOf(piece).origin.y) < 0.5f
				&& std::abs(mapOf(other).tile.x - mapOf(piece).tile.x) < 0.5f && std::abs(mapOf(other).tile.y - mapOf(piece).tile.y) < 0.5f;
		};
		const auto alike = [&](const Object& other)
		{
			if (place.alike.empty()) { return true; }
			const auto mine = piece.variable.find(place.alike);
			const auto theirs = other.variable.find(place.alike);
			return mine != piece.variable.end() && theirs != other.variable.end() && mine->second == theirs->second;
		};
		const int column = whole(this->place(piece).column);
		const int row = whole(this->place(piece).row);
		const auto look = [&](Object& other, bool joined, int wantColumn, int wantRow)
		{
			if (&other == &piece || !other.exists || !other.onCells || isShape(other) || awayFromHome(other)) { return; }
			if (!joined && !isShown(other)) { return; }
			if (!objClass.empty() && other.objClass != objClass) { return; }
			if (whole(this->place(other).column) != wantColumn || whole(this->place(other).row) != wantRow || !sameLayout(other) || !alike(other)) { return; }
			found.push_back(&other);
		};
		for (const auto& [across, down] : offsets)
		{
			for (Object& other : objects) { look(other, false, column + across, row + down); }
			for (Object& other : spawned) { look(other, true, column + across, row + down); }
		}
		return found;
	}

	std::vector<Game::Kept> Game::keepAll() const
	{
		std::vector<Kept> kept;
		kept.reserve(objects.size());
		for (const Object& object : objects)
		{
			kept.push_back({ object.name, static_cast<const Moment&>(object), place(object), object.looks.shown });
		}
		return kept;
	}

	void Game::pressBegins()
	{
		merged.clear();
		if (!keepsUndo) { return; }
		undoneThisPress = false;
		pressKept = keepAll();
	}

	void Game::pressEnds()
	{
		if (!keepsUndo || pressKept.empty()) { return; }
		std::vector<Kept> before = std::move(pressKept);
		pressKept.clear();
		if (undoneThisPress) { return; }

		// Kept only when a piece on cells moved, came or went.
		bool changed = false;
		std::size_t at = 0;
		for (const Object& object : objects)
		{
			const Kept* was = at < before.size() && before[at].name == object.name ? &before[at] : nullptr;
			++at;
			if (!object.onCells) { continue; }
			if (!was || was->moment.exists != object.exists || was->place.column != place(object).column || was->place.row != place(object).row)
			{
				changed = true;
				break;
			}
		}
		changed = changed || before.size() != objects.size() || !spawned.empty();
		if (changed) { undoSteps.push_back(std::move(before)); }
	}

	bool Game::undo()
	{
		if (undoSteps.empty()) { return false; }
		std::vector<Kept> back = std::move(undoSteps.back());
		undoSteps.pop_back();
		undoneThisPress = true;

		for (Object& made : spawned) { made.exists = false; }
		for (Object& object : objects)
		{
			const auto was = std::find_if(back.begin(), back.end(), [&](const Kept& kept) { return kept.name == object.name; });
			if (was == back.end())
			{
				// Made since: gone again.
				if (object.exists) { despawn(object); }
				continue;
			}
			static_cast<Moment&>(object) = was->moment;
			if (Object::Cells* cells = objectParts.cells.find(object.id)) { cells->place = was->place; }
			object.placed = true;
			if (object.looks.shown != was->look && was->look < object.looks.all().size()) { object.showLook(was->look); }
			for (const auto& [variable, value] : was->moment.variable) { setVariable(object.name, variable, value); }
			shareVariables(object);
		}
		stageChanged();
		return true;
	}
}
