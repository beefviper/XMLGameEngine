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
		// any other turns where it is or not at all.
		using Kicks = std::array<std::pair<int, int>, 5>;
		const Kicks& kicksFor(int size, int from, bool clockwise)
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
			static const Kicks none{ { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } } };

			const int turn = clockwise ? from * 2 : (from + 3) % 4 * 2 + 1;
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
			for (Object* cell : cells) { stepping.push_back({ mover.mapName, whole(cell->cellColumn) + across, whole(cell->cellRow) + down }); }
			Touch touch;
			touch.edge = edge;
			touch.normal = outOf(opposite(edge));
			touch.along = 0.5f;
			touch.point = middleOf(*blocker);
			reactTo(mover, *blocker, touch);
			Touch back = touch;
			back.edge = opposite(edge);
			back.normal = outOf(edge);
			reactTo(*blocker, mover, back);
			stepping.clear();

			// A touch that took what was in the way away (food eaten) lets
			// the step go on.
			if (inTheWay(offMap) || offMap || !mover.exists) { return false; }
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

		// As kept, while every link is still there.
		std::vector<Object*> links;
		for (const std::string& name : head.chainLinks)
		{
			Object* link = findAnywhere(name);
			if (!link || !link->exists) { links.clear(); break; }
			links.push_back(link);
		}
		if (!links.empty()) { return links; }

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

	bool Game::putInFreeCell(Object& made, const std::string& map)
	{
		if (!map.empty() && !moveToMap(made, map))
		{
			copyRaws.erase(made.name);
			spawned.pop_back();
			return false;
		}
		// Every cell the whole of it fits in, one of them at random.
		std::vector<std::pair<int, int>> free;
		for (int row = 1; row <= made.mapRows; ++row)
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
		for (const auto& [kickAcross, kickUp] : kicksFor(size, shape.quarter, clockwise))
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
			for (Object* cell : cellsOf(mover)) { cell->shapeOwner.clear(); }
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
			const auto alike = [&](const Object* a, const Object* b) { return a && b && kindOf(*a) == kindOf(*b); };

			std::vector<Object*> found;
			const int ways[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { -1, 1 } };
			for (int row = 1; row <= rows; ++row)
			{
				for (int column = 1; column <= columns; ++column)
				{
					Object* start = loose(column, row);
					if (!start) { continue; }
					for (const auto& way : ways)
					{
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

	void Game::fallCells(const std::string& map)
	{
		const Object* any = nullptr;
		for (const Object& object : objects)
		{
			if (object.onCells && object.mapName == map) { any = &object; break; }
		}
		if (!any) { return; }
		const int columns = any->mapColumns;
		const int rows = any->mapRows;
		for (int column = 1; column <= columns; ++column)
		{
			// From the bottom up: each loose piece to the lowest free cell
			// under it; anything else (a shape's cell) stays, and stops it.
			int lowest = rows;
			for (int row = rows; row >= 1; --row)
			{
				Object* holder = cellHolder(map, column, row, {});
				if (!holder) { continue; }
				if (holder->shapeOwner.empty())
				{
					if (row != lowest) { putInCell(*holder, column, lowest); }
					--lowest;
				}
				else { lowest = row - 1; }
			}
		}
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
}
