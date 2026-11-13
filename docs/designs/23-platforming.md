# Standing, jumping and climbing

**Status:** Built for Donkey Kong, Pitfall!, Mario Bros. and Q*bert: `<land />`, `<jump>`, `<climb>`, slopes, `<arc>`, `<grip>`. Air control is an idea. Reference: [moving](../reference/commands.md#moving).

## What we looked for

Donkey Kong: Jumpman and barrels stand on girders, a barrel rolls off an open end onto the next, Jumpman jumps in a fixed arc he cannot steer, and climbs ladders through the girders.

## What we looked at

- Standing: a "solid" flag on platforms (a world rule, the same objection as world gravity); **a collision verb, `<land />`, on the faller's rule**; reusing `<stop />` (takes the pull away, so nothing walks off an edge).
- The jump: more children on the straight timed move; **a new verb giving an upward speed under the object's own pull.**
- Ladders: a collision rule with the ladder (runs only while moving, so a man resting would lose it); **a held action that finds the ladder itself.**

## Chosen

- **`<land />`** acts only on the faller's bottom against the other's top, only when not going up, and marks it on the ground until the next frame. The pull keeps acting, so it walks off an open end by itself. One-way platforms: jumped up through.
- **`<jump>height</jump>`:** the speed up is `sqrt(2 * pull * height)`, so the author gives the height.
  - Only from the ground; no air control.
  - The loader refuses a jump with no pull or no `<land />` rule.
  - First called `<leap>`, while `<jump>` was a straight timed move used only by Frostbite; that move was a timed `<hop>` under another name, so Frostbite took `<hop>` and the platform jump took the word every 2D platformer uses.
  - A hop goes to a place (Frogger, Q*bert, Frostbite); a jump goes up and falls.
- **`<climb direction class>step</climb>`:** finds a ladder under the climber's middle, lines it up, moves it with no pull. **The step is a velocity**, so the sweep sees two climbers on one ladder. Rejected: a special pass for climbers.
- A move key changes only its own axis, so walking does not stop a fall.
- **Sloped girders are a layout:** `<slope>` drops each column so much, a staircase of pieces half a pixel apart, as the arcade drew them. Rejected: landing on a line at an angle.
- **Ladders of rungs:** a column of pieces; pieces stacked and lined up climb as one; a broken ladder is two short columns.
- **A climb a rule starts takes one ladder** and lets go at the end, so a barrel does not take every ladder after. Barrels choose by a `luck` variable drawn again at each ladder. Rejected: a random condition, a "take a ladder" verb.
- **Holding at a broken ladder's top:** a held climb holds where nothing the climber lands on is under any of it, rather than stepping off into the air. Rejected: marking ladders broken.
- **Jump points:** a condition sees a barrel near Jumpman while his jump lasts; his landing rule scores. Rejected: a second hitbox (an object has one).
- Found on the way: a spawned copy's rule conditions were never made; copies now take them.

## Arc and grip

- **`<arc>`** after a timed hop's `<seconds>`: the altitude follows `4 * arc * t * (1 - t)`, drawn up and over while what it touches moves straight. Q*bert's hops must land on a cube exactly, so a real pull was rejected.
- **`<grip>`** eases the speed toward what the held keys ask, that share a frame: a slow start and a slide. Skidding is read from it. On ground pieces, `<surface>` changes it ([tables and tracks](25-tables-and-tracks.md)).

## Jumps (idea)

- Two types: **static** (same arc every time; Castlevania) and **dynamic** (height from how long it is held, steering in the air; Super Mario Bros.).
- Refined into **a jump model plus an air-control ladder**: none, steer, accelerate, brake, reverse, full. Mega Man can stall but not reverse.
- Better still, **movement as capabilities:** describe the ground (how input becomes speed) and the air (how much control), not a jump type.
- Open: barrels choosing ladders by where Jumpman is, dying from too long a fall, the other three screens, a held variable-height jump, grip in the air.
