# Groups, maps and cells

**Status:** Built: groups with members, columns and rows, groups inside a group, maps, and cell maps for grid puzzles. `<grid>` was folded into `<group>` and removed. Reference: [objects](../reference/objects.md#groups), [maps](../reference/maps.md).

## Groups

- **Problem:** a Frogger lane is three logs sharing shape, speed, row and rule, differing only in start. As separate objects, 42 of Frogger's 59 objects repeated their neighbours.
- **Chosen: a `<group>` beside `<object>`, with one rule: whatever a member leaves out it takes from its group.** Then it must be complete like an object. Position and velocity can come in halves (the group gives `<y>`, each member `<x>`).
- A member may give its own sprites, animation, position and velocity, never collisions, actions, variables or timers: members act alike, and one that acts differently is an object beside the group.
- **Members are ordinary objects.** A group is read as one raw object per member; the engine remembers only `groupName`. The group's name means every member wherever a name is taken.
- Frogger and Space Race got about 30% smaller.

| Option | Verdict |
|---|---|
| **One group, members override what differs** | Chosen: one rule |
| Group position as an origin | Reads worse, hides off-screen starts |
| Reusable named templates | A different idea (opt-in parts; later the [kit](13-kit.md)) |
| Members overriding collisions or actions | Not allowed |

## Columns and rows

- **Problem:** there were two ways to repeat things. A `<grid>` made identical cells; a group listed members. Breakout was six groups, each a whole grid, to change one colour.
- **Chosen:** a group may have `<columns>`, `<rows>` and `<padding>` instead of members (`bricks.5.3`: column, then row). `<row number>`, `<column number>` and `<cell row column>` change what they pick, from the general to the particular. Breakout's bricks went from 104 lines to 29.
- **Picking:** `number="2"` picks one; several are a `<number>` each; `<odd />`, `<even />`.
- **A sprite that changes one thing gives only that** (`<rectangle><color>color.orange</color></rectangle>`); another shape must be complete. This partial-sprite merge is what later made nesting possible.
- **The gap is `<padding>`**, not `<x><inc>3</inc></x>`: a verb inside a value would break the format's one rule.
- **A row and a column changing the same thing in one cell is an error**, not "the later wins". A `<cell>` beats both.
- A slot is its row's sprite size; a bigger cell sits centred and its neighbours do not move. A picture shared by many cells is drawn once (`drawOnce`).
- Open: a named row or column, picking a range, a cell changing its collisions.

## Groups inside a group

- **Problem:** one rule repeated over unrelated groups: Frogger's ten lanes each wrote the same `<wrap />`.
- Looked at a group inside a group, and rules on a class. A class does not fit (Frogger's lanes are two classes sharing the wrap) and is a second place behaviour comes from. **Chosen: groups inside a group.**
- **From the general to the particular.** A single thing (a half of position, columns, heading, class...) is replaced; a sprite is merged by name; lists add: collision rules run after the outer's, and actions, variables and timers join it, a repeated name replacing.
- **Names:** members take their innermost group's name (`logrow3.2`), and every enclosing group's name means them too (`outerGroups`). Lockstep blocks are the innermost groups.
- A group holds members, cells or groups, never two kinds. The schema's `innerGroupType` differs only in allowing partial sprites.
- Generated games make each group a function that takes the outer's `Member` and changes it, so the C++ reads like the file.
- `<heading>` may be given by a group, so Combat's two tanks are one group.

## Maps

- **Problem:** Pac-Man's maze is hundreds of pieces; as groups the file would not show the maze.
- **Chosen: `<map>`, rows of characters.** Pieces pick their character with `char=`; a group gets a member on every tile of it, an object takes the one tile it is on. Any character can be any piece.
- A map's members are named under the map (`maze.dots.17`), so two maps can each have `walls`; the group's own name still means all of them.
- The layout is shared code (`RawMap`, `tilesOf`, `placeOnMap`), so a generated game lays a map out the same.
- Rejected: one element per piece kind, a separate legend. Open: a map taking pieces one by one from the kit.

## Cells

The grid puzzles (Tetris, Columns, Snake, Centipede, Capsules) needed pieces that move a cell at a time, turn, settle, and a playfield that answers questions ("is this row full").

- **Folded into `<map>`** rather than a new `<grid>`: a map is already a grid with tile size and position, and Pac-Man's walkers already ask whether the next tile is a wall. A map gains `<columns>`/`<rows>`, a `<cells />` switch, `<checks>`, spawning into a cell, live `column`/`row`. Code: `game/cells.cpp`.
- **A piece waiting:** a piece on a cell map that picks no tile is out of play until a `<spawn>` with `<column>`/`<row>`; `<random />` picks a member.
- **`<shape>`** is rows of characters; each character is the map's piece, so colour comes from the blocks. The shape is an undrawn holder; its cells stay when it `<settle />`s.
- **A refused move is a touch.** A step into a taken cell or off the map is not made, and both sides' rules run, so landing, eating and turning are ordinary rules.
- **Turning** is SRS: each shape in the smallest square it turns in, the standard kick tables. A 4x4 box for every piece was rejected (no middle cell).
- **Checks** run after a settle: `<full>` rows bottom up with `<clear />`/`<collapse />`; `<line>` of alike pieces, all at once and asked again until nothing goes (the chain); `<fall />` is gravity per column. **Alike** is a group, not a look, since a look can change.
- **A second map** (Tetris's next box): `<send object from into>` and `<spawn into>`. A piece away from its home map is only shown, so the file needs no word for "this one is not played".
- **Snake:** `<go>` on cells, `<chain object>` (links drawn in the rows, kept as a list), `<grow />`, `<spawn><free />`. Eating needed no word: a refused step whose touch removes the blocker goes on.
- **Centipede:** `<split />` cuts a chain and the link behind becomes a head. Turning is a refused step whose rules `<reverse />` and hop down. `<through />` makes a refused step anyway, sharing the cell, so heads are never stuck. The player's zone is a row of undrawn pieces.
- **Capsules:** two-cell shapes stay joined once settled and fall together; `<joins>` gives a look by where the other half lies; `<line><straight /><alike variable="hue" />` matches across classes; `<fall class="pill" />` lets only halves fall.
- Open: hold, lock delay, ghost piece, hidden rows, a bag of seven, scoring by rows at once, chain rounds for Columns, gliding between cells, wrapping chains, the flea, spider and scorpion.
