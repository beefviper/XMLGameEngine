# XMLGameEngine: current design

This document describes what the engine does **today**, as read from the source on the `claude` branch on 2026-09-29. For the reasoning behind these choices, the alternatives that were considered, and ideas that are not built yet, see [designs/00-designs.md](designs/00-designs.md).

XMLGameEngine is a video game description language (VGDL) written in XML, plus a C++ engine that loads a game description and runs it. A game is one `.xml` file. The description is declarative: there are no loops and no `if` statements in it. Behavior comes from a fixed vocabulary of verbs (`bounce()`, `stick()`, `die()`, ...) that the engine knows how to carry out.

## Running a game

```
XMLGameEngine              # loads "pong"
XMLGameEngine breakout     # a bare name gets ".xml" appended
XMLGameEngine pong.xml     # a name containing "." is used as given
```

The file is looked for in the current directory first, then in `./games/`. If it is not found the program prints an error and exits. Shipped games: `games/pong.xml`, `games/breakout.xml`, `games/spaceinvaders.xml`.

The XML and window libraries are chosen in C++ (`Game(file, XmlBackend)` and `Engine(game, WindowBackend)`); `main.cpp` uses the defaults, Xerces and SFML3. There is no command-line switch for either yet. See [Backends](#backends).

## Game file layout

A game file has one `<game>` root with exactly four children, in this order, as enforced by [assets/xmlgameengine.xsd](../assets/xmlgameengine.xsd):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../assets/xmlgameengine.xsd">
  <window     ... />
  <variables> <variable ... /> ... </variables>
  <objects>   <object ...> ... </object> ... </objects>
  <states>    <state ...> ... </state> ... </states>
</game>
```

### `<window>`

| Attribute | Meaning |
|---|---|
| `name` | Window title |
| `width`, `height` | Size in pixels |
| `background` | A color name, e.g. `color.black` |
| `fullscreen` | `true` or `false` |
| `framerate` | Frames per second |

### `<variables>`

Global named numbers. Each `<variable name="..." value="..." />` becomes a constant that any expression in the file can use (for example `margin`, `ball.radius`). Names may contain dots. The schema declares `value` as an integer.

### `<object>`

An object is anything that can be drawn, moved, or collided with: a ball, a paddle, a score readout, a title. Attributes: `name` (required, how other elements refer to it) and `class` (optional, a group label that collision rules and conditions can match, like a CSS class).

Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<sprite src="..."/>` | yes | What the object looks like. `src` is an expression that calls a sprite function (see [Expression functions](#expression-functions)). |
| `<position x="..." y="..."/>` | yes | Starting position, in pixels from the top-left. `x` and `y` are expressions. |
| `<velocity x="..." y="..."/>` | yes | Starting velocity, in pixels per frame. Expressions, so `random.range(-7,7)` works. |
| `<collisions enabled="true\|false" [group="true"]>` | yes | Zero or more `<collision .../>` rules. See [Collisions](#collisions). |
| `<actions>` | no | Named actions the object can perform, each `<action name="up" value="move.up(step)"/>`. States bind keys to these names. |
| `<variables>` | no | Variables owned by this object, each `<variable name="score" value="0"/>`. Other expressions refer to them as `objectName.variableName`, e.g. `paddle1.score`. |

An object with `class="projectile"` starts invisible (used for bullets).

### `<state>`

A state is one screen: a menu, the playfield, a pause screen, a game-over screen. Attribute: `name`. Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<shows>` | yes | A list of `<show object="name"/>`. Only these objects are drawn and updated while the state is current. |
| `<inputs>` | yes | A list of `<input button="key" action="..."/>`. `action` is a command, e.g. `state('playing')` or `action('paddle1','up')`. |
| `<conditions>` | no | A list of `<condition .../>`, checked every frame. See [Conditions](#conditions). |

The first state in the file is the starting state. States form a **stack**: `state('name')` pushes a state, and `state()` with no argument pops back to the previous one.

## Expression functions

Attribute values that are expressions are evaluated by exprtk at load time. Ordinary arithmetic works (`window.width.center - ball.radius`). The engine adds these names.

**Constants**

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | window height, window width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| any global `<variable>` | its value |
| any `objectName.variableName` | that object's variable (available regardless of the order objects appear in the file) |

**Sprite functions** (used in `<sprite src>`); a trailing color is optional and defaults to `color.white`.

| Call | Draws |
|---|---|
| `shape.circle(radius [, 'color'])` | A filled circle |
| `shape.rectangle(width, height [, 'color'])` | A filled rectangle |
| `text('label' \| number, size [, 'color'])` | Text. A quoted first argument is a fixed label. An unquoted `owner.variable` (e.g. `paddle1.score`) is a live number that redraws when that variable changes. |
| `image('path' [, 'flip.horizontal'])` | An image file |
| `grid(columns, rows [, xPadding, yPadding], shape...)` | Repeats a shape as a grid of separate objects. Used for Breakout bricks and Space Invaders. |

**Random**

| Call | Result |
|---|---|
| `random.number(max)` | A random float from 0 to `max` |
| `random.range(min, max)` | A random float from `min` to `max` |

**Commands** (verbs; used in collision `action`, object `<action value>`, and state `<input>`/`<condition>` `action`). Several can be chained with `;`, e.g. `inc('paddle2.score');reset()`.

| Command | Where it is meaningful | Effect |
|---|---|---|
| `bounce()` | collision | Reverses velocity away from the touched edge |
| `stick()` | collision | Clamps the object inside the screen edge it touched and stops only the velocity heading into that edge; the other axis keeps going, so an object pressed against the bottom wall still slides left or right. Re-applied after the frame's move, so a stuck object never ends a frame outside the screen |
| `die()` | collision | Disables the object's collisions and hides it (a circular object is also stopped and parked off-screen) |
| `reset()` | collision | Puts the object back at its starting position |
| `reset()` | state input or condition | Full game reset: every object's position, velocity and variables go back to their starting values, and the state stack collapses to the first state |
| `reset('name')` | state input or condition | Resets that one object's position, velocity and variables |
| `inc('owner.variable')` | collision | Adds 1 to that variable and refreshes any text bound to it |
| `move.up(step)`, `move.down(step)`, `move.left(step)`, `move.right(step)` | collision, or an object `<action>` | In a collision: shifts the object (or its whole group) once. In an object action: sets a held-key velocity (see [Input](#input)). |
| `state('name')` / `state()` | state input or condition | Push a state / pop back |
| `action('object','name')` | state input | Runs one of that object's named `<action>`s |
| `fire('projectile')` | object action | Launches the named projectile object from the shooter's top-center, using the projectile's `speed` variable as vertical velocity |

**Colors:** `color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`. Any other name is fully transparent.

## Input

An `<input button="q" action="action('paddle1','up')"/>` names a key and a command. Objects never mention keys, and states never mention what an action does, so remapping a key means editing one attribute in one state.

Key names are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more listed in `source/keycode.cpp`.

While a key bound to `move.*` in an object action is held, that direction's step is recorded. Velocity is recomputed from all four directions on every key change, so holding Down and tapping Up cancels out and releasing Up resumes Down. Left/right and up/down are independent axes, so two keys can make a diagonal.

The current state decides what a held key means. When the state changes, keys that are still down stop driving whatever the old state bound them to, and start driving what the new state binds them to. In Breakout, holding Left in `playing` and pressing Space stops the paddle, because `paused` binds only Space; unpausing while Left is still down moves it again with no re-press, and a Left first pressed during the pause starts moving it on unpause. Letting go of a key always stops what it was driving. Only continuous bindings (an action's `move.*`) resume this way; state changes and one-shot commands such as `fire` run only when the key is actually pressed, so holding Space through the main menu does not pause the game. Alternatives that were considered are in [designs/10](designs/10-input-and-actions.md#held-keys-across-state-changes).

## Collisions

Each object has `<collision>` rules. A rule's `action` is a command chain. There are two kinds:

- **Screen edge:** `edge="left"`, `"right"`, `"top"`, `"bottom"`, plus the groupings `"vertical"` (top and bottom), `"horizontal"` (left and right) and `"all"`. Several rules that touch the same edge all run.
- **Object against object:** `basic="basic"`. Optional `class="..."` and `object="..."` narrow the rule to a kind of other object or one named object; with neither, it matches anything.

Detection (pure geometry, `CollisionDetector`) is kept apart from response (`CommandExecutor`). Two rectangles use an axis-aligned overlap test; if either is a circle, the circle's center is compared with the nearest point on the rectangle. Both return which edge was touched, and each side of the pair then sees the edge from its own point of view.

Rules that apply:

- Only objects that are **shown** in the current state and have `enabled="true"` take part.
- A pair is skipped unless at least one of the two is moving.
- `group="true"` gives all cells of a `grid()` object a shared group number. Group members never collide with each other, `move.*` in a collision moves the whole group, and a group hitting the left or right screen edge with `bounce()` moves the whole block (this is how the invaders march).

## Conditions

```xml
<condition class="paddle" variable="score" value="15" action="state('gameover')" />
```

Checked once per frame while the state is current. It fires when any object matching `class` and/or `object` (both optional, combined with AND) has a variable named `variable` that has reached `value` (greater than or equal). The first match runs its `action` and stops checking for that frame. The schema declares `value` as an unsigned byte, so thresholds are limited to 0-255.

## What happens when a game runs

1. **Parse and validate.** The XML backend loads the file. If the file names a schema, it is validated: Xerces does full XSD validation ("strong"); the other three backends use a small built-in validator for the subset of XSD this project uses ("weak", `xsd_lite`). `printGame()` reports which one ran.
2. **Evaluate.** exprtk evaluates every expression once. Objects, their variables, `grid()` cells and states are built. Nothing here needs a window.
3. **Open the window.** `Engine` creates the window backend and measures each object's real size for drawing and collisions, then pushes the first state.
4. **Loop.** Each frame: read key changes and run the current state's bindings for them; run collisions and conditions and then move every shown object by its velocity (per frame, not scaled by time); clear; draw shown objects; present.

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
| `game_xml.cpp` | Walk the parsed XML into raw window/variable/object/state data |
| `game_expr.cpp` | exprtk symbol table and evaluation of raw data into `Object`s and `State`s |
| `command.cpp` | Turn the token stream from exprtk into typed `Command`s |
| `game.cpp` | Objects, state stack, per-frame update, collision pairs, conditions, resets |
| `collision_detector.cpp` | Geometry only |
| `command_executor.cpp` | What each command does |
| `engine.cpp` | Frame loop and key handling |
| `object.h`, `states.h` | Data model |
| `window_*.cpp`, `xml_*.cpp`, `xsd_lite.cpp` | Backends and the weak validator |
| `tests/` | Catch2 tests: collision geometry, command parsing, conditions, input resolution, `stick()`, engine key handling, object variables (opt-in with `BUILD_TESTING`) |

## Known limitations

- Object-object collision knows only the four edges of the other object; there are no verbs beyond the table above (no jump, gravity, shooting patterns, AI, sound).
- Object names need not be unique: every cell of a `grid()` shares the grid object's name, so a single brick cannot be addressed.
- Movement is in pixels per frame with no acceleration and no time step.
- `CollisionDetector::circleRectangle` picks the touched edge with conditions that compare a coordinate against a rectangle edge minus that same coordinate (for example `midpoint.y > rectTop - midpoint.y`), which does not look geometrically meaningful; it happens to work for the shipped games but has not been proven correct.
- Function-call syntax inside attribute strings (`shape.circle(...)`, `bounce()`) hides structure from XSD and XSLT; the design notes discuss replacing it.
- Expressions and verbs inside attribute strings are not checked by the schema; a typo in one is a runtime error (the engine reports it and exits), not a validation error.
- The root `readme.md` still describes an earlier alpha (SFML/Xerces/exprtk only, collisions/scoring/win condition "missing"); this document is the current description.
