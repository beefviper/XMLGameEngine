# Groups

**Status:** Built: listed members, or cells in columns and rows. `<grid>` was folded into `<group>` and removed.

## Groups

- Problem: a Frogger lane is three logs sharing shape, speed, row and rule (`wrap`), differing only in start. As separate objects each was about 22 lines, and 42 of Frogger's 59 objects repeated their neighbors. The `<grid>` of the time could not help: one spacing, one velocity, one shape.
- **A `<group>` sits beside `<object>`.** It says what its members share, then lists `<member>`s. One rule: **whatever a member leaves out it takes from its group**, then it must be complete like an `<object>` (else a load error naming the member). `<position>` and `<velocity>` can be given in halves (the group gives `<y>`, each member `<x>`).
- A member can give its own `<sprite>`s, `<animation>`, `<position>`, `<velocity>`. It **cannot** give `<collisions>`, `<actions>`, `<variables>` or timers: those are the group's, so members act the same way. A member that acts differently is an `<object>` beside the group.
- **Members are ordinary objects.** `game_xml.cpp` reads a group as one raw object per member; the engine only remembers `Object::groupName`. Each log wraps by itself, each pad dies by itself. Names are `group.N` from 1 (or `name=`); the group's name means all members wherever a name is taken (`<show>`, rules, conditions, `<reset object>`, `<fire>`, `<release>`, `<reveal>`). Drawn in file order where the group stands. `class` is the group's.
- A hidden group is a **pool** ([timers](09-timers.md)); a group with lockstep is a block (Space Invaders: one group of 11 columns and 5 rows).
- **Measured:** Frogger and Space Race got about 30% smaller (lines, bytes, tags). Still 5 lines per member and a repeated 7-line `wrap` block per lane.

| Option | Verdict |
|---|---|
| **One group per lane, members override what differs** | Chosen: one rule |
| Members give full x and y | Repeats the row in every member |
| Group position as an origin | Reads worse, hides off-screen starts |
| Nested groups, inner inherits outer | Bigger saving (the shared `wrap` block) but needs merge rules for partly given sprites |
| A grid with per-row velocity and a position list | Fixed lanes only, one shape per grid at the time; rows that change sprites (below) took away the objection |
| Reusable named templates | A different idea (opt-in parts) |
| Members overriding collisions/actions | Not allowed |

- Open: nested groups; a bare `<x>` in a member (3 lines instead of 5, about 35% total saving). Evenly spaced members are now a group of one row in columns with a `<padding>`. The flag that makes a block move as one was once also called `<group>`; it is `<lockstep>` now.


## Columns and rows: `<grid>` folded into `<group>`

- Problem: there were two ways to repeat things. A `<grid>` (inside a `<sprite>`) made identical cells on a regular pattern; a `<group>` listed members that could differ. Breakout was a group of six members, each a whole 9 by 1 grid repeated to change one color; Space Invaders the same for three kinds of alien. A grid was really a group whose members are placed on a pattern.
- **Chosen:** a group may have `<columns>`, `<rows>` and a `<padding>` instead of members, a cell to each place (`bricks.5.3`: column, then row; made the top row first). Then `<row number="...">`, `<column number="...">` and `<cell row column name>` change what the cells they pick have: sprites, animation, velocity, variables, and for a row or column the gap before it. From the general to the particular: group, then row and column, then cell. `<grid>` is gone; Breakout, both Space Invaders and Frogger's lane markings were rewritten: Breakout's bricks went from 104 lines to 29, Space Invaders' aliens from 161 to 115 and Space Invaders 2's from 194 to 108.
- **Picking** borrows the edge words' idea (`edge="all"`, `horizontal`, `left`): `number="2"`, several (`"2 4 6"`), `odd` or `even`. An attribute, since it picks.
- **A sprite that changes one need only give what changes.** The same shape keeps the rest (`<rectangle><color>color.orange</color></rectangle>`); another shape replaces it and must be complete. Sprites are matched by name. This merge rule was the open question that held back nested groups; it now applies to members too (a member's complete sprite comes out as before).
- **The gap is `<padding>`, not `<x><inc>3</inc></x>`:** a row's `<y>` is the gap above it and its `<x>` the gaps between its cells, a column's `<x>` the gap to its left. `<inc>` is a verb; a value holding a verb would muddy the format's one rule.
- **A row and a column that change the same thing in one cell is an error,** not "the later wins": which should win is a guess. A `<cell>` wins over both. Collisions, actions and timers stay the group's, so every cell acts the same; a brick that acts differently is an object of its own.
- **Layout:** a slot is the size of its row's sprite; a cell with a sprite of another size (a bigger boss in the middle) sits centred in its slot and its neighbours do not move. Rows can be different heights (Space Invaders 2's three kinds are 50, 46 and 49 pixels tall) and still follow one another.
- `<columns>` and `<rows>` are whole numbers, settled when the file is read, since they say how many objects there are and what they are called. A picture drawn the same way for many cells is drawn once and shared (`game_expr::drawOnce`).

| Option | Verdict |
|---|---|
| **`<group>` with columns, rows and row/column/cell changes** | Chosen: one way to repeat things |
| Keep `<grid>` and add per-row changes to it | Two ways to say a block; a grid lived inside a sprite |
| Later of a row and a column wins | Hides a mistake; refused instead |
| A row's place given as a position | A gap reads better and keeps rows following one another |
| `<x><inc>3</inc></x>` for a nudge | A verb inside a value |

- Open: a row or column with a `name` (to say `object="squids"`), a pick by range (`2-4`), a cell changing its collisions.
