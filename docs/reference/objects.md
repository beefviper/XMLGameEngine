# Objects

An object is anything drawn, moved or touched. `name` is required; `class` is optional (a kind that rules and conditions match); `from=` takes a [piece](#pieces-and-from).

```xml
<object name="ball" class="ball">
  <sprite><circle><radius>8</radius></circle></sprite>
  <position><x><centered /></x><y>100</y></position>
  <velocity><x>3</x><y>2</y></velocity>
  <collisions>
    <collision edge="vertical"><bounce /></collision>
  </collisions>
</object>
```

## Parts of an object

**What is left out is nothing:** no velocity stands still, no `<collisions>` touches nothing. Every part but the position is optional. In this order:

| Part | What it does |
|---|---|
| `<sprite>` | what it looks like: one [shape](pictures.md#sprites). Several named sprites are [looks](pictures.md#looks) or [animation](pictures.md#animation) frames |
| `<animation>` | which sprites show, in what order, for how long |
| `<position>` | where it starts, `<x>` and `<y>` from the top left. `<centered>value</centered>` puts its middle there instead (empty: the window's middle); `<centered><offset>-60</offset></centered>` puts it that far from the window's middle, right or down (a negative number left or up). A half written `update="live"` [follows](format.md#live-numbers-of-an-object) what it reads every frame. Required unless it has no sprite |
| `<velocity>` | pixels a frame, `<x>` and `<y>`; a half left out is 0 |
| `<acceleration>` | added to the velocity every frame: gravity is a `<y>`. `<stop />` takes it away, a reset gives it back |
| `<heading>` | degrees clockwise from straight up. Lets `<turn>`, `<thrust>`, `<swing>` work, and turns the picture. `update="live"`: [follows](format.md#live-numbers-of-an-object) what it reads |
| `<pivot>` | the point the picture turns about ([turning about a pivot](pictures.md#turning-about-a-pivot)); the position is then where that point sits |
| `<drag>` | 0 up to 1: the share of its speed lost each frame |
| `<friction>` | 0 or more: how much a still surface holds it back, and what brings it to [rest](collisions.md#knocks) |
| `<mass>` | above 0: how hard it is to [knock](collisions.md#knocks). None: a knock does not move it |
| `<surface>` | this is ground: a `<drag>` and `<grip>` that a gripping thing over it takes ([surfaces](pictures.md#surfaces)) |
| `<pull object="sun">strength</pull>` | that much more speed toward the nearest of that name each frame; several add up |
| `<seek object="ship">degrees</seek>` | a homing missile: its way turns toward the nearest of that name, at most that much a frame, keeping its speed |
| `<light>`, `<dark>` | how far it lights; drawn only when lit ([light and dark](pictures.md#light-and-dark)) |
| `<altitude>`, `<shadow object>` | height off the ground; kept under another ([height and shadows](pictures.md#height-and-shadows)) |
| `<facing>` | `up`, `down`, `left`, `right`, without turning the picture: follows its last move; a `<fire>` leaves that way |
| `<poses>`, `<mirror>`, `<grip>` | looks picked by how it moves, flipped pictures, how its speed follows the keys ([poses](pictures.md#poses-mirror-grip-and-blinking)) |
| `<maze map="maze">` | walks that map's tiles ([walking a map](maps.md#walking-a-map)) |
| `<waiting />` | starts out of play (not drawn, touching nothing, not counted) until a `<reveal>`. `<ready />` in a take of a waiting piece, or an inner group of a waiting one, is in play after all (the one change rule: either written replaces the other) |
| `<hidden />` | in play but never drawn: an exit, a trigger zone |
| `<most>` | how many [copies](#spawn-and-despawn) may be out at once |
| `<collisions>` | how and what it touches ([collisions](collisions.md)). Written, it is on unless it says `<off />`; `<collisions />` alone is something others touch |
| `<actions>` | named `<action>`s that a key or a rule runs |
| `<variables>` | its own numbers, read anywhere as `name.variable`; one written `update="live"` [follows](format.md#live-numbers-of-an-object) what it reads |
| `<timers>` | what it does every so often, or once after a while ([timers](timers.md)) |

A group's member, a map's piece and a piece of the top-level `<objects>` have the same parts (the schema's `objectParts`), so they cannot drift apart.

### Numbers only

An object with no `<sprite>` only holds numbers or cards: never drawn, at 0, 0 unless it gives a `<position>` (where a hand it `<holds>` spreads from), and it must have `<variables>`, `<timers>` or `<holds>`. Keep score, lives and clock on one that no state draws (Pitfall's `status`), so resetting the player does not touch them.

An empty `<sprite name="dealing" />` is a look that draws nothing. An object whose looks are all empty keeps only where something is up to (Hearts' `table`: playing, full, taking, done, dealing), which rules test and `<become>` changes, and like a holder needs no `<position>`; `<hidden />` on it is an error. A look with a picture beside empty ones draws nothing while it wears an empty one (a message's `none`), and a member's `<sprite name="free" />` leaves it unseen in a look its group draws (the computer's players in Hearts).

```xml
<object name="status">
  <variables>
    <variable name="score">0</variable>
    <variable name="lives">3</variable>
  </variables>
</object>
```

An object with `class="projectile"` starts out of play, waiting to be fired.

## Groups

A `<group>` is several objects written once. It gives what its members share, then lists them; **whatever a member leaves out it takes from its group**.

```xml
<group name="logrow3" class="logs">
  <sprite><rectangle><width>3 * cell</width><height>body</height><color>color.brown</color></rectangle></sprite>
  <position><y>3 * cell</y></position>                 <!-- the row, once -->
  <velocity><x>-1</x></velocity>
  <collisions><collision edge="horizontal"><wrap /></collision></collisions>
  <member><position><x>20</x></position></member>      <!-- where each starts -->
  <member><position><x>272</x></position></member>
  <member name="last"><position><x>524</x></position></member>
</group>
```

- **Members are ordinary objects**, named after the group with a number from 1 (`logrow3.1`), or by their own `name`. Each moves and touches on its own.
- **The group's name means every member** wherever a name is taken: `<reset object="logrow3" />`, a rule's or condition's `object=`. Its `class` is every member's.
- **A member gives only** sprites, animations, position and velocity (or a half of one), its own starting `<variables>`, and for a player its `<owns>` and `<think>` ([players](states.md#players-as-a-group)). Rules, actions and timers are the group's; a member that acts differently is an object of its own.
- **A member's sprite changes the group's of that name.** Of the same shape it gives only what changes (`<rectangle><width>10</width></rectangle>`); of another shape it must be complete. A new name adds a sprite.
- Members are drawn in the order written, where the group stands.

### Groups inside a group

A group can hold groups instead of members; **an inner group takes from the one around it** the same way. Frogger writes `<wrap />` once for all ten lanes:

```xml
<group name="lanes">
  <rows>1</rows>
  <collisions><collision edge="horizontal"><wrap /></collision></collisions>
  <group name="river" class="logs">
    <sprite><bitmap> ... </bitmap></sprite>
    <group name="logrow2">
      <columns>3</columns>
      <padding><x>(window.right + 180) / 3 - 180</x></padding>
      <position><x>20</x><y>2 * cell + 6</y></position>
      <velocity><x>1.4</x></velocity>
    </group>
    <group name="logrow4"> ... </group>
  </group>
  <group name="traffic" class="hazard"> ... </group>
</group>
```

- **A single thing is replaced** (a half of position, velocity or padding; columns, rows, animation, acceleration, heading, facing, active, class). A sprite changes the outer's of its name.
- **A list adds:** inner rules run after the outer's; actions, variables and timers join (one of the same name replaces).
- **Names.** Members are named after their innermost group (`logrow4.2.1`), and every group around them names them too: `object="river"` is every log.
- A group holds members, cells or groups, never two of these.

### Columns and rows

Instead of members, a group can lay out a block: `<columns>` and `<rows>` make a cell per place, `<padding>` is the gap. Its `<position>` is the top left of the first cell.

```xml
<group name="bricks" class="bricks">
  <columns>14</columns>
  <rows>8</rows>
  <padding><x>4</x><y>4</y></padding>
  <sprite><rectangle><width>brick</width><height>12</height><color>color.red</color></rectangle></sprite>
  <position><x>margin</x><y>top</y></position>
  <row><number>3</number><number>4</number><sprite><rectangle><color>color.orange</color></rectangle></sprite></row>
  <row number="5"><sprite><rectangle><color>color.green</color></rectangle></sprite></row>
  <cell row="1" column="7" name="boss"> ... </cell>
</group>
```

- Each cell is an object named by group, column and row (`bricks.5.3`), made top row first, left to right.
- `<slope>N</slope>` after the padding puts each column N pixels lower than the last (Donkey Kong's girders).
- Three tags change the cells they pick, from general to particular:

| Tag | Picks | Can change |
|---|---|---|
| `<row number="2">` | one row; several by a `<number>` each; every other by `<odd />` or `<even />` | sprites, animation, velocity, padding (`<y>` the gap above, `<x>` between its cells), variables |
| `<column number="...">` | columns, the same way | the same; its `<x>` padding is the gap to its left |
| `<cell row column name>` | one cell, which it may name | sprites, animation, velocity, variables |

- A cell's slot is the size of its row's sprite; a cell of another size sits centred in its slot.
- A `<cell>` wins over its row and column, but a row and a column that change the same thing in one cell stop the load.

### Lines of text

`<rows>` alone is one column (and `<columns>` alone one row), its cells named by the one number (`help.2`). This is how a screen writes lines of words once: the group gives the style and the centring, each row its words.

```xml
<group name="help">
  <rows>3</rows>
  <padding><y>30</y></padding>
  <sprite><text style="help" /></sprite>
  <position><x><centered /></x><y><centered><offset>-90</offset></centered></y></position>
  <row number="1"><sprite><text><content>build the columns down in alternating colours,</content></text></sprite></row>
  <row number="2"><sprite><text><content>each suit up from ace to king on the foundations</content></text></sprite></row>
  <row number="3"><sprite><text><content>w a s d or the arrows move</content><color>color.lightgrey</color></text></sprite><padding><y>40</y></padding></row>
</group>
```

- A text is only measured by a window, so rows of text are their `<padding>` apart, top to top; a row's own padding is the step before it (a paragraph).
- Each line is centred on its own width.
- Only a group's text may leave out its words; a cell that ends up with none stops the load.

## Pieces and from=

The game's top-level `<objects>` holds objects, groups and maps written once, called **pieces**. A piece is never in play itself: it comes into play where a deeper `<objects>` takes it (of `<states>`, a state or a scene), or as copies a [spawn](#spawn-and-despawn) makes. An object is only a definition; where it is written says whether, and for how long, it is in play.

```xml
<objects>                                         <!-- pieces -->
  <object name="paddle" class="paddle"> ... </object>
  <object name="bomb" class="bomb"> ... <most>6</most> </object>   <!-- only ever spawned -->
</objects>
<states>
  <objects>                                       <!-- in play the whole game -->
    <object name="paddle1" from="paddle"><position><x>10</x></position></object>
    <object name="paddle2" from="paddle"><position><x>780</x></position></object>
  </objects>
  ...
</states>
```

- **Taking.** `from="piece"` takes a piece under the name written, changed by what is written. An object with no `from=` named like a piece takes it as it is (`<object name="topwall" />`).
- A named member of a group piece can be taken alone; it keeps the group's class, rules and picture.
- A piece can be `from=` an earlier piece (Asteroids' medium and small rocks are `from="rock"`). A group `from=` an object piece is a group whose members all have what the object has (Cannon Hill's two barrels, `<group name="barrels" from="barrel">`).
- **Where it is taken says how long it lasts:** the game's, a state's or a scene's `<objects>`.
- **Only ever spawned.** A piece a `<spawn>` or `<fire>` names and nothing takes is put beside its spawner, so its copies belong where the spawner does and read that scene's variables.

### The one change rule

A take, a state `from=` another and a scene `from=` another all change what they start from the same way:

| What is written | Does |
|---|---|
| attributes (`class`) | replace |
| a `<sprite>` or `<animation>` | replaces the one of that name, whole, or is added |
| an `<x>` or `<y>` of position, velocity, acceleration or padding | replaces that half |
| a `<variable>`, `<action>`, `<timer>` | replaces the one of that name, or is added (an unnamed timer is added) |
| `<collision>` rules | are added after |
| `<solid>`s; `<member>`s, `<row>`s, `<column>`s, `<cell>`s | replace all of them |
| an inner group or map piece of the same name | is changed the same way, or added |
| anything else | replaces that tag |

These stop the load: a `from=` naming no piece, taking a name the list already has, a map `from=` an object (or the reverse), two pieces of one name, a piece `from=` a later one, a dotted name (`rocks.2`; name the member instead).

All of this is worked out on the XML before any object is read (`load/merge.cpp`), and the same in XSLT for a generated program (`merge.xsl`).

## Spawn and despawn

Objects can be made and taken away while the game runs, and **a copy is the same as an object the file wrote.**

```xml
<action name="fire">
  <fire object="shot" />                 <!-- a new copy of the shot piece -->
  <play sound="zap" />                   <!-- only if one left -->
</action>
...
<object name="shot" class="shot">        <!-- a piece: in the top-level <objects> -->
  <sprite> ... </sprite>
  <velocity><y>-8</y></velocity>
  <most>3</most>
  <collisions>
    <collision edge="all"><despawn /></collision>
    <collision class="rock"><despawn /></collision>
  </collisions>
</object>
```

- `<spawn>` and `<fire>` each make a copy named with the first free number (`shot.1`, `shot.2`, then `shot.1` again once that one is gone).
- A copy moves, touches, counts its own timers, holds its own variables and draws its own `<random>`s. In its rules, `self` is the copy.
- **The piece's name means every copy:** `<collision object="shot">`, a condition counting `object="shot"`, `<despawn object="shot" />`. A copy's own name works too (`<tether from="base" to="abm.1">`).
- **A group piece** spawns its members in turn, or one at random with `<random />`.
- **`<most>`** caps copies out at once; at the cap the spawn is [refused](commands.md#a-press-that-cannot-be-done).
- **`<despawn />`** takes a copy away at the frame's end (its name free again); one the file wrote stays out until a reset.
- A copy made during a frame joins when the frame ends, and first moves the frame after.
- A full reset takes every copy away; `<reset object>` and `<reset scene>` take away copies of what they reset.
- Any object can be spawned: `<spawn object="ball">` makes a second ball.

`<fire>` sends only copies of a piece. An object that starts `<waiting />` waits out of play for a `<reveal>`, and `<die />` puts it back out: that suits fixed things that come back where they were (Frostbite's igloo blocks, a door, a saucer).
