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

	Object* Game::findAnywhere(const std::string& name) noexcept
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

	Object* Game::cellHolder(const std::string& map, int column, int row, const std::vector<Object*>& mover)
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
					if (object.exists && object.onCells && !object.isShape() && object.mapName == map && isShown(object))
					{
						cells.emplace(cellKey(whole(object.cellColumn), whole(object.cellRow)), &object);
					}
				}
				for (Object& object : spawned)
				{
					if (object.exists && object.onCells && !object.isShape() && object.mapName == map)
					{
						cells.emplace(cellKey(whole(object.cellColumn), whole(object.cellRow)), &object);
					}
				}
				found = heldCells.maps.emplace(map, std::move(cells)).first;
			}
			const auto cell = found->second.find(cellKey(column, row));
			return cell == found->second.end() ? nullptr : cell->second;
		}
		const auto holds = [&](Object& object)
		{
			return object.exists && object.onCells && !object.isShape() && object.mapName == map
				&& whole(object.cellColumn) == column && whole(object.cellRow) == row
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
		if (!mover.isShape()) { return { &mover }; }
		std::vector<Object*> cells;
		for (const std::string& part : mover.shapeParts)
		{
			if (Object* cell = findAnywhere(part)) { cells.push_back(cell); }
		}
		return cells;
	}

	void Game::putInCell(Object& object, int column, int row)
	{
		object.cellColumn = static_cast<float>(column);
		object.cellRow = static_cast<float>(row);
		object.mapColumn = column - 1;
		object.mapRow = row - 1;
		const Vector2f size = sizeOf(object);
		object.position = object.mapOrigin + Vector2f{ (static_cast<float>(column) - 0.5f) * object.mapTile.x - size.x / 2.0f,
			(static_cast<float>(row) - 0.5f) * object.mapTile.y - size.y / 2.0f };
	}

	bool Game::moveToMap(Object& object, const std::string& map)
	{
		if (object.mapName == map) { return true; }
		const auto onIt = [&](const Object& each) { return each.onCells && each.mapName == map; };
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
		const Vector2f origin = like->mapOrigin;
		const Vector2f tile = like->mapTile;
		const int columns = like->mapColumns;
		const int rows = like->mapRows;
		std::vector<Object::CellCheck> checks = like->cellChecks;
		object.mapName = map;
		object.mapOrigin = origin;
		object.mapTile = tile;
		object.mapColumns = columns;
		object.mapRows = rows;
		object.cellChecks = std::move(checks);
		object.cellHeld = false;
		return true;
	}

	bool Game::putOnCells(Object& made, int column, int row, const std::string& map)
	{
		const auto fits = [&](int c, int r)
		{
			return c >= 1 && r >= 1 && c <= made.mapColumns && r <= made.mapRows && !cellHolder(made.mapName, c, r, { &made });
		};

		const auto takeBack = [&]()
		{
			copyRaws.erase(made.name);
			spawned.pop_back();
			return false;
		};

		if (!map.empty() && !moveToMap(made, map)) { return takeBack(); }

		if (!made.isShape())
		{
			if (!fits(column, row)) { return takeBack(); }
			putInCell(made, column, row);
			return true;
		}

		for (const Object::ShapeCell& cell : made.shapeCells)
		{
			if (!fits(column + cell.column, row + cell.row)) { return takeBack(); }
		}
		putInCell(made, column, row);
		for (const Object::ShapeCell& cell : made.shapeCells)
		{
			Object* part = spawn(cell.piece, true);
			if (!part) { continue; }
			moveToMap(*part, made.mapName);
			part->shapeOwner = made.name;
			putInCell(*part, column + cell.column, row + cell.row);
			made.shapeParts.push_back(part->name);
		}
		return true;
	}

	bool Game::sendCells(const std::string& target, const std::string& from, const std::string& into, std::optional<int> column, std::optional<int> row)
	{
		// The first of that name on `from` that moves as one: a shape, or a
		// piece that is no shape's cell.
		Object* piece = nullptr;
		for (Object& object : objects)
		{
			if (object.exists && object.onCells && object.mapName == from && object.shapeOwner.empty()
				&& (object.name == target || object.inGroup(target)) && isShown(object))
			{
				piece = &object;
				break;
			}
		}
		if (!piece) { return false; }

		const std::vector<Object*> cells = cellsOf(*piece);
		const int left = column.value_or(whole(piece->cellColumn));
		const int top = row.value_or(whole(piece->cellRow));
		const int fromLeft = whole(piece->cellColumn);
		const int fromTop = whole(piece->cellRow);

		// Where each cell goes, kept as it lies from the shape's top left.
		Object probe = *piece;
		if (!moveToMap(probe, into)) { return false; }
		for (Object* cell : cells)
		{
			const int c = left + whole(cell->cellColumn) - fromLeft;
			const int r = top + whole(cell->cellRow) - fromTop;
			if (c < 1 || r < 1 || c > probe.mapColumns || r > probe.mapRows || cellHolder(into, c, r, cells)) { return false; }
		}

		for (Object* cell : cells)
		{
			const int c = left + whole(cell->cellColumn) - fromLeft;
			const int r = top + whole(cell->cellRow) - fromTop;
			moveToMap(*cell, into);
			putInCell(*cell, c, r);
		}
		if (piece->isShape())
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
		if (!links.empty() && mover.chainGrow == 0) { passing.push_back(links.back()); }

		// The first thing in the way, or the map's edge.
		const auto inTheWay = [&](bool& offMap) -> Object*
		{
			offMap = false;
			for (Object* cell : cells)
			{
				const int column = whole(cell->cellColumn) + across;
				const int row = whole(cell->cellRow) + down;
				if (column < 1 || row < 1 || column > mover.mapColumns || row > mover.mapRows)
				{
					offMap = true;
					return nullptr;
				}
				if (Object* holder = cellHolder(mover.mapName, column, row, passing)) { return holder; }
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
			for (Object* cell : cells) { stepping.push_back({ mover.mapName, whole(cell->cellColumn) + across, whole(cell->cellRow) + down }); }
			Touch touch;
			touch.edge = edge;
			touch.normal = outOf(opposite(edge));
			touch.along = 0.5f;
			touch.point = middleOf(*blocker);
			const int beforeColumn = whole(mover.cellColumn);
			const int beforeRow = whole(mover.cellRow);
			goThrough = false;
			reactTo(mover, *blocker, touch);
			Touch back = touch;
			back.edge = opposite(edge);
			back.normal = outOf(edge);
			reactTo(*blocker, mover, back);
			// A shoved piece's own step is asked inside these rules.
			stepping.resize(steppingBefore);

			// A touch that took what was in the way away (food eaten) lets
			// the step go on; one that moved the mover (a centipede turning)
			// has made its own step instead.
			// A <through /> in the rules makes it anyway.
			const bool moved = whole(mover.cellColumn) != beforeColumn || whole(mover.cellRow) != beforeRow;
			const bool through = goThrough;
			goThrough = false;
			if (moved || !mover.exists) { return false; }
			if (!through && (inTheWay(offMap) || offMap)) { return false; }
		}

		const int fromColumn = whole(mover.cellColumn);
		const int fromRow = whole(mover.cellRow);
		for (Object* cell : cells) { putInCell(*cell, whole(cell->cellColumn) + across, whole(cell->cellRow) + down); }
		if (mover.isShape()) { putInCell(mover, whole(mover.cellColumn) + across, whole(mover.cellRow) + down); }
		if (!mover.chain.empty()) { followChain(mover, links, fromColumn, fromRow); }
		return true;
	}

	bool Game::isLink(const Object& piece, const std::string& chain)
	{
		if (piece.named(chain)) { return true; }
		if (!piece.isCopy()) { return false; }
		const Object* of = findAnywhere(piece.copyOf);
		return piece.copyOf == chain || (of && of != &piece && of->named(chain));
	}

	std::vector<Object*> Game::chainOf(Object& head)
	{
		if (head.chain.empty()) { return {}; }

		// As kept: a link no longer there is dropped.
		std::vector<Object*> links;
		if (head.chainKnown)
		{
			std::vector<std::string> kept;
			for (const std::string& name : head.chainLinks)
			{
				Object* link = findAnywhere(name);
				if (!link || !link->exists) { continue; }
				links.push_back(link);
				kept.push_back(name);
			}
			head.chainLinks = std::move(kept);
			return links;
		}

		// Found from where they lie: from the head, the next one beside
		// the last, until there is none.
		head.chainLinks.clear();
		int column = whole(head.cellColumn);
		int row = whole(head.cellRow);
		for (;;)
		{
			Object* next = nullptr;
			const auto look = [&](auto& list)
			{
				for (Object& object : list)
				{
					if (next) { return; }
					if (!object.exists || !object.onCells || object.mapName != head.mapName || &object == &head || !isLink(object, head.chain)) { continue; }
					if (std::find(links.begin(), links.end(), &object) != links.end()) { continue; }
					if (std::abs(whole(object.cellColumn) - column) + std::abs(whole(object.cellRow) - row) == 1) { next = &object; }
				}
			};
			look(objects);
			look(spawned);
			if (!next) { break; }
			links.push_back(next);
			head.chainLinks.push_back(next->name);
			column = whole(next->cellColumn);
			row = whole(next->cellRow);
		}
		head.chainKnown = true;
		return links;
	}

	void Game::followChain(Object& head, const std::vector<Object*>& links, int column, int row)
	{
		// Each link into the cell of the one ahead of it.
		for (Object* link : links)
		{
			const int nowColumn = whole(link->cellColumn);
			const int nowRow = whole(link->cellRow);
			putInCell(*link, column, row);
			column = nowColumn;
			row = nowRow;
		}
		if (head.chainGrow <= 0) { return; }

		// Growing: a new link where the tail was.
		--head.chainGrow;
		Object* grown = spawn(head.chain, true);
		if (!grown) { return; }
		moveToMap(*grown, head.mapName);
		putInCell(*grown, column, row);
		head.chainLinks.push_back(grown->name);
	}

	void Game::splitChain(Object& piece)
	{
		if (piece.chain.empty()) { return; }

		// The head it is, or the head it follows.
		Object* head = nullptr;
		std::size_t at = 0;
		const auto lead = [&](auto& list)
		{
			for (Object& object : list)
			{
				if (head || !object.exists || object.chain.empty()) { continue; }
				if (&object == &piece && (object.chainKnown || object.cellGoing)) { head = &object; return; }
				const auto found = std::find(object.chainLinks.begin(), object.chainLinks.end(), piece.name);
				if (found != object.chainLinks.end())
				{
					head = &object;
					at = static_cast<std::size_t>(found - object.chainLinks.begin()) + 1;
					return;
				}
			}
		};
		lead(objects);
		lead(spawned);
		if (!head) { return; }
		chainOf(*head);

		// at: where the piece is, 0 for the head itself.
		const std::vector<std::string> links = head->chainLinks;
		std::vector<std::string> behind(links.begin() + static_cast<std::ptrdiff_t>(std::min(at, links.size())), links.end());
		head->chainLinks.resize(at == 0 ? 0 : at - 1);
		head->chainKnown = true;

		Object* next = behind.empty() ? nullptr : findAnywhere(behind.front());
		if (next)
		{
			next->chainLinks.assign(behind.begin() + 1, behind.end());
			next->chainKnown = true;
			next->chainGrow = 0;
			next->cellGoing = head->cellGoing;
			next->cellGoEvery = head->cellGoEvery;
			next->cellGoWait = head->cellGoWait;
			// The way it was going: toward the cell of the piece cut out,
			// across; or, just turning down or up, the head's way.
			const int across = whole(piece.cellColumn) - whole(next->cellColumn);
			next->cellGoWay = across > 0 ? Direction::Right : across < 0 ? Direction::Left : head->cellGoWay;
			next->facing = next->cellGoWay;
			next->variable = head->variable;
			if (const std::string look = head->lookName(); !look.empty()) { showLookNamed(*next, look); }
		}
		if (&piece == head)
		{
			piece.cellGoing = false;
			piece.chainLinks.clear();
		}
	}

	bool Game::goCells(Object& mover, Direction way, float speed)
	{
		// Never straight back into its own first link.
		const std::vector<Object*> links = chainOf(mover);
		const Vector2f step = stepToward(way, 1.0f);
		if (!links.empty() && whole(links.front()->cellColumn) == whole(mover.cellColumn) + whole(step.x)
			&& whole(links.front()->cellRow) == whole(mover.cellRow) + whole(step.y))
		{
			return false;
		}

		const float framerate = static_cast<float>(windowDesc.framerate > 0 ? windowDesc.framerate : 60);
		mover.cellGoEvery = speed > 0.0f ? std::max(1, whole(framerate / speed)) : 0;
		if (!mover.cellGoing || mover.cellGoWait > mover.cellGoEvery) { mover.cellGoWait = mover.cellGoEvery; }
		mover.cellGoing = speed > 0.0f;
		mover.cellGoWay = way;
		mover.facing = way;
		return true;
	}

	bool Game::putInFreeCell(Object& made, const std::string& map, int below)
	{
		if (!map.empty() && !moveToMap(made, map))
		{
			copyRaws.erase(made.name);
			spawned.pop_back();
			return false;
		}
		// Every cell the whole of it fits in, one of them at random.
		std::vector<std::pair<int, int>> free;
		for (int row = below + 1; row <= made.mapRows; ++row)
		{
			for (int column = 1; column <= made.mapColumns; ++column)
			{
				const bool taken = cellHolder(made.mapName, column, row, { &made })
					|| std::any_of(stepping.begin(), stepping.end(), [&](const Stepping& each) { return each.map == made.mapName && each.column == column && each.row == row; });
				if (!taken) { free.emplace_back(column, row); }
			}
		}
		if (free.empty())
		{
			copyRaws.erase(made.name);
			spawned.pop_back();
			return false;
		}
		const std::size_t pick = std::min(free.size() - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(free.size()))));
		return putOnCells(made, free[pick].first, free[pick].second);
	}

	void Game::slideCells(Object& mover, Direction way)
	{
		const Vector2f step = stepToward(way, 1.0f);
		for (int most = mover.mapColumns + mover.mapRows; most > 0; --most)
		{
			if (!stepCells(mover, whole(step.x), whole(step.y))) { return; }
		}
	}

	bool Game::turnCells(Object& shape, bool clockwise)
	{
		if (!shape.isShape()) { return false; }
		const int size = shape.shapeSize;
		std::vector<Object::ShapeCell> turned = shape.shapeCells;
		for (Object::ShapeCell& cell : turned)
		{
			const int column = cell.column;
			cell.column = clockwise ? size - 1 - cell.row : cell.row;
			cell.row = clockwise ? column : size - 1 - column;
		}

		const std::vector<Object*> cells = cellsOf(shape);
		if (cells.size() != turned.size()) { return false; }
		const int column = whole(shape.cellColumn);
		const int row = whole(shape.cellRow);
		for (const auto& [kickAcross, kickUp] : kicksFor(size, static_cast<int>(shape.shapeCells.size()), shape.quarter, clockwise))
		{
			const int left = column + kickAcross;
			const int top = row - kickUp;
			const bool fits = std::all_of(turned.begin(), turned.end(), [&](const Object::ShapeCell& cell)
				{
					const int c = left + cell.column;
					const int r = top + cell.row;
					return c >= 1 && r >= 1 && c <= shape.mapColumns && r <= shape.mapRows && !cellHolder(shape.mapName, c, r, cells);
				});
			if (!fits) { continue; }

			shape.shapeCells = turned;
			shape.quarter = (shape.quarter + (clockwise ? 1 : 3)) % 4;
			putInCell(shape, left, top);
			for (std::size_t i = 0; i < cells.size(); ++i) { putInCell(*cells[i], left + turned[i].column, top + turned[i].row); }
			return true;
		}
		return false;
	}

	void Game::settle(Object& mover)
	{
		if (!mover.onCells) { return; }
		const std::string map = mover.mapName;
		if (mover.isShape())
		{
			// Cells with <joins> stay joined to one another.
			const std::vector<Object*> cells = cellsOf(mover);
			for (Object* cell : cells)
			{
				cell->shapeOwner.clear();
				if (!cell->hasJoins) { continue; }
				cell->joinedWith.clear();
				for (const Object* other : cells)
				{
					if (other != cell) { cell->joinedWith.push_back(other->name); }
				}
			}
			mover.shapeParts.clear();
			despawn(mover);
		}
		askChecks(map);
	}

	void Game::askChecks(const std::string& map)
	{
		// Every piece of a map carries its checks; any one says them.
		std::vector<Object::CellCheck> checks;
		int columns = 0;
		int rows = 0;
		for (const Object& object : objects)
		{
			if (object.mapName == map && object.onCells)
			{
				checks = object.cellChecks;
				columns = object.mapColumns;
				rows = object.mapRows;
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
					if (!holder || !holder->shapeOwner.empty() || (!check.objClass.empty() && holder->objClass != check.objClass)) { break; }
					match.pieces.push_back(holder->name);
				}
				if (static_cast<int>(match.pieces.size()) != columns)
				{
					--row;
					continue;
				}

				for (const Command& command : check.commands)
				{
					if (!executor.executeCheck(command, match)) { break; }
				}
				// Still full (nothing cleared it): on up.
				const bool stillFull = std::all_of(match.pieces.begin(), match.pieces.end(), [&](const std::string& name)
					{
						const Object* piece = findAnywhere(name);
						return piece && piece->exists && whole(piece->cellRow) == row;
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

	void Game::askLines(const Object::CellCheck& check, const std::string& map, int columns, int rows, CommandExecutor& executor)
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
				if (!holder || !holder->shapeOwner.empty() || (!check.objClass.empty() && holder->objClass != check.objClass)) { return nullptr; }
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
				if (!executor.executeCheck(command, match)) { break; }
			}
			const bool taken = std::any_of(match.pieces.begin(), match.pieces.end(), [&](const std::string& name)
				{
					const Object* piece = findAnywhere(name);
					return !piece || !piece->exists;
				});
			if (!taken) { return; }
		}
	}

	void Game::fallCells(const std::string& map, const std::string& objClass)
	{
		const Object* any = nullptr;
		for (const Object& object : objects)
		{
			if (object.onCells && object.mapName == map) { any = &object; break; }
		}
		if (!any) { return; }
		const int columns = any->mapColumns;
		const int rows = any->mapRows;

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
					if (!holder || !holder->shapeOwner.empty() || (!objClass.empty() && holder->objClass != objClass) || std::find(done.begin(), done.end(), holder) != done.end()) { continue; }
					std::vector<Object*> together{ holder };
					for (const std::string& name : holder->joinedWith)
					{
						Object* other = findAnywhere(name);
						if (other && other->exists && other->onCells && other->mapName == map && other->shapeOwner.empty() && (objClass.empty() || other->objClass == objClass)
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
								const int below = whole(piece->cellRow) + by;
								return below <= rows && !cellHolder(map, whole(piece->cellColumn), below, together);
							});
					};
					while (fits(drop + 1)) { ++drop; }
					if (drop == 0) { continue; }
					for (Object* piece : together) { putInCell(*piece, whole(piece->cellColumn), whole(piece->cellRow) + drop); }
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
			if (!piece.hasJoins || !piece.exists || !piece.onCells) { return; }
			const auto& looks = piece.joinLooks;
			if (std::find(looks.begin(), looks.end(), piece.look) == looks.end()) { return; }

			// Falling, the other cells of its shape; settled, those it came with.
			std::vector<std::string> partners = piece.joinedWith;
			if (!piece.shapeOwner.empty())
			{
				const Object* shape = findAnywhere(piece.shapeOwner);
				partners = shape ? shape->shapeParts : std::vector<std::string>{};
			}
			Object::Join way = Object::Join::Alone;
			for (const std::string& name : partners)
			{
				const Object* other = findAnywhere(name);
				if (!other || other == &piece || !other->exists || other->mapName != piece.mapName) { continue; }
				const int across = whole(other->cellColumn) - whole(piece.cellColumn);
				const int down = whole(other->cellRow) - whole(piece.cellRow);
				if (std::abs(across) + std::abs(down) != 1) { continue; }
				way = across < 0 ? Object::Join::Left : across > 0 ? Object::Join::Right : down < 0 ? Object::Join::Up : Object::Join::Down;
				break;
			}
			const std::size_t look = looks[static_cast<std::size_t>(way)];
			if (look != piece.look) { piece.showLook(look); }
		};
		for (Object& object : objects) { join(object); }
	}

	void Game::cycleCells(Object& shape)
	{
		if (!shape.isShape()) { return; }
		const std::vector<Object*> cells = cellsOf(shape);
		const std::size_t count = cells.size();
		if (count < 2 || count != shape.shapeCells.size()) { return; }

		// The cell at place i goes to place i + 1; the places stay.
		std::vector<std::string> parts(count);
		std::vector<std::string> pieces(count);
		for (std::size_t i = 0; i < count; ++i)
		{
			parts[(i + 1) % count] = shape.shapeParts[i];
			pieces[(i + 1) % count] = shape.shapeCells[i].piece;
		}
		const int column = whole(shape.cellColumn);
		const int row = whole(shape.cellRow);
		for (std::size_t i = 0; i < count; ++i)
		{
			shape.shapeParts[i] = parts[i];
			shape.shapeCells[i].piece = pieces[i];
			if (Object* cell = findAnywhere(parts[i])) { putInCell(*cell, column + shape.shapeCells[i].column, row + shape.shapeCells[i].row); }
		}
	}

	void Game::clearCells(const Object::CellMatch& match)
	{
		for (const std::string& name : match.pieces)
		{
			if (Object* piece = findAnywhere(name)) { despawn(*piece); }
		}
	}

	void Game::collapseCells(const Object::CellMatch& match)
	{
		const auto down = [&](Object& object)
		{
			if (object.exists && object.onCells && !object.isShape() && object.shapeOwner.empty() && object.mapName == match.map && whole(object.cellRow) < match.row)
			{
				putInCell(object, whole(object.cellColumn), whole(object.cellRow) + 1);
			}
		};
		for (Object& object : objects) { down(object); }
		for (Object& object : spawned) { down(object); }
	}

	void Game::holdCellMove(Object& object, const CmdMove& move, bool pressed)
	{
		if (!pressed)
		{
			if (object.cellHeld && object.cellHeldWay == move.direction) { object.cellHeld = false; }
			return;
		}

		const float framerate = static_cast<float>(windowDesc.framerate > 0 ? windowDesc.framerate : 60);
		object.cellHeld = move.step > 0.0f;
		object.cellHeldWay = move.direction;
		object.cellHeldEvery = move.step > 0.0f ? std::max(1, whole(framerate / move.step)) : 0;
		object.cellHeldWait = move.wait > 0.0f ? std::max(1, whole(move.wait * framerate)) : object.cellHeldEvery;

		const Vector2f step = stepToward(move.direction, 1.0f);
		stepCells(object, whole(step.x), whole(step.y));
	}

	void Game::cellMoves()
	{
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			Object& object = objects[i];
			if (!object.exists || !isShown(object) || object.awayFromHome()) { continue; }
			if (object.cellGoing && --object.cellGoWait <= 0)
			{
				object.cellGoWait = object.cellGoEvery;
				const Vector2f step = stepToward(object.cellGoWay, 1.0f);
				stepCells(objects[i], whole(step.x), whole(step.y));
			}
			if (!objects[i].cellHeld || --objects[i].cellHeldWait > 0) { continue; }
			objects[i].cellHeldWait = objects[i].cellHeldEvery;
			const Vector2f step = stepToward(objects[i].cellHeldWay, 1.0f);
			stepCells(objects[i], whole(step.x), whole(step.y));
		}
	}

	const Object::MoveSet* Game::moveSetOf(const Object& piece) const noexcept
	{
		const Object::MoveSet* any = nullptr;
		for (const Object::MoveSet& set : piece.moveSets)
		{
			if (set.look && *set.look == piece.look) { return &set; }
			if (!set.look && !any) { any = &set; }
		}
		return any;
	}

	std::vector<Game::PieceMoveTo> Game::movesOf(Object& piece, bool jumpsOnly)
	{
		std::vector<PieceMoveTo> found;
		const Object::MoveSet* set = moveSetOf(piece);
		if (!set || !piece.onCells || !piece.exists) { return found; }
		const int column = whole(piece.cellColumn);
		const int row = whole(piece.cellRow);
		const auto onMap = [&](int c, int r) { return c >= 1 && r >= 1 && c <= piece.mapColumns && r <= piece.mapRows; };
		for (const Object::PieceMove& move : set->moves)
		{
			if (jumpsOnly && !move.jump) { continue; }
			const int by = move.jump ? 2 : 1;
			const int toColumn = column + move.x * by;
			const int toRow = row + move.y * by;
			if (!onMap(toColumn, toRow) || cellHolder(piece.mapName, toColumn, toRow, { &piece })) { continue; }
			if (!move.jump)
			{
				found.push_back({ toColumn, toRow, nullptr });
				continue;
			}
			Object* over = cellHolder(piece.mapName, column + move.x, row + move.y, { &piece });
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
				if (other.moveSets.empty() || other.objClass != piece.objClass || other.mapName != piece.mapName || !isShown(other)) { continue; }
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
			if (piece.moveSets.empty()) { continue; }
			if (!isShown(piece)) { piece.moveCount = 0.0f; continue; }
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
			piece.moveCount = count;
		}
	}

	Object* Game::putTemplate(const std::string& objClass)
	{
		const std::vector<Object*> all = putTemplates(objClass);
		return all.empty() ? nullptr : all.front();
	}

	std::vector<Object*> Game::putTemplates(const std::string& objClass)
	{
		// One of each group: pieces of a group are alike.
		std::vector<Object*> found;
		std::vector<std::string> groups;
		for (std::size_t i = 0; i < loadedCount && i < objects.size(); ++i)
		{
			Object& object = objects[i];
			if (object.objClass != objClass || !object.onCells || object.isShape()) { continue; }
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
		const auto onMap = [&](int c, int r) { return c >= 1 && r >= 1 && c <= kind.mapColumns && r <= kind.mapRows; };
		if (!onMap(column, row)) { return trapped; }
		const Object::MoveSet* set = moveSetOf(kind);
		// On a piece of a class it goes on (which it takes the place of), or
		// else in a free cell.
		Object* under = cellHolder(kind.mapName, column, row, {});
		if (set && !set->on.empty())
		{
			if (!under || std::find(set->on.begin(), set->on.end(), under->objClass) == set->on.end()) { return trapped; }
		}
		else if (under) { return trapped; }
		// Falling: only onto the floor or onto a piece.
		if (set && set->fall && row < kind.mapRows && !cellHolder(kind.mapName, column, row + 1, {})) { return trapped; }
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
						Object* there = cellHolder(kind.mapName, c, r, {});
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
		for (int row = 1; row <= kind.mapRows; ++row)
		{
			for (int column = 1; column <= kind.mapColumns; ++column)
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
		if (!picker.onCells || !picker.picked.empty()) { return false; }
		const int column = whole(picker.cellColumn);
		// The first piece of the class that may go there.
		Object* kind = nullptr;
		int row = 0;
		bool can = false;
		std::vector<Object*> trapped;
		for (Object* each : putTemplates(put.objClass))
		{
			row = whole(picker.cellRow);
			// Falling: wherever the picker is in its column, to the lowest free cell.
			if (const Object::MoveSet* set = moveSetOf(*each); set && set->fall)
			{
				row = 0;
				for (int at = each->mapRows; at >= 1 && row == 0; --at)
				{
					if (!cellHolder(each->mapName, column, at, {})) { row = at; }
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
		const std::string name = kind->name;
		const std::string map = kind->mapName;
		const int columns = kind->mapColumns;
		const int rows = kind->mapRows;
		if (Object* under = cellHolder(map, column, row, {})) { despawn(*under); }
		std::vector<std::pair<int, int>> cells{ { column, row } };
		for (Object* each : trapped)
		{
			cells.emplace_back(whole(each->cellColumn), whole(each->cellRow));
			despawn(*each);
		}
		for (const auto& [c, r] : cells)
		{
			if (Object* made = spawn(name)) { putInCell(*made, c, r); }
		}

		// <close>: each piece of that class beside it now walled in, no place
		// left beside it that a piece could be put <on> (lines of either
		// side: the slots are gone), is taken by a copy of the object named.
		bool closed = false;
		if (!put.closeClass.empty())
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
				if (!box || box->objClass != put.closeClass) { continue; }
				if (!put_(c + 1, r) || !put_(c - 1, r) || !put_(c, r + 1) || !put_(c, r - 1)) { continue; }
				despawn(*box);
				if (Object* made = spawn(put.closeObject)) { putInCell(*made, c, r); }
				closed = true;
			}
		}
		picker.moveFromColumn = column;
		picker.moveFromRow = row;
		picker.moveToColumn = column;
		picker.moveToRow = row;
		// The map's checks asked about what it has now (a line made).
		askChecks(map);
		countMoves();
		// Something closed: the turn goes on, as after a jump that must be
		// followed; what is written after the put is not done.
		return !closed;
	}

	float Game::countClass(const std::string& objClass)
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

	bool Game::pickPiece(Object& picker, const std::string& objClass, bool only)
	{
		if (picker.pickChain || !picker.onCells) { return false; }
		if (only)
		{
			Object* one = nullptr;
			for (Object& piece : objects)
			{
				if (piece.moveSets.empty() || piece.objClass != objClass || !piece.onCells || !isShown(piece) || legalMoves(piece, false).empty()) { continue; }
				if (one) { return false; }
				one = &piece;
			}
			if (!one || picker.picked == one->name) { return false; }
			picker.picked = one->name;
			return true;
		}
		const int column = whole(picker.cellColumn);
		const int row = whole(picker.cellRow);
		for (Object& piece : objects)
		{
			if (&piece == &picker || piece.moveSets.empty() || piece.objClass != objClass || !piece.onCells || !isShown(piece)
				|| whole(piece.cellColumn) != column || whole(piece.cellRow) != row)
			{
				continue;
			}
			if (legalMoves(piece, false).empty()) { return false; }
			picker.picked = piece.name;
			return true;
		}
		return false;
	}

	bool Game::placePiece(Object& picker)
	{
		if (picker.picked.empty() || !picker.onCells) { return false; }
		Object* piece = findAnywhere(picker.picked);
		if (!piece || !piece->exists || !isShown(*piece))
		{
			picker.picked.clear();
			picker.pickChain = false;
			return false;
		}
		const int column = whole(picker.cellColumn);
		const int row = whole(picker.cellRow);
		const std::vector<PieceMoveTo> moves = legalMoves(*piece, picker.pickChain);
		const auto move = std::find_if(moves.begin(), moves.end(), [&](const PieceMoveTo& each) { return each.column == column && each.row == row; });
		if (move == moves.end()) { return false; }

		const Object::MoveSet* set = moveSetOf(*piece);
		const bool jumped = move->taken != nullptr;
		if (!picker.pickChain)
		{
			picker.moveFromColumn = whole(piece->cellColumn);
			picker.moveFromRow = whole(piece->cellRow);
		}
		picker.moveToColumn = column;
		picker.moveToRow = row;
		if (move->taken) { despawn(*move->taken); }
		putInCell(*piece, column, row);

		// Promoted on reaching its row: that ends its move.
		bool promoted = false;
		if (set && set->promoteLook && row == set->promoteRow && piece->look != *set->promoteLook)
		{
			piece->showLook(*set->promoteLook);
			promoted = true;
		}

		// A jump that can be followed by another must be: the turn goes on.
		if (jumped && !promoted && !movesOf(*piece, true).empty())
		{
			picker.pickChain = true;
			countMoves();
			return false;
		}
		picker.picked.clear();
		picker.pickChain = false;
		countMoves();
		return true;
	}

	bool Game::dropPiece(Object& picker)
	{
		if (picker.picked.empty() || picker.pickChain) { return false; }
		picker.picked.clear();
		return true;
	}

	// The cells holding a piece in play of one of these classes.
	std::vector<std::pair<int, int>> Game::ontoCells(const std::vector<std::string>& onto)
	{
		std::vector<std::pair<int, int>> cells;
		for (const Object& object : objects)
		{
			if (!object.onCells || !isShown(object)) { continue; }
			if (std::find(onto.begin(), onto.end(), object.objClass) == onto.end()) { continue; }
			cells.emplace_back(whole(object.cellColumn), whole(object.cellRow));
		}
		return cells;
	}

	// <hop direction="right"><onto class="hslot" /></hop>: left and right,
	// to the nearest such cell along the row; up and down, to the nearest
	// row ahead that has one, the cell in it nearest the column kept (the
	// one the first of these hops started from, as a text cursor keeps
	// its column). Refused when there is none that way.
	bool Game::hopOnto(Object& mover, Direction direction, const std::vector<std::string>& onto)
	{
		const int column = whole(mover.cellColumn);
		const int row = whole(mover.cellRow);
		const bool upDown = direction == Direction::Up || direction == Direction::Down;
		const int ahead = direction == Direction::Down || direction == Direction::Right ? 1 : -1;
		if (!upDown || mover.keptAt != std::pair<int, int>{ column, row }) { mover.keptColumn = column; }

		std::optional<std::pair<int, int>> best;
		int bestAlong = 0;
		int bestAcross = 0;
		for (const auto& [c, r] : ontoCells(onto))
		{
			if (!upDown && r != row) { continue; }
			const int along = upDown ? (r - row) * ahead : (c - column) * ahead;
			const int across = upDown ? std::abs(c - mover.keptColumn) : 0;
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
		mover.keptAt = upDown ? *best : std::pair<int, int>{};
		return true;
	}

	void Game::trackCells(Object& object, bool horizontal, float at, const std::vector<std::string>& onto)
	{
		// <onto>: the cell nearest where it points, of those holding such a
		// piece, while it points over the map.
		if (!onto.empty())
		{
			(horizontal ? object.pointer.x : object.pointer.y) = at;
			const auto middle = [&](int column, int row)
			{
				return Vector2f{ object.mapOrigin.x + (static_cast<float>(column) - 0.5f) * object.mapTile.x,
					object.mapOrigin.y + (static_cast<float>(row) - 0.5f) * object.mapTile.y };
			};
			Vector2f point = object.pointer;
			const Vector2f here = middle(whole(object.cellColumn), whole(object.cellRow));
			if (point.x < 0.0f) { point.x = here.x; }
			if (point.y < 0.0f) { point.y = here.y; }
			if (point.x < object.mapOrigin.x || point.y < object.mapOrigin.y
				|| point.x > object.mapOrigin.x + object.mapTile.x * static_cast<float>(object.mapColumns)
				|| point.y > object.mapOrigin.y + object.mapTile.y * static_cast<float>(object.mapRows)) { return; }
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
			if (best && (best->first != whole(object.cellColumn) || best->second != whole(object.cellRow))) { putInCell(object, best->first, best->second); }
			return;
		}
		const float from = horizontal ? object.mapOrigin.x : object.mapOrigin.y;
		const float tile = horizontal ? object.mapTile.x : object.mapTile.y;
		if (tile <= 0.0f || at < from) { return; }
		const int cell = static_cast<int>((at - from) / tile) + 1;
		if (cell > (horizontal ? object.mapColumns : object.mapRows)) { return; }
		const int column = horizontal ? cell : whole(object.cellColumn);
		const int row = horizontal ? whole(object.cellRow) : cell;
		if (column != whole(object.cellColumn) || row != whole(object.cellRow)) { putInCell(object, column, row); }
	}

	void Game::showMarks()
	{
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			// Not in play now (a pause over it): its marks stay as they are.
			if (objects[i].markSets.empty() || !isShown(objects[i])) { continue; }
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
				Object* held = picker.picked.empty() ? nullptr : findAnywhere(picker.picked);
				for (const Object::MarkSet& set : picker.markSets)
				{
					if (set.look && *set.look != picker.look) { continue; }
					for (const Object::Mark& mark : set.marks)
					{
						if (!on(mark.variable)) { continue; }
						switch (mark.kind)
						{
						case Object::Mark::Kind::Held:
							if (held) { want.push_back({ mark.object, whole(held->cellColumn), whole(held->cellRow) }); }
							break;
						case Object::Mark::Kind::Moves:
							if (held)
							{
								for (const PieceMoveTo& move : legalMoves(*held, picker.pickChain)) { want.push_back({ mark.object, move.column, move.row }); }
							}
							break;
						case Object::Mark::Kind::Movable:
							for (Object& piece : objects)
							{
								if (piece.moveSets.empty() || piece.objClass != mark.objClass || !piece.onCells || !isShown(piece) || legalMoves(piece, false).empty()) { continue; }
								want.push_back({ mark.object, whole(piece.cellColumn), whole(piece.cellRow) });
							}
							break;
						case Object::Mark::Kind::Put:
							if (const Object* kind = putTemplate(mark.objClass))
							{
								const CellsStill still(*this);
								for (const auto& [column, row] : putCells(*kind)) { want.push_back({ mark.object, column, row }); }
							}
							break;
						case Object::Mark::Kind::Last:
							if (picker.moveToColumn > 0)
							{
								want.push_back({ mark.object, picker.moveFromColumn, picker.moveFromRow });
								// A piece put: one cell.
								if (picker.moveToColumn != picker.moveFromColumn || picker.moveToRow != picker.moveFromRow)
								{
									want.push_back({ mark.object, picker.moveToColumn, picker.moveToRow });
								}
							}
							break;
						}
					}
				}
			}
			if (want == objects[i].marked) { continue; }

			// Made again: the old copies out, a new one in each cell marked.
			std::vector<std::string> old = std::move(objects[i].markCopies);
			objects[i].markCopies.clear();
			objects[i].marked = want;
			for (const std::string& copy : old) { despawn(copy); }
			std::vector<std::string> made;
			for (const Object::Marked& mark : want)
			{
				Object* copy = spawn(mark.object);
				if (!copy || !copy->onCells) { continue; }
				putInCell(*copy, mark.column, mark.row);
				made.push_back(copy->name);
			}
			objects[i].markCopies = std::move(made);
		}
	}

	bool Game::shoveCells(Object& piece, Edge way)
	{
		if (!piece.onCells || !piece.exists) { return false; }
		// A shape's cell is shoved with its shape.
		Object* shoved = &piece;
		if (!piece.shapeOwner.empty())
		{
			if (Object* owner = tryGetObject(piece.shapeOwner)) { shoved = owner; }
		}
		const int across = way == Edge::Left ? -1 : way == Edge::Right ? 1 : 0;
		const int down = way == Edge::Top ? -1 : way == Edge::Bottom ? 1 : 0;
		// Its own step is asked about in the middle of the mover's.
		const bool through = goThrough;
		const bool moved = stepCells(*shoved, across, down);
		goThrough = through;
		return moved;
	}

	bool Game::mergeCells(Object& mover, Object& other, const std::string& variable)
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

	bool Game::slideClass(const std::string& objClass, Direction way)
	{
		std::vector<Object*> pieces;
		for (Object& object : objects)
		{
			if (object.objClass == objClass && object.exists && object.onCells && !object.isShape() && !object.awayFromHome() && isShown(object)) { pieces.push_back(&object); }
		}
		// The one nearest that way first, so the rest slide up behind it.
		const auto key = [way](const Object* piece)
		{
			const int column = whole(piece->cellColumn);
			const int row = whole(piece->cellRow);
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
			const int column = whole(piece->cellColumn);
			const int row = whole(piece->cellRow);
			slideCells(*piece, way);
			moved = moved || !piece->exists || whole(piece->cellColumn) != column || whole(piece->cellRow) != row;
		}
		merged.clear();
		return moved;
	}

	std::vector<Object*> Game::piecesAbout(const Object& piece, const std::string& objClass, const CellPlace& place)
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
			return std::abs(other.mapOrigin.x - piece.mapOrigin.x) < 0.5f && std::abs(other.mapOrigin.y - piece.mapOrigin.y) < 0.5f
				&& std::abs(other.mapTile.x - piece.mapTile.x) < 0.5f && std::abs(other.mapTile.y - piece.mapTile.y) < 0.5f;
		};
		const auto alike = [&](const Object& other)
		{
			if (place.alike.empty()) { return true; }
			const auto mine = piece.variable.find(place.alike);
			const auto theirs = other.variable.find(place.alike);
			return mine != piece.variable.end() && theirs != other.variable.end() && mine->second == theirs->second;
		};
		const int column = whole(piece.cellColumn);
		const int row = whole(piece.cellRow);
		const auto look = [&](Object& other, bool joined, int wantColumn, int wantRow)
		{
			if (&other == &piece || !other.exists || !other.onCells || other.isShape() || other.awayFromHome()) { return; }
			if (!joined && !isShown(other)) { return; }
			if (!objClass.empty() && other.objClass != objClass) { return; }
			if (whole(other.cellColumn) != wantColumn || whole(other.cellRow) != wantRow || !sameLayout(other) || !alike(other)) { return; }
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
			kept.push_back({ object.name, static_cast<const Moment&>(object), object.look });
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
			if (!was || was->moment.exists != object.exists || was->moment.cellColumn != object.cellColumn || was->moment.cellRow != object.cellRow)
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
			object.placed = true;
			if (object.look != was->look && was->look < object.looks.size()) { object.showLook(was->look); }
			for (const auto& [variable, value] : was->moment.variable) { setVariable(object.name, variable, value); }
			holdVariablesForExpressions(object);
		}
		stageChanged();
		return true;
	}
}
