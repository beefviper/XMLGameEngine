# Known limitations

What the engine cannot do today. Why, and what might come, is in the [design files](../designs/00-designs.md); what is next is [the plan](../plan.md).

## Movement and collisions

- Movement is pixels per frame and timers count frames, so a slow machine slows the game.
- Only touches between objects are swept. Screen edges are checked by position, so something faster than a window's width a frame can miss one.
- A pair where nothing moves is not looked at, so a rule against something still runs only when the object moves, rides, hops or lands.
- A `pixel` touch reads its slope from four pixels round the point, so stairs of one-pixel steps are a slope. It is exact to half a pixel; lines meant to touch should be at least 2 pixels thick. Text and images cannot be `pixel`.
- `unless=` names a class, not an object. An edge rule cannot have a `<condition>` yet.
- `<ride />` lends velocity a frame at a time; it does not attach.
- `<wrap />` assumes a lane spaced with its objects' size in mind.

## Jumping, steering and paths

- Jumps are an instant step (`<hop>`), a timed straight line with an optional drawn arc, or a fixed arc under a pull (`<jump>`). No air control.
- `<chase>` and `<aim>` go straight, through walls, with no lead.
- A maze walker steers by the next tile nearest a target, with no search, so a target behind a wall can leave it circling. Pac-Man's ghosts keep the first level's schedule on every level.
- A path is straight legs at one speed. Enemies fire on timers from wherever they are (an invader behind others fires through them).
- Cell pieces jump a cell at a time; nothing glides. A `<go>` on cells keeps its speed until the next turn; a chain cannot wrap round the map.
- Cell checks cannot find touching groups of alike pieces (Puyo Puyo) or count a chain's rounds. No hold, lock delay, ghost piece or hidden rows; `<random />` is a plain pick, not a bag of seven.
- `<undo />` cannot bring back a copy despawned since (Doubles' merged numbers), so it suits pushing puzzles. A `<condition>` in an action holds no conditions. A spawn picks no number by weight, so Doubles brings in only 2s. Minefield's first square can be a mine. A look cannot be a live text.
- `<facing>` is one of four ways (diagonals only from two held keys) and never turns the picture.

## Pictures and sound

- Engine-drawn pictures are fixed at load (an SVG at one scale); they can be turned, swapped for another look, recoloured, mirrored and holed.
- An animation cannot run once and stop, and cannot be started by a collision. A sheet's cells take the animation's time, not one each. An object with a `<heading>` cannot take a palette swap.
- Turning keeps two pictures per object; a big drawing costs four bytes a pixel of its box. A picture (lines, polygons, a bitmap, a text, a cut from a sheet or an SVG) is at most 4096 pixels on a side; a larger one stops the load.
- The built-in 8x8 font is only a fallback, and its widths differ from the real font's.
- An object's own variable named `width` or `height` shadows its size.
- Music is a chip's: one wave a part, no envelope, no stereo. A recording is an uncompressed WAV only (no MP3 or Ogg), played in mono, at most 60 seconds. A state's looping music restarts after a pause and cannot be silenced while current. A sound is made when the window opens, so notes cannot follow a variable.

## Values

- Values outside a command, a condition's test or an object's `update="live"` number are fixed at load; a game `<variable>`, a velocity and a size cannot be live. A class condition cannot compare each object with its own other variable.
- No tangent, square root, power or "if" among the operations: a comparison is a 1 or 0 to multiply by.
- A `<divide>` by something not known at load gives 0; only positions and timer intervals are worked out again.
- Expressions are text to the schema, so a typo is found at load, not by validation. Without Xerces, the engine's own schema checker reads only the XSD constructs and built-in types the schemas use.
- The window's size and frame rate are read once.

## States, turns, demos

- Entering a scene draws no `<random>`s; a game-level `<random>` is drawn once.
- A state shown still is frozen whole, so a demo plays as a game of its own, not behind an attract screen. An object shown in two states runs in both.
- A state with scenes cannot be copied with `from=`, and a copied condition or timer can be replaced but not removed.
- Players taking turns see only their own game.
- A demo records keys, not the mouse or a stick; game-level `<random>`s do not follow its seed; an old demo plays its keys into a changed game.
- A table of the best has no place numbers unless each member's text has its own. A save holds numbers and looks, not places.

## Generating

- A generated program is the engine with the game built in, so it shares the engine's limits. It always carries lunasvg.
