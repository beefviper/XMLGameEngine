# 21. Frogger and the vocabulary it added

**Status:** built (`games/frogger.xml`, with the verbs below)

## Why Frogger

The first three games share one core: a ball or a bullet, a paddle or a shooter, and things that die or bounce. None had a hazard that hurts the player, objects that loop round the screen, or objects that carry another. Frogger (1981) needs all three, plus lives and one-step movement, in a game that is otherwise small: about sixty rectangles, two dozen of them moving. It is the first game chosen to *test the vocabulary* ([13](13-verb-vocabulary.md)): the rule was to add a verb only where the game could not be described without it, and to keep each verb a behavior, not an implementation.

## The game as described

A grid of 48-pixel cells, 13 across and 14 down: a row for the score and lives, a row of home pads between hedges, five lanes of river, a strip of grass, five lanes of road, and the pavement the frog starts on. Everything is a rectangle drawn 6 pixels in from its cell (so the frog fits a lane with room to spare), which keeps every position a small sum of `cell` and `inset`. A truck is two cells long, a log three to five. Cars, trucks and logs are each their own object, spaced by hand: a lane has a different spacing from every other, which a `<grid>` (one spacing, one velocity) cannot say. The scenery (river, grass, road, lane markings, pavement) is more rectangles, drawn first because objects are drawn in file order.

## What it needed

| Need | Chosen | Options considered |
|---|---|---|
| Lives that end the game at zero | `dec()` beside `inc()`, and a condition form `atmost` | Count deaths up and end at 3 (hides the number of lives in the condition); comparison operators (`remaining="<3"`) instead of names; a separate lives object |
| One step per key press, however long it is held or paused | `hop.up/down/left/right(distance)`, queued by the press and made at the start of the next move | A held `move.*` stopped by a timer; a flag on `<input>` saying "once"; a hop that is a velocity for one frame (swept along the way); `move.*` in a collision (needs something to collide with) |
| Lanes that loop | `wrap()` on the screen-edge rule: acts only once the object is completely off the screen and still heading that way, and keeps any overshoot | `reset()` at the far edge (every object jumps back to its own start position, and appears whole); teleporting the moment an edge is touched (pops in whole); duplicating each object at both ends |
| Riding a log | `carry()` in a rule about another object: lends that object's velocity for the frame | Copy the velocity into the object's own (has to be undone when it steps off, and fights the hop and the keys); attach as a child (an ownership model the language does not have); move it by a position offset (bypasses the sweep) |
| The river kills the frog unless it is on a log | `unless="logs"` on the rule: passed over while the object is also touching something of that class | Rule order among the frog's rules (they are separate pairs, so order does not compose); a "safe" class that overrides a "hazard"; water only in the gaps between logs (moving and re-shaped every frame); an object-level condition counting overlaps |
| What touching a hazard or a pad does | `reset()`, `inc()`, `dec()` and `move.*` in rules about another object (they were ignored there) | A second verb set for object rules |
| A frog left sitting in a home | A bright green frog under every pad; when the pad `die()`s, the frog is seen | `show()`/`hide()` verbs; swapping the sprite (there is no way to change one) |
| Scenery colors | More named colors: grey, darkgrey, lightgrey, brown, orange, purple, darkblue, darkgreen, forestgreen | `#rrggbb` literals as a color (probably where this ends up; the named set is what fits the current way of resolving a color) |

A few reasons behind the choices worth keeping:

- **A hop is judged where it lands.** Swept along its path, a frog hopping onto a log from the grass would first touch the river at its edge, before the log is under it, and be lost on the way. A step is one lane, so nothing is skipped by not sweeping it.
- **Carrying is recomputed every frame** and never stored, so there is no state to undo: when the frog steps off, or a log runs out from under it, nothing carries it. The same reason lets `wrap()` and the screen-edge rules use the carried velocity without special cases.
- **The river is one wide static object**, which is what makes `unless` the right shape: the rule says what the water does and the exception says what makes it safe, in the file, in one line.
- **A frog carried off the side dies at the edge** by an ordinary `edge="horizontal"` rule on the frog, so the game needs no rule for it beyond what it already had.

## Approximations

Frogger as described here is the game without its extras: no diving turtles, crocodiles, snakes or flies; no timer; one point for each frog home, because `inc()` adds one (a way to name an amount is an idea); no speed-up between levels; a lost frog goes back to the start at once, with no animation; and a filled home can be entered again without harm (the frog under the pad is drawn there, and nothing tells the game it is taken). The lanes' hand-set spacing repeats every window width plus the object's size, so a lane is laid out with that in mind.

## Ideas that came out of it

- A way to name an amount for `inc`/`dec` (`inc('frog.score', 10)`), or a variable that changes by an expression.
- A `<grid>` with a velocity per row, or a list of positions, for lanes.
- Show and hide as verbs, so a state or a rule can bring an object back without layering.
- A rule that must be looked at every frame even when nothing moves (see the known weaknesses in [11](11-collision-detection-and-response.md)).
- A general way to say where in a sequence of rules a touch falls, if `unless` turns out not to be enough (compare [12](12-collision-escalation.md)).
