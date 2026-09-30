# XMLGameEngine: current design

This document describes what the engine does **today**, as read from the source on 2026-09-30 (before that, games were written as function-call attribute strings; see [designs/03](designs/03-expression-syntax.md)). For the reasoning behind these choices, the alternatives that were considered, and ideas that are not built yet, see [designs/00-designs.md](designs/00-designs.md).

XMLGameEngine is a video game description language (VGDL) written in XML, plus a C++ engine that loads a game description and runs it. A game is one `.xml` file. The description is declarative: there are no loops and no `if` statements in it, and no function calls either. Behavior comes from a fixed vocabulary of verbs (the tags `<bounce />`, `<stick />`, `<die />`, ...) that the engine knows how to carry out. In this document, and in the design notes, a verb is sometimes written as `bounce()` for short; in a game file it is always the tag.

## Running a game

```
XMLGameEngine              # loads "pong"
XMLGameEngine breakout     # a bare name gets ".xml" appended
XMLGameEngine pong.xml     # a name containing "." is used as given
```

The file is looked for in the current directory first, then in `./games/`. If it is not found the program prints an error and exits. Shipped games: `games/pong.xml`, `games/breakout.xml`, `games/spaceinvaders.xml`, `games/frogger.xml`, `games/spacerace.xml` (two players, W/S and Up/Down, first to two points), `games/kaboom.xml` (A/D or Left/Right; catch bombs in three waves, three missed bombs end the game, 60 points win), `games/freeway.xml` (two players, W/S and Up/Down, first to five crossings), `games/depthcharge.xml` (A/D or Left/Right to move, Space to drop, Enter to start; sink all nine submarines before eight charges are wasted) and `games/astrosmash.xml` (A/D or Left/Right to move, Space to fire, Enter to start; shoot 20 rocks before five land).

The XML and window libraries are chosen in C++ (`Game(file, XmlBackend)` and `Engine(game, WindowBackend)`); `main.cpp` uses the defaults, Xerces and SFML3. There is no command-line switch for either yet. See [Backends](#backends).

## Game file layout

A game file has one `<game>` root with exactly four children, in this order, as enforced by [assets/xmlgameengine.xsd](../assets/xmlgameengine.xsd):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../assets/xmlgameengine.xsd">
  <window name="..."> ... </window>
  <variables> <variable name="...">...</variable> ... </variables>
  <objects>   <object name="..."> ... </object> <group name="..."> ... </group> ... </objects>
  <states>    <state name="..."> ... </state> ... </states>
</game>
```

### Attributes and content

The rule the whole format follows: **an attribute names or picks something; everything else is the content of an element.**

- Attributes are labels that cannot be computed: `name`, `class`, `object`, `variable`, `state`, `action`, `button`, `edge`, `direction`, `unless`. `<inc variable="paddle1.score" />`, `<collision edge="left">`, `<input button="space">`.
- Everything else, a size, a color, a position, a flag, is written between tags. `<radius>10</radius>`, `<fullscreen>false</fullscreen>`.
- What something *does* is a list of command tags, run in the order written. There are no `;`-separated strings.

### Values

Wherever the content of an element is a number (a position, a velocity, a size, a variable's value, a threshold, a step) it is a **value**, and a value is one of two things:

- **An expression written out as text.** Ordinary arithmetic, with the names below: `window.width.center - title.width / 2`, `margin`, `ball.radius * 2`, `30`. It is evaluated by exprtk at load time.
- **One value tag**, which makes the number. Today there is one: `<random min="-7" max="7" />`, a random number from `min` to `max` (in either order; both ends are themselves expressions). Any place that takes a value takes it:

```xml
<velocity>
  <x><random min="-7" max="7" /></x>
  <y><random min="-3" max="3" /></y>
</velocity>
```

A value holds text or a tag, not both, and not two tags (`<x>5 <random .../></x>` is an error naming the element). A value tag that is evaluated at load gives one number for the whole run: a `reset()` puts an object back to the position and velocity it started with, not a new draw.

The names an expression can use:

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | window height, window width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| any global `<variable>` | its value. They are worked out in the order written, so a variable can use the ones above it |
| any `objectName.variableName` | that object's variable (available regardless of the order objects appear in the file) |
| `objectName.width`, `objectName.height` | the width and height of any object (its sprite's footprint); an object refers to itself by its own name, like any other field. Meant for `<position>`. An object's own `<variable>` named `width` or `height` wins over its size. A circle's or rectangle's size is known from its sprite, so the position is exact at load. A text's or image's size is only known once the window has measured it, so its position is finished then, and worked out again whenever the size changes (a score gaining a digit); until then `printGame` shows it as unknown. |

The other exprtk math functions (`min`, `max`, `sqrt`, ...) exist because exprtk brings them, but the game files do not use them and nothing about the format depends on them.

### `<window>`

`<window name="Pong">` (the title), then in this order:

| Element | Meaning |
|---|---|
| `<width>`, `<height>` | Size in pixels. Plain whole numbers: the expressions in the rest of the file are worked out against the window's size, so these cannot be expressions themselves |
| `<background>` | A color name, e.g. `color.black` |
| `<fullscreen>` | `true` or `false` |
| `<framerate>` | Frames per second (0-255) |

### `<variables>`

Global named numbers. Each `<variable name="margin">30</variable>` becomes a constant that any expression in the file can use (for example `margin`, `ball.radius`). Names may contain dots. The content is a value, so `<variable name="b">a * 2</variable>` and `<variable name="start"><random min="1" max="3" /></variable>` both work. Declaring a name twice keeps the later value.

### `<object>`

An object is anything that can be drawn, moved, or collided with: a ball, a paddle, a score readout, a title. Attributes: `name` (required, how other elements refer to it) and `class` (optional, a group label that collision rules and conditions can match, like a CSS class).

Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<sprite>` | yes | What the object looks like: one shape (see [Sprites](#sprites)), or a `<grid>` of them |
| `<position>` | yes | Starting position, in pixels from the top-left: `<x>` and `<y>`, each a value. May use an object's own size by its name, `title.width` and `title.height`, to place it by its size, for example a text called `title` centered: `<x>window.width.center - title.width / 2</x>` |
| `<velocity>` | yes | Starting velocity, in pixels per frame: `<x>` and `<y>`, each a value |
| `<collisions>` | yes | `<enabled>` (`true`/`false`), an optional `<lockstep>` (`true`), then zero or more `<collision>` rules. See [Collisions](#collisions) |
| `<actions>` | no | Named actions the object can perform, each `<action name="up"><move direction="up">step</move></action>`: the commands are what it does. States bind keys to these names |
| `<variables>` | no | Variables owned by this object, each `<variable name="score">0</variable>`. Other expressions refer to them as `objectName.variableName`, e.g. `paddle1.score` |

An object with `class="projectile"` starts invisible (used for bullets).

### `<group>`

A `<group>` is several objects that share a description, written once: a lane of logs, a row of debris, the pads along the top of Frogger. It sits beside `<object>` under `<objects>`. It holds the parts its members have in common, in the same order as an object (`<sprite>`, `<position>`, `<velocity>`, `<collisions>`, `<actions>`, `<variables>`, each optional except `<collisions>`), then one or more `<member>`s. Attributes: `name` (required) and `class` (optional, and every member's class).

A `<member>` says only what is its own: a `<sprite>`, a `<position>` and a `<velocity>`, and an optional `name`. **Whatever a member leaves out it takes from its group**, and after that it must be as complete as an `<object>` (a missing part is a load error that names the member). A `<position>` or `<velocity>` can give just an `<x>` or just a `<y>`, so a lane gives its row once and each member gives where along it it starts:

```xml
<group name="logrow3" class="logs">
  <sprite>
    <rectangle>
      <width>3 * cell - 2 * inset</width>
      <height>body</height>
      <color>color.brown</color>
    </rectangle>
  </sprite>
  <position>
    <y>3 * cell + inset</y>
  </position>
  <velocity>
    <x>-1</x>
    <y>0</y>
  </velocity>
  <collisions>
    <enabled>true</enabled>
    <collision edge="horizontal">
      <wrap />
    </collision>
  </collisions>
  <member><position><x>20</x></position></member>
  <member><position><x>272</x></position></member>
  <member><position><x>524</x></position></member>
</group>
```

A member is an ordinary object: it is loaded as one object of its own (`logrow3.1`, `logrow3.2`, `logrow3.3`: the group's name, a dot, and its number counting from 1 in the order written, unless the member says `name="..."`), it is drawn, moved and collided with on its own, and each `<wrap />` or `<die />` happens to that one member. A group only shares what is written. Members are drawn in the order written, at the place in the file where the group stands. The group's name means every member wherever the file names an object (`<show object="logrow3" />`, `object="pads"` in a rule or condition, `<reset object="pads" />`), and a member can be named on its own (`object="logrow3.2"`). A group with `<lockstep>true</lockstep>` in its `<collisions>` moves as one block, like the cells of a `<grid>`.

A `<grid>` and a `<group>` differ in what they repeat: a `<grid>` makes identical cells on a regular pattern from one sprite, a `<group>` lists members that can each differ in place, shape or speed and share everything else.

### Sprites

A `<sprite>` holds one shape. Colors are named (`color.red`, below); a `<color>` left out is `color.white`.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | A filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | A filled rectangle |
| `<text>` | `<content>` (a fixed label) **or** `<number>` (a value), then `<size>`, `<color>` | Text. `<content>PONG</content>` is written as is. `<number>paddle1.score</number>` shows a number: when the value is exactly one `owner.variable` it is live and redraws when that variable changes; any other value (`paddle1.score + 1`, a `<random>`) is worked out once |
| `<image>` | `<path>`, then `<flip>` (`horizontal` or `vertical`, optional) | An image file |

A `<grid>` repeats a shape as a grid of separate objects (Breakout bricks, Space Invaders): `<columns>`, `<rows>`, an optional `<padding>` (with `<x>` and `<y>`), then the shape.

```xml
<sprite>
  <grid>
    <columns>11</columns>
    <rows>5</rows>
    <padding><x>15</x><y>15</y></padding>
    <rectangle>
      <width>width</width>
      <height>height</height>
    </rectangle>
  </grid>
</sprite>
```

### Commands

Commands are tags, and where they are meaningful is what the table says. Any list of them (inside a `<collision>`, an `<action>`, an `<input>` or a `<condition>`) is run in the order written, for example `<inc variable="paddle2.score" />` then `<reset />`.

| Command | Where it is meaningful | Effect |
|---|---|---|
| `<bounce />` | collision | Reverses velocity away from the touched edge |
| `<stick />` | collision | Clamps the object inside the screen edge it touched and stops only the velocity heading into that edge; the other axis keeps going, so an object pressed against the bottom wall still slides left or right. Re-applied after the frame's move, so a stuck object never ends a frame outside the screen |
| `<wrap />` | collision (screen edge) | Once the object has gone completely off the screen through that edge and is still heading that way, puts it back in from the opposite edge, one window width (or height) plus its own size along, so anything spaced along a lane keeps its spacing. While any of it is still in view nothing happens |
| `<carry />` | collision (another object) | Lends the object the touched object's velocity for that frame, on top of its own: a frog on a log rides along with it. Worked out again every frame from whatever is still being touched, so an object that steps off is at rest |
| `<die />` | collision | Disables the object's collisions and hides it; it stops moving and being drawn until something brings it back (`<fire>` re-launching a bullet, `<reset />`) |
| `<reset />` | collision (screen edge or another object) | Puts the object back at its starting position |
| `<reset />` | state input or condition | Full game reset: every object's position, velocity, variables, visibility and collisions go back to how they started (a bullet in flight is put away, a dead alien is back), and the state stack collapses to the first state |
| `<reset object="name" />` | state input or condition | Resets that one object (or every cell of a grid, for its grid name) the same way |
| `<inc variable="owner.variable" />` | collision (screen edge or another object) | Adds 1 to that variable and refreshes any text bound to it |
| `<dec variable="owner.variable" />` | collision (screen edge or another object) | Takes 1 off that variable and refreshes any text bound to it. The variable may go below zero; a condition with `<atmost>` is what notices it has run out |
| `<move direction="up">step</move>` (also `down`, `left`, `right`) | collision, or an object `<action>` | In a collision: shifts the object (or everything in lockstep with it) once. In an object action: sets a held-key velocity (see [Input](#input)). The content is a value |
| `<hop direction="up">distance</hop>` (also `down`, `left`, `right`) | object `<action>` | A one-shot jump of `distance` pixels for each press of the key (see [Input](#input)). The content is a value |
| `<push state="name" />` / `<pop />` | state input or condition | Push a state / pop back |
| `<trigger object="name" action="up" />` | state input | Runs one of that object's named `<action>`s |
| `<fire object="projectile" />` | object action | Launches the named projectile object from the shooter's top-center, moving with the projectile's own `<velocity>`. A projectile is not drawn, moved or collided with until it is fired, and is put away again by `<die />` (hitting a target, or `edge="all"`); only one can be in flight at a time |

A command the engine does not know, or one missing an attribute it needs, stops the game loading with a message that says where (`object 'ball' > <collisions> > <collision>: unknown command <explode>`).

**Colors:** `color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`, and the muted `color.grey`, `color.darkgrey`, `color.lightgrey`, `color.brown`, `color.orange`, `color.purple`, `color.darkblue`, `color.darkgreen`, `color.forestgreen`. Any other name is fully transparent.

### `<state>`

A state is one screen: a menu, the playfield, a pause screen, a game-over screen. Attribute: `name`. Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<shows>` | yes | A list of `<show object="name" />`. Only these objects are drawn and updated while the state is current |
| `<inputs>` | yes | A list of `<input button="key">`, each holding the commands that key runs, e.g. `<push state="playing" />` or `<trigger object="paddle1" action="up" />` |
| `<conditions>` | no | A list of `<condition>`, checked every frame. See [Conditions](#conditions) |

The first state in the file is the starting state. States form a **stack**: `<push state="name" />` pushes a state, and `<pop />` pops back to the previous one.

## Input

An `<input button="q"><trigger object="paddle1" action="up" /></input>` names a key and the commands it runs. Objects never mention keys, and states never mention what an action does, so remapping a key means editing one attribute in one state.

Key names are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more listed in `source/keycode.cpp`.

While a key bound to a `<move>` in an object action is held, that direction's step is recorded. Velocity is recomputed from all four directions on every key change, so holding Down and tapping Up cancels out and releasing Up resumes Down. Left/right and up/down are independent axes, so two keys can make a diagonal.

The current state decides what a held key means. When the state changes, keys that are still down stop driving whatever the old state bound them to, and start driving what the new state binds them to. In Breakout, holding Left in `playing` and pressing Space stops the paddle, because `paused` binds only Space; unpausing while Left is still down moves it again with no re-press, and a Left first pressed during the pause starts moving it on unpause. Letting go of a key always stops what it was driving. Only continuous bindings (an action's `<move>`) resume this way; state changes and one-shot commands such as `<fire>` run only when the key is actually pressed, so holding Space through the main menu does not pause the game. Alternatives that were considered are in [designs/10](designs/10-input-and-actions.md#held-keys-across-state-changes).

**Hop.** `<hop direction="up">distance</hop>` (and `down`, `left`, `right`) in an object `<action>` is a one-shot jump, not a held move. Pressing the key queues a hop of `distance` pixels; the next frame's move makes it before collisions are worked out, so the object is judged where it lands, and it is refused (the object stays where it is) if it would leave the window. Holding the key does nothing more and releasing it does nothing, and a state change never repeats it: only continuous bindings are resumed, so a key held through a pause has to be pressed again to hop. Two hops asked for in one frame keep the later one, so a hop is always one step in one direction.

## Collisions

Each object has `<collision>` rules. A rule's content is a list of command tags, run in order. There are two kinds:

- **Screen edge:** `<collision edge="left">`, `"right"`, `"top"`, `"bottom"`, plus the groupings `"vertical"` (top and bottom), `"horizontal"` (left and right) and `"all"`. Several rules that touch the same edge all run.
- **Object against object:** name what the rule applies to with `class="..."` (a kind of other object, matched against that object's `class` attribute) and/or `object="..."` (one named object), for example `<collision class="bricks"><die /></collision>`. A `<collision>` with no selector at all applies to anything. A `<collision>` with `edge` is always a screen-edge rule.

Detection (pure geometry, `CollisionDetector`) is kept apart from response (`CommandExecutor`). Object-against-object collisions are **swept**: each object moves along its own path for the frame, and the detector finds the moment two of them first touch (rectangles as boxes, a circle against the other object's box with rounded corners), so a small or fast object cannot jump over a thin one between frames. The earliest touch in the whole frame is handled first: everything moves up to that moment, the pair's rules run, and the rest of the frame is played with whatever velocities they left, so a bounce spends the remaining part of the frame heading away. Each pair reacts at most once per frame. The detector reports which edge of the other object was touched, and each side of the pair then sees the edge from its own point of view. Screen edges are still checked by position, before the move.

Rules that apply:

- Only objects that are **shown** in the current state and have `enabled="true"` take part.
- **What a rule can do.** Against a screen edge: `<bounce />`, `<stick />`, `<reset />`, `<die />`, `<move>`, `<inc>`, `<dec>` and `<wrap />`. Against another object: `<bounce />`, `<die />`, `<reset />`, `<move>`, `<inc>`, `<dec>` and `<carry />`. `<stick />` and `<wrap />` are about a screen edge and do nothing in a rule about another object, and `<carry />` is about another object.
- **`unless`.** An object-against-object rule can carry `unless="class"`: it is passed over while the object is, at that same moment, touching something in play of that class. Frogger's river is `<collision class="water" unless="logs"><dec variable="frog.lives" /><reset /></collision>`: water costs the frog a life, unless it is also on a log. It looks at where things are right now, so it does not matter which of the two touches was handled first.
- A pair is skipped unless at least one of the two is moving, and unless one of them has a rule that answers to the other (its `class`/`object`, or a rule with no selector). An object counts as moving if its velocity is not zero, if it is being carried, or if it has just hopped, so an object that lands somewhere by hopping is judged there even though nothing else in the pair moves.
- `<lockstep>true</lockstep>` puts all cells of a `<grid>` object, or all members of a `<group>`, in lockstep: they share a lockstep number. Cells in lockstep never collide with each other, `<move>` in a collision moves all of them, and one hitting the left or right screen edge with `<bounce />` moves the whole block (this is how the invaders march). Being in lockstep changes none of the geometry: every cell is swept on its own, so a bullet only ever meets the cells that are still alive, and there is no bounding box around the block.
- Every cell of a `<grid>` is its own object, named after the grid with its column and row counted from 1: a grid called `aliens` has `aliens.1.1`, `aliens.2.1`, ... `aliens.11.5`. Names of the whole grid still work where the whole set is meant: `<show object="aliens"/>`, a rule or condition `object="aliens"`, `<reset object="aliens" />`; a cell can be named on its own (`object="aliens.3.2"`). A `<group>`'s members are named the same way (`logrow3.2`) and the group's name means all of them; see [`<group>`](#group).

## Conditions

```xml
<condition class="paddle" variable="score">
  <atleast>15</atleast>
  <push state="gameover" />
</condition>
```

Checked once per frame while the state is current. It fires when any object matching `class` and/or `object` (both optional, combined with AND) has a variable named `variable` that has reached the `<atleast>` value (greater than or equal). The first match runs its commands and stops checking for that frame. The threshold is an ordinary value, so it is not limited to 0-255 and can be an expression.

The other forms:

```xml
<condition class="aliens">
  <remaining>0</remaining>
  <push state="gameover" />
</condition>
```

`<remaining>` counts objects instead of reading a variable. It fires when no more than that many of the matching objects are still in play, that is still visible (`<die />` hides an object). `0` means they are all gone, which is how Space Invaders is won: `class="aliens"` covers every cell of the grid, and `object="aliens.3.2"` would watch a single one. `<atmost>` reads a variable from above: `<condition object="frog" variable="lives"><atmost>0</atmost>...</condition>` fires when the variable has fallen to that value or below, which is how a lives counter that goes down with `<dec>` ends the game (Frogger). A condition uses exactly one of `<atleast>`, `<atmost>` or `<remaining>` (the schema enforces this). If the filter matches no object at all, the game warns when it loads, since the condition would fire at once. Ending the game with a `gameover` state and starting over with a bare `<reset />` on that screen works as in Pong; `<reset />` also brings the dead aliens back.

## What happens when a game runs

1. **Parse and validate.** The XML backend loads the file. If the file names a schema, it is validated: Xerces does full XSD validation ("strong"); the other three backends use a small built-in validator for the subset of XSD this project uses ("weak", `xsd_lite`). `printGame()` reports which one ran.
2. **Evaluate.** exprtk evaluates every value once (an expression text, or a value tag such as `<random>`, which is drawn here). Objects, their variables, `<grid>` cells and states are built. Nothing here needs a window.
3. **Open the window.** `Engine` creates the window backend and measures each object's real size for drawing and collisions, finishes the position of any text or image that uses `objectName.width` or `objectName.height`, then pushes the first state. The program prints the game twice, once before this step (sizes and size-dependent positions shown as unknown) and once after.
4. **Loop.** Each frame: read key changes and run the current state's bindings for them; run the screen-edge rules; make any queued hops; move every shown object by its velocity, and by what it is being carried at, running the object-against-object rules at each touch on the way (per frame, not scaled by time); check conditions; clear; draw shown objects; present.

## Backends

| Job | Interface | Implementations |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml_document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keyboard | `Window` (`window.h`) | SFML3 (default), Raylib, SDL2 |

`Game` and `Engine` only ever see the interfaces. Each interface has a factory that is the single place that knows every implementation. Build-time dependency selection is in `scripts/cmake/` (see the `FORCE_LOCAL_*` options in `options.cmake`).

## Source map

| File | Responsibility |
|---|---|
| `main.cpp`, `cli.cpp` | Resolve the game filename, build `Game` and `Engine` |
| `game_xml.cpp` | Walk the parsed XML tags into raw window/variable/object/state data (`RawValue`, `RawCommand`, `RawSprite`); a `<group>` is read here as one raw object per member |
| `game_expr.cpp` | exprtk symbol table and evaluation of raw values into `Object`s and `State`s |
| `command.cpp` | Turn raw command tags into typed `Command`s |
| `game.cpp` | Objects, state stack, per-frame update, collision pairs, conditions, resets |
| `collision_detector.cpp` | Geometry only |
| `command_executor.cpp` | What each command does |
| `engine.cpp` | Frame loop and key handling |
| `object.h`, `states.h`, `color.cpp`, `keycode.cpp` | Data model, named colors, key names |
| `window_*.cpp`, `xml_*.cpp`, `xsd_lite.cpp` | Backends and the weak validator |
| `tests/` | Catch2 tests: collision geometry and swept collision, command parsing, conditions, input resolution, `stick()`, collision rules, lockstep bounce, size expressions, engine key handling, object variables, the new verbs (`dec`, `hop`, `wrap`, `carry`, `unless`, `atmost`, colors), the tag format and its rejections (`test_xml_format`), groups (`test_group`: expansion, overrides, names, lockstep, errors, both schema checkers) and Frogger played frame by frame (opt-in with `BUILD_TESTING`) |

## Known limitations

- Object-object collision knows only the four edges of the other object; there are no verbs beyond the table above (no gravity or acceleration, no arcing jump (`hop` is a single step), no shooting patterns, AI, sound).
- An object that is not visible is not moved and does not collide, whatever its velocity; that is how a bullet waits, unseen and still, to be fired. There are no hidden objects that still move and collide (an invisible trigger zone, an off-screen enemy on its way in); a game that wants one will need visibility and "in play" to be separate things.
- An object's velocity and collisions belong to the object, not to a state: any state that shows it lets it move. There is no way to show the Space Invaders aliens standing still behind the menu and have them march only in `playing`; they start marching as soon as they are shown. See [designs/09](designs/09-states-and-screens.md).
- An object's own `<variable>` named `width` or `height` shadows its measured size (`objectName.width` then reads the variable). A non-colliding spelling is under consideration; see design note 07.
- A text or image placed with `objectName.width` / `objectName.height` is re-placed only when the size of an object it names changes, so it is not re-centered after it has moved on its own, and other expressions (velocity, variables) see those sizes as 0 for unmeasured text and images.
- Movement is in pixels per frame with no acceleration and no time step.
- A `<random>` is drawn once, when the game loads. A piece that falls, is caught and is reset falls again at the same speed from the same place, so Kaboom's and Astrosmash's rhythm is different on every launch but repeats within one. There is no way yet to draw again on `<reset />`, and nothing in the language creates a new object while the game runs, so things that fall are a fixed set that cycles.
- A hop is a jump, not a slide: the object is simply one step away, with no animation between and nothing touched on the way (a step is meant to be one lane). It is not the arcing jump planned in [designs/13](designs/13-verb-vocabulary.md).
- `carry()` lends velocity one frame at a time and only while touching. It is not attached: a carried object that meets a screen edge is handled by that edge's rule like any other (Frogger's frog loses a life), and nothing pushes it into a wall.
- A pair in which nothing moves is not looked at (see the collision rules above), so a rule against something that stands still runs only when the object moves, is carried, or hops.
- `unless` names a class, not a single object, and only exists on object-against-object rules.
- `wrap()` assumes what it wraps is spaced with its own size in mind (a lane of things that are `size` wide repeats every window width plus `size`), and the members of a lane (a `<group>`) are placed by hand, one `<x>` each: there is no way yet to say "this many, evenly spaced". A `<grid>` gives one velocity and one spacing to all its cells.
- Only the object-against-object test is swept. A screen edge is still checked by position before the move, so an object that moves more than a whole window's width in one frame is not caught by it; rotation and acceleration are not modeled. A shape that is not a rectangle or circle (text, image) is treated as its bounding box.
- Expressions inside a value are not checked by the schema (they are text); a typo in one is a runtime error (the engine reports it and exits), not a validation error. Structure, tag names, attributes and command verbs are checked.
- The weak validator (`xsd_lite`) covers only the XSD subset this project uses; Xerces is the full check.
- Window sizes (`width`, `height`, `framerate`) are ordinary values in the file but are read once, before the window opens.
