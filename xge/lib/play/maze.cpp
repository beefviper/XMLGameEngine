// maze.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026

#include "play/maze.h"

#include "play/geometry.h"
#include "game/random.h"
#include "play/verbs.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <string>

namespace xge
{
	namespace
	{
		// The four ways in the order a tie between them goes to the first, as
		// the arcade's ghosts break one: up, left, down, right.
		constexpr Direction ways[]{ Direction::Up, Direction::Left, Direction::Down, Direction::Right };

		Direction opposite(Direction way) noexcept
		{
			switch (way)
			{
			case Direction::Up: return Direction::Down;
			case Direction::Down: return Direction::Up;
			case Direction::Left: return Direction::Right;
			default: return Direction::Left;
			}
		}

		Vector2f unit(Direction way) noexcept { return stepToward(way, 1.0f); }

		float distanceSquared(const Vector2f& a, const Vector2f& b) noexcept
		{
			const Vector2f apart = a - b;
			return apart.x * apart.x + apart.y * apart.y;
		}

		// A map's tiles, and the objects on each (by index), as this frame
		// finds them.
		using Grid = MazeGrid;

		// One walker's frame.
		class Walker
		{
		public:
			Walker(Object& who, std::vector<Object>& everything, const Grid& map, FrameRules& frameRules)
				: walker(who), objects(everything), grid(map), rules(frameRules)
			{
			}

			void walk()
			{
				Walk& walk = walker.walk;
				const Vector2f size = sizeOf(walker);
				const Vector2f middle = walker.position + size * 0.5f;
				const int column = static_cast<int>(std::floor((middle.x - grid.origin.x) / grid.tile.x));
				const int row = static_cast<int>(std::floor((middle.y - grid.origin.y) / grid.tile.y));
				const bool onMap = grid.onMap(column, row);
				speed = onMap ? walk.speed * shareAt(column, row) : walk.speed;

				// Standing still: on a tile, it starts from the middle of it,
				// any way open (a <go>: only the way wished).
				if (walker.velocity.x == 0.0f && walker.velocity.y == 0.0f)
				{
					if (!onMap) { return; }
					walker.position = grid.middle(column, row) - size * 0.5f;
					if (const auto way = choose(column, row, walk.going, false))
					{
						setOff(*way, column, row);
					}
					return;
				}

				const Direction going = directionOf(walker.velocity);
				if (going != walk.going)
				{
					// Turned by something else (a <reverse />): it chooses
					// again at the next tile.
					walk.chose = false;
					walk.going = going;
				}

				// In the middle of its lane.
				const Vector2f lane = grid.middle(column, row);
				if (going == Direction::Left || going == Direction::Right)
				{
					if (row >= 0 && row < grid.rows) { walker.position.y = lane.y - size.y * 0.5f; }
				}
				else if (column >= 0 && column < grid.columns)
				{
					walker.position.x = lane.x - size.x * 0.5f;
				}

				// Pushed back the way it came: at once, wherever it is.
				if (walk.mode == Walk::Mode::Go && walk.wish == opposite(going))
				{
					head(walk.wish);
					walk.chose = false;
					return;
				}

				if (!onMap || (walk.chose && walk.chosenColumn == column && walk.chosenRow == row))
				{
					head(going);
					return;
				}

				// It chooses in the frame that would take it to (or past) the
				// middle of the tile.
				constexpr float slack = 0.01f;
				const float ahead = (lane.x - middle.x) * unit(going).x + (lane.y - middle.y) * unit(going).y;
				if (ahead < -slack || ahead >= speed - slack)
				{
					head(going);
					return;
				}

				walk.chose = true;
				walk.chosenColumn = column;
				walk.chosenRow = row;

				const auto way = choose(column, row, going, true);
				if (!way)
				{
					// A wall ahead and no way it may take: it stops in the middle.
					walker.position = lane - size * 0.5f;
					walker.velocity = {};
					return;
				}
				if (*way != going)
				{
					// Round the corner exactly at the middle: as far back along
					// the new way as it was short of the middle, so this frame's
					// move goes the whole speed.
					walker.position = lane - size * 0.5f - unit(*way) * ahead;
				}
				head(*way);
			}

		private:
			Object& walker;
			std::vector<Object>& objects;
			const Grid& grid;
			FrameRules& rules;

			// The nearest in play of that name or group, other than the walker.
			const Object* nearest(const std::string& name) const
			{
				return nearestInPlay(objects, walker, name, [this](const Object& other) { return rules.inPlay(other); });
			}

			// Its speed this frame: what it was told, times the share of a
			// <slow> for the tile it is on.
			float speed{};

			// The share of its speed on this tile: the first of its <slow>s
			// whose class something in play on the tile has, or the whole.
			float shareAt(int column, int row) const
			{
				if (walker.mazeSlows.empty()) { return 1.0f; }
				const auto& here = grid.at[static_cast<std::size_t>(row) * static_cast<std::size_t>(grid.columns) + static_cast<std::size_t>(column)];
				for (const MazeSlow& slow : walker.mazeSlows)
				{
					for (const std::size_t index : here)
					{
						if (objects[index].objClass == slow.objClass && rules.inPlay(objects[index])) { return slow.share; }
					}
				}
				return 1.0f;
			}

			// Whether a tile holds a wall to this walker.
			bool blocked(int column, int row) const
			{
				const auto& here = grid.at[static_cast<std::size_t>(row) * static_cast<std::size_t>(grid.columns) + static_cast<std::size_t>(column)];
				for (const std::size_t index : here)
				{
					const Object& other = objects[index];
					for (const MazeWall& wall : walker.mazeWalls)
					{
						if (other.objClass == wall.objClass && (wall.sprite.empty() || wall.sprite == walker.lookName()) && rules.inPlay(other))
						{
							return true;
						}
					}
				}
				return false;
			}

			// The way it may take from this tile: `moving` that way (a stopped
			// walker has none), never back, and off the map only straight on.
			bool open(int column, int row, Direction way, Direction going, bool moving) const
			{
				if (moving && way == opposite(going)) { return false; }
				const Vector2f step = unit(way);
				const int nextColumn = column + static_cast<int>(step.x);
				const int nextRow = row + static_cast<int>(step.y);
				if (!grid.onMap(nextColumn, nextRow)) { return moving && way == going; }
				return !blocked(nextColumn, nextRow);
			}

			// Where its target is, for a <hunt>; none when what it hunts is
			// not in play.
			std::optional<Vector2f> target() const
			{
				const CmdHunt& hunt = walker.walk.hunt;
				const Object* quarry = nearest(hunt.target);
				if (!quarry) { return std::nullopt; }

				const Vector2f there = middleOf(*quarry);
				if (!hunt.retreatTo.empty() && distanceSquared(middleOf(walker), there) < hunt.retreatWithin * grid.tile.x * hunt.retreatWithin * grid.tile.x)
				{
					if (const Object* corner = nearest(hunt.retreatTo)) { return middleOf(*corner); }
				}

				Vector2f point = there + unit(quarry->facing) * (hunt.ahead * grid.tile.x);
				if (!hunt.partner.empty())
				{
					if (const Object* partner = nearest(hunt.partner))
					{
						point = point * 2.0f - middleOf(*partner);
					}
				}
				return point;
			}

			// Its way from this tile, by its <go>, <hunt> or <wander>; none
			// when it stops. A hunter or wanderer at a dead end turns round.
			std::optional<Direction> choose(int column, int row, Direction going, bool moving) const
			{
				const Walk& walk = walker.walk;
				if (walk.mode == Walk::Mode::Go)
				{
					if (open(column, row, walk.wish, going, moving)) { return walk.wish; }
					if (moving && open(column, row, going, going, moving)) { return going; }
					return std::nullopt;
				}

				std::vector<Direction> choices;
				for (const Direction way : ways)
				{
					if (open(column, row, way, going, moving)) { choices.push_back(way); }
				}
				if (choices.empty())
				{
					if (moving && open(column, row, opposite(going), going, false)) { return opposite(going); }
					return std::nullopt;
				}

				if (walk.mode == Walk::Mode::Wander)
				{
					const auto count = static_cast<float>(choices.size());
					const auto pick = static_cast<std::size_t>(std::floor(randomBetween(0.0f, count)));
					return choices[std::min(pick, choices.size() - 1)];
				}

				const std::optional<Vector2f> point = target();
				if (!point)
				{
					for (const Direction way : choices)
					{
						if (moving && way == going) { return way; }
					}
					return choices.front();
				}

				std::optional<Direction> best;
				float bestDistance = 0.0f;
				for (const Direction way : choices)
				{
					const Vector2f step = unit(way);
					const float distance = distanceSquared(grid.middle(column + static_cast<int>(step.x), row + static_cast<int>(step.y)), *point);
					if (!best || distance < bestDistance)
					{
						best = way;
						bestDistance = distance;
					}
				}
				return best;
			}

			// Going that way at its speed, facing it.
			void head(Direction way)
			{
				walker.velocity = unit(way) * speed;
				walker.walk.going = way;
				face(way);
			}

			void setOff(Direction way, int column, int row)
			{
				walker.walk.chose = true;
				walker.walk.chosenColumn = column;
				walker.walk.chosenRow = row;
				head(way);
			}

			void face(Direction way)
			{
				walker.facing = way;
				if (walker.hasHeading)
				{
					walker.heading = headingOf(unit(way));
					walker.showHeading();
				}
			}
		};
	}

	void startGo(Object& walker, const CmdGo& go)
	{
		walker.walk.mode = Walk::Mode::Go;
		walker.walk.speed = go.speed;
		walker.walk.wish = go.direction;
	}

	void startHunt(Object& walker, const CmdHunt& hunt)
	{
		walker.walk.mode = Walk::Mode::Hunt;
		walker.walk.speed = hunt.speed;
		walker.walk.hunt = hunt;
	}

	void startWander(Object& walker, const CmdWander& wander)
	{
		walker.walk.mode = Walk::Mode::Wander;
		walker.walk.speed = wander.speed;
	}

	void walkMazes(std::vector<Object>& objects, FrameRules& rules)
	{
		for (Object& object : objects)
		{
			if (!object.walksMaze() || object.walk.mode == Walk::Mode::None || object.isFollowing() || object.isAirborne() || !rules.inPlay(object))
			{
				continue;
			}

			const Grid& grid = rules.mazeGrid(objects, object.mazeMap);
			if (grid.at.empty() || !(grid.tile.x > 0.0f) || !(grid.tile.y > 0.0f)) { continue; }

			Walker(object, objects, grid, rules).walk();
		}
	}
}
