# Maps

A `<map>` is level data drawn as text: each `<row>` is a row of tiles, one character a tile, so the file shows the level as it looks. It stands in any `<objects>`, or in the kit. With `<cells />` it is a playfield of cells for grid puzzles and [board games](boards.md).

## Drawing a map

```xml
<map name="maze">
  <tile><width>tile</width><height>tile</height></tile>
  <position><x>0</x><y>top</y></position>
  <row>r--------T--------7</row>
  <row>|o.......|.......o|</row>
  <row>|.[].[-].u.[-].[].|</row>
  ...
  <group name="walls" class="wall">
    <group name="across" char="-"> <sprite>...</sprite> </group>
    <group name="corner" char="r"> <sprite>...</sprite> </group>
  </group>
  <group name="dots" char="." class="food"> ... </group>
  <object name="pacman" char="@"> ... </object>
</map>
```

- `<tile>` is a tile's size, `<position>` the map's top left. Rows must be equally wide.
- **A group** with `char="x"` has a member in the middle of every tile of that character, named under the map in reading order (`maze.dots.1`). It has no position or members of its own, but everything else a group has, inner groups too.
- **An object** picks a character found on exactly one tile, where it starts.
- A character nothing picks is an empty tile; two pieces may not pick one character. Rows are trimmed, so an empty tile at an end is a character nobody picks (Pac-Man's `,`), not a space.
- The map's name means everything on it (`<reset object="maze" />`).
- **A map in the kit.** Maps that share their tile, place and pieces (Berserk's rooms) take one kit map and give only what differs, usually the rows: `<map name="maze"><row>+-+x+-+</row> ... </map>`.

## Walking a map

An object with `<maze map="maze">` walks that map's tiles: it keeps to the middle of its lane, chooses its way only in the middle of a tile, and never steps onto a tile that is a wall to it.

```xml
<maze map="maze">
  <wall class="wall" />
  <wall class="door" sprite="normal" />    <!-- a wall only while it shows "normal" -->
  <slow class="tunnel">0.5</slow>          <!-- half speed on those tiles -->
</maze>
```

- **Walls** are the `<wall>`s: every object of that class on the map in play, optionally only while the walker shows a [look](pictures.md#looks). So ghost eyes go in through a door that stops ghosts.
- **Slow ground**: on a tile holding an object of that class (drawn or not), it goes at that share of its speed.
- **How it chooses** is the last [steering verb](commands.md#steering) it was given: `<go>` (a player's stick), `<hunt>` (arcade targeting), `<wander>` (at random).
- Off the map (a tunnel) it goes straight on; an edge's `<wrap />` brings it round.
- It faces the way it goes. `<reverse />` turns it round; `<stop />` or a reset stops it until told again.
- A tile is found from the walker's middle, so any size works; keep it under half a tile a frame.

## Cells

A map with `<cells />` is a playfield such as Tetris's well: pieces move a whole cell at a time, it knows which cell holds what, and it can be asked about its rows.

```xml
<map name="board">
  <tile><width>cell</width><height>cell</height></tile>
  <position><x>left</x><y>top</y></position>
  <columns>10</columns>                       <!-- in place of rows: an empty map -->
  <rows>20</rows>
  <cells />
  <group name="blocks" class="block">
    <group name="cyan" char="i"> ... </group> <!-- on no tile: waits to be spawned -->
  </group>
  <group name="pieces" class="falling">
    <timers><timer><every>status.gravity</every><hop direction="down" /></timer></timers>
    <collisions><collision edge="bottom"><settle /></collision></collisions>
    <group name="t">
      <shape>
        <row>.t.</row>
        <row>ttt</row>
        <row>...</row>
      </shape>
    </group>
  </group>
  <checks>
    <full class="block"><clear /><collapse /><inc variable="status.lines" /></full>
  </checks>
</map>
```

### Pieces

- **Size**: its `<row>`s, or `<columns>` and `<rows>` for one that starts empty (not both).
- **A piece waiting.** A group or object that picks no character, or one on no tile, is a piece out of play (`board.cyan`) until spawned into a cell.
- **Spawning into a cell**: `<spawn object="pieces"><random /><column>4</column><row>1</row></spawn>`. Column and row count from 1; left out, they are the spawner's cell. `<free />` is a free cell at random (`<free><below>6</below></free>`: under row 6). `<random />` picks one member of a group. A spawn into a taken cell or off the map is refused.
- **A `<shape>`** is a piece of several cells drawn as rows; each character is the map's piece that picks it, `.` is no cell. A group of groups may pick one character for all of them: a shape's cell of that character is any one at random (Columns' jewels). Live numbers `column` and `row` are a piece's cell.

### Moving on cells

| Verb | Does |
|---|---|
| `<hop direction="down" />` | one cell (its content: that many) |
| `<move direction="left">12<wait>0.2</wait></move>` | held: one step, waits, then 12 cells a second |
| `<slide direction="down" />` | cell by cell at once until refused (Tetris's hard drop). `class="tile"`: every piece of that class, the one nearest that way first; refused if none moved (Doubles) |
| `<turn direction="clockwise" />` | a quarter turn of a shape, once a press |
| `<cycle />` | each cell of a shape moves on along its outline (Columns) |
| `<go direction="up">8</go>` | keeps going 8 cells a second until another `<go>` |

- **A refused move is a touch.** A step into a taken cell or off the map is not made, and runs both sides' rules: `<collision class="block" side="bottom">`, `<collision edge="bottom">`. If the rules take away what was in the way (`<despawn />`), the step goes on: that is how Snake eats. A [`<through />`](commands.md#touching) makes the step into the taken cell anyway, the two sharing it.
- Cell pieces touch only this way, never by pixels, and the window's edges do not apply.
- A step whose touch rules moved the piece (Centipede: reverse and hop down) is not also made.
- **Turning** is the standard rule (SRS): a shape turns about the middle of its square (3x3 for T S Z J L, 4x4 for I) and tries the standard kicks. A two-cell shape turns about its bottom left cell.

### Chains

`<chain object="body" />` on a piece: the pieces of that name behind it, drawn beside one another in the rows (`ooo@`), are its links; each step every link takes the cell of the one ahead. The tail is no blocker.

- `<grow />`: one link longer after the next step.
- `<split />` on a link (the one shot) cuts the chain: the link behind becomes a head with the rest, the old head's way and look.
- `<reverse />` turns a `<go>` round; a go straight back into the first link is refused.

### Settling and joining

- **`<settle />`** ends a shape: it is despawned and its cells stay, loose. Then the map asks its checks.
- **`<joins>`** (after `<grip>`) names a look per way the joined piece lies: `<alone sprite="pill" /><left sprite="openleft" /><right .../><up .../><down .../>`. Settled cells with `<joins>` stay joined, and `<fall />` moves them together (Capsules).

### Checks

Asked after each settle or put:

| Check | Finds | Then |
|---|---|---|
| `<full class="block">` | each row whose every cell holds that class | its commands run once a row, bottom up; `<clear />` takes the row away, `<collapse />` brings everything above down a row |
| `<line class="jewel"><atleast>3</atleast>` | every line of that many alike, across, down or slanting (`<straight />`: no slants) | its commands run once for all; asked again until nothing is taken (chains) |

- Alike is the same group (a copy is of its piece's group), or with `<alike variable="hue" />` the same value of that variable.
- `<fall />` drops every loose piece down its column; `<fall class="pill" />` only that class, the rest holding things up.
- A check runs no `<push>`: set a variable and let a condition answer.

### Pushing, merging and counting

Turn-by-turn puzzles (Crates, Doubles, Minefield) are cell pieces moved by keys and touches:

- **`<shove />`** in a rule for a refused step: the piece in the way is pushed one cell on (its whole shape if it is part of one), and the step goes on. A shove that is itself refused (a crate against a wall) leaves both where they were, so a worker pushes one crate, never two.
- **`<merge variable="value" />`** in a rule: if the other piece's variable is equal, the other is despawned, this one's variable becomes the sum and the step goes on. A piece merges once a slide.
- **Maps laid in the same place** (same tile and position) are layers: a piece meets only pieces of its own map, so the worker walks over the floor's spots.
- **`<here />` and `<around>`** in a `<count>` or a `<trigger class>` name cells: `<here />` this piece's cell, `<around>` the eight round it (`<straight />`: the four beside it; `<alike variable="value" />`: only pieces whose variable equals this one's). Every layer is looked at.

```xml
<count class="spot" into="self.home"><here /></count>
<count class="mine" into="self.near"><here /><!-- or --><around /></count>
<trigger class="square" action="open"><around /></trigger>
```

- A trigger about the cells around runs each piece once a press, however often it is reached, so a square with no mines near opening those round it floods (Minefield).
- **`<undo />`** in a key's input puts every piece, look and number back as before the last press that moved a cell piece, as many times as there were such presses. A new scene starts with nothing to undo.

### A second map: the next-piece box

A cell map may hold nothing: `<map name="next">` with its tile, position, `<columns>4</columns><rows>2</rows><cells />`.

- `<spawn object="pieces" into="next"><random /></spawn>` makes a piece there.
- `<send object="pieces" from="next" into="board"><column>4</column><row>1</row></send>` moves it (refused if there is none or no room).
- **A piece away from its own map is only shown**: its timers and actions do nothing until it is sent home.
