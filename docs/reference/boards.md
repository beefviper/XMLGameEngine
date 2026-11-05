# Board games

A board game's pieces sit on a [map of cells](maps.md#cells) and move a move at a time, by rules of how each piece may go. A **picker** (a cursor) chooses the piece and where it goes. The computer opponent is a [`<think>`](states.md#computer-players).

## Moves

Checkers' red pieces:

```xml
<group name="red" class="red" char="r">
  <sprite name="man"> ... </sprite>
  <sprite name="king"> ... </sprite>
  <moves>
    <forced />
    <step><x>-1</x><y>-1</y></step>
    <step><x>1</x><y>-1</y></step>
    <jump class="white"><x>-1</x><y>-1</y></jump>
    <jump class="white"><x>1</x><y>-1</y></jump>
    <promote sprite="king"><row>1</row></promote>
  </moves>
  <moves sprite="king"> ... all four ways ... </moves>
</group>
```

- **`<moves>`** come after `<joins>`. A `<step>` goes to the empty cell that many cells across and down (`<y>` grows down the board). A `<jump class="white">` goes over a piece of that class into the empty cell beyond, and takes it.
- **One set per look**: `<moves sprite="king">` while it shows that look, the plain `<moves>` otherwise.
- **`<forced />`** first: while any piece of its class can jump, it may only jump.
- **A run of jumps**: a piece that has jumped and can jump again must.
- **`<promote sprite="king"><row>1</row></promote>`** last: a piece moved onto that row takes that look, and its move ends.
- **Live number `moves`**: how many moves a piece can make now, counted each frame before the conditions. `class="white" variable="moves" total="sum"` at 0 is a side that is stuck.

## The picker

Any object on a map of cells, usually a cursor on a map of its own laid over the board (same tile and place), so it walks over the pieces without blocking them.

| Verb | Does | Refused when |
|---|---|---|
| `<pick class="red" />` | takes up the piece of that class in its cell (picking again takes another) | that piece has no move, or a run of jumps is under way |
| `<pick class="red"><only /></pick>` | takes up the one piece with a move, wherever it is, and goes to it | none or several can move, or one is held |
| `<place />` | makes the held piece's move that ends in the picker's cell | no move ends there, or the piece must jump on |
| `<drop />` | puts the held piece back | nothing held, or a run of jumps has begun |
| `<put class="black" />` | puts a new piece (a copy of the first one written) in its cell | the cell is taken or the rules forbid it ([below](#pieces-put-not-moved)) |

All are self verbs. Since a refused verb ends its list, what is written after `<place />` (passing the turn) runs only when the move is over. Whose turn it is is the game's own: Checkers keeps it in a variable and in the cursor's looks.

**The mouse.** A [`<track />`](input.md#pointing-controls) on a map of cells puts the object in the cell under the pointer. `<onto>` narrows it to useful cells:

```xml
<hop direction="right"><onto class="hslot" /><onto class="vslot" /></hop>
<track><onto class="hslot" /></track>
```

A hop with `<onto>` goes to the nearest cell holding one of those classes: left and right along its row; up and down to the nearest row ahead that has one, nearest the column kept (as a text cursor keeps its column). Refused when there is none. A track with `<onto>` takes the one nearest the pointer.

## Marks

`<marks>` on the picker (after its `<moves>`) show the play with objects of their own, copies made and taken away as the play changes:

```xml
<marks sprite="redheld">
  <held object="heldmark" />                       <!-- the cell of the piece held -->
  <moves object="movemark" variable="options.moves" />  <!-- each cell it can go to -->
  <movable object="movablemark" class="red" />     <!-- each piece of the class with a move -->
  <put object="putmark" class="black" />           <!-- each cell a piece can be put -->
  <last object="lastmark" />                       <!-- where the last move began and ended -->
</marks>
```

- `sprite=` shows them only while the picker has that look; none, always.
- `variable=` on a mark shows it only while that variable is not 0, so a title screen can toggle it.
- Put the marks on a map of their own so they block nothing.

## Pieces put, not moved

What a piece's `<moves>` say decides where `<put>` may go:

| In `<moves>` | A put goes | Game |
|---|---|---|
| `<flank class="white" />` | only where a line of white pieces is trapped against one of its own; the trapped pieces turn (`<straight />`: no slants) | Reversi |
| `<fall />` | to the lowest free cell of the picker's column | Four in a Row |
| nothing (`<moves />`) | any free cell | Tic-Tac-Toe |
| `<on class="hslot" />` | only on a piece of that class, taking its place | Dots and Boxes |

- `<put class="line" />` takes whichever group of the class fits the cell (`across` on an `hslot`, `down` on a `vslot`).
- `<put class="line"><close class="box" object="redbox" /></put>`: after the put, each `box` beside it with no free place left on its four sides becomes a copy of `redbox`. A put that closes counts as refused, so the turn does not pass.
- Live `moves` of a putting piece counts the cells it could go, so a side whose `moves` add up to 0 must pass.
- The map's `<checks>` are asked after each put, so `<line class="red"><atleast>4</atleast>` finds four in a row.
- `<count class="black" into="score.black" />` sets the variable to how many of that class are in play, those put this frame included.
