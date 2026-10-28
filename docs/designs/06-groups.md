# Groups

**Status:** Built: listed members, cells in columns and rows, and groups inside a group. `<grid>` was folded into `<group>` and removed.

## Groups

- Problem: a Frogger lane is three logs sharing shape, speed, row and rule (`wrap`), differing only in start. As separate objects each was about 22 lines, and 42 of Frogger's 59 objects repeated their neighbors. The `<grid>` of the time could not help: one spacing, one velocity, one shape.
- **A `<group>` sits beside `<object>`.** It says what its members share, then lists `<member>`s. One rule: **whatever a member leaves out it takes from its group**, then it must be complete like an `<object>` (else a load error naming the member); a velocity half nobody gives is 0, and a group with no `<collisions>` touches nothing ([objects](05-objects.md)). `<position>` and `<velocity>` can be given in halves (the group gives `<y>`, each member `<x>`).
- A member can give its own `<sprite>`s, `<animation>`, `<position>`, `<velocity>`. It **cannot** give `<collisions>`, `<actions>`, `<variables>` or timers: those are the group's, so members act the same way. A member that acts differently is an `<object>` beside the group.
- **Members are ordinary objects.** `load/xml.cpp` reads a group as one raw object per member; the engine only remembers `Object::groupName`. Each log wraps by itself, each pad dies by itself. Names are `group.N` from 1 (or `name=`); the group's name means all members wherever a name is taken (`<show>`, rules, conditions, `<reset object>`, `<fire>`, `<release>`, `<reveal>`). Drawn in file order where the group stands. `class` is the group's.
- A group with `<active>false</active>` waits out of play until revealed or released ([pools-and-enemies](10-pools-and-enemies.md)); a group with lockstep is a block (Space Invaders: one group of 11 columns and 5 rows).
- **Measured:** Frogger and Space Race got about 30% smaller (lines, bytes, tags). Still 5 lines per member and a repeated 7-line `wrap` block per lane.

| Option | Verdict |
|---|---|
| **One group per lane, members override what differs** | Chosen: one rule |
| Members give full x and y | Repeats the row in every member |
| Group position as an origin | Reads worse, hides off-screen starts |
| Nested groups, inner inherits outer | Built later (below), once sprites could be changed in part |
| A grid with per-row velocity and a position list | Fixed lanes only, one shape per grid at the time; rows that change sprites (below) took away the objection |
| Reusable named templates | A different idea (opt-in parts) |
| Members overriding collisions/actions | Not allowed |

- Open: a bare `<x>` in a member (3 lines instead of 5, about 35% total saving). Evenly spaced members are now a group of one row in columns with a `<padding>`. The flag that makes a block move as one was once also called `<group>`; it is `<lockstep>` now.


## Columns and rows: `<grid>` folded into `<group>`

- Problem: there were two ways to repeat things. A `<grid>` (inside a `<sprite>`) made identical cells on a regular pattern; a `<group>` listed members that could differ. Breakout was a group of six members, each a whole 9 by 1 grid repeated to change one color; Space Invaders the same for three kinds of alien. A grid was really a group whose members are placed on a pattern.
- **Chosen:** a group may have `<columns>`, `<rows>` and a `<padding>` instead of members, a cell to each place (`bricks.5.3`: column, then row; made the top row first). Then `<row number="...">`, `<column number="...">` and `<cell row column name>` change what the cells they pick have: sprites, animation, velocity, variables, and for a row or column the gap before it. From the general to the particular: group, then row and column, then cell. `<grid>` is gone; Breakout, both Space Invaders and Frogger's lane markings were rewritten: Breakout's bricks went from 104 lines to 29, Space Invaders' aliens from 161 to 115 and Space Invaders 2's from 194 to 108.
- **Picking** borrows the edge words' idea (`edge="all"`, `horizontal`, `left`): `number="2"`, several (`"2 4 6"`), `odd` or `even`. An attribute, since it picks.
- **A sprite that changes one need only give what changes.** The same shape keeps the rest (`<rectangle><color>color.orange</color></rectangle>`); another shape replaces it and must be complete. Sprites are matched by name. This merge rule was the open question that held back nested groups; it now applies to members too (a member's complete sprite comes out as before).
- **The gap is `<padding>`, not `<x><inc>3</inc></x>`:** a row's `<y>` is the gap above it and its `<x>` the gaps between its cells, a column's `<x>` the gap to its left. `<inc>` is a verb; a value holding a verb would muddy the format's one rule.
- **A row and a column that change the same thing in one cell is an error,** not "the later wins": which should win is a guess. A `<cell>` wins over both. Collisions, actions and timers stay the group's, so every cell acts the same; a brick that acts differently is an object of its own.
- **Layout:** a slot is the size of its row's sprite; a cell with a sprite of another size (a bigger boss in the middle) sits centred in its slot and its neighbours do not move. Rows can be different heights (Space Invaders 2's three kinds are 50, 46 and 49 pixels tall) and still follow one another.
- `<columns>` and `<rows>` are whole numbers, settled when the file is read, since they say how many objects there are and what they are called. A picture drawn the same way for many cells is drawn once and shared (`drawOnce` (`load/expr.cpp`)).

| Option | Verdict |
|---|---|
| **`<group>` with columns, rows and row/column/cell changes** | Chosen: one way to repeat things |
| Keep `<grid>` and add per-row changes to it | Two ways to say a block; a grid lived inside a sprite |
| Later of a row and a column wins | Hides a mistake; refused instead |
| A row's place given as a position | A gap reads better and keeps rows following one another |
| `<x><inc>3</inc></x>` for a nudge | A verb inside a value |

- Open: a row or column with a `name` (to say `object="squids"`), a pick by range (`2-4`), a cell changing its collisions.


## Maps

- Need: Pac-Man's maze is a few hundred wall pieces, dots and pellets. As groups in columns and rows it would be dozens of groups, and the file would not show the maze.
- **Chosen: `<map>`, level data drawn as text.** Each `<row>` is a row of tiles, one character a tile; the pieces that follow pick their character with `char=`. A group gets a member on every tile of its character (numbered in reading order); an object picks a character found once, its starting place. So any character can be any shape (Pac-Man's 14 wall pieces are 14 inner groups of a `walls` group sharing its class) and objects are placed by the same picture. An attribute picking a character fits the format's rule; the rows are data the author draws, not values packed into a string.
- Members are ordinary objects; the map only places them, and leaves each its tile (`mapColumn`/`mapRow`) for the maze walkers. The expansion is shared code (`RawMap`, `tilesOf`, `placeOnMap`) so a generated game lays a map out exactly as the engine does.
- Rejected: one map element per piece kind, a legend block separate from the pieces (the piece already says what it looks like), and taking a map's pieces one by one from the game's kit for now (open; a whole map in the kit, taken by maps that give their rows, was built 2026-10-28, [kit](13-kit.md)).

## Groups inside a group

- Problem (2026-10-13, counting repetition across the games): one rule repeated over groups that are otherwise unrelated. Frogger's ten lanes each wrote the same `<wrap />` block, Space Race's nine debris lanes the same, and Breakout's two-hit row and its marching block repeated the bricks' columns, gap and size.
- Looked at: **a group inside a group** (the group rule one level up), and **rules on a class** (`<class name="logs">` holding collisions every object of that class gets). A class does not fit Frogger: its lanes are two classes (logs and hazard) that share the wrap, so the rule would still be written twice; and it is a second place an object's behavior comes from, which the author did not want. Chosen: groups inside a group.
- **Rules, from the general to the particular.** Whatever an inner group leaves out it takes from the outer. A single thing (a half of position, velocity or padding, columns, rows, animation, acceleration, heading, facing, active, class) is replaced. A sprite changes the outer's sprite of its name, as a member's does: this is the merge rule that held nesting back, built for cells. A list adds: collision rules run after the outer's (every matching rule runs, so adding is the only reading that keeps the outer's), and actions, variables and timers join the outer's, a name given again replacing it. An inner `<collisions>` may give its own `<enabled>`.
- **Names.** Members are named after their innermost group (`logrow3.2`); every enclosing group's name means them too (`Object::outerGroups`, `inGroup`), so `<show object="lanes" />` replaces ten shows and `<show object="debris" />` nine. Lockstep blocks are the innermost groups.
- A group holds members, cells, or groups, never two of these. The schema has `innerGroupType`, which differs from `groupType` only in that its sprites may be partial; the recursion goes through the type, not an `xs:group`, because `load/xsd` builds groups by value.
- Generated games: each group is a function; an inner one takes the outer's `Member` by value and changes it, so the C++ reads like the file (`lanesGroup` calls `riverGroup` calls `logrow3Group`).
- Measured: Frogger's lanes and Space Race's debris each lost about 150 lines (also by writing a member on one line); Breakout's wall about the same length, its two groups now sharing the brick. Open: none known; whether a deeper chain (a wave inside a level) wants a name for what it changes is for [open work](58-open-work.md)'s numbered bundles.
- `<heading>` on a group (2026-10-19, rewriting Combat): it had been objects only, so Combat's two tanks were two objects with the same picture, drag, rules and driving. Now a group or inner group gives it like `<acceleration>`; the tanks are one group and each player's an inner group with its corner, heading and shell. Not on a member: a member that faces another way is an inner group of its own.
