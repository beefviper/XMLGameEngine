# Chasing, aiming and paths

**Status:** Built: `<chase>`, `<aim>`, paths of straight steps. Curved paths and other AI strategies are ideas.

## Chasing and aiming (Berserk)

**What we looked for.** The author's ask: "chase to get close, then aim to target, then fire". Berserk's Evil Otto should come for the man, and its robots should shoot at him, not along the way their group faces.

**What we looked at.** (a) A behavior property on the object (`<ai targeting="direct" />`, below): it runs every frame and needs a frame order of its own. (b) Verbs that act once, run by a timer or a rule: the timer says how often the chaser looks again, which is also how sluggish it is. (c) Aiming as a `<fire at="...">` attribute: one verb, but a shooter could not face its target without firing.

**Chosen: (b).**
- *`<chase object><speed/><near/></chase>`* sets the velocity straight at the middle of the nearest one in play of that name or group, at the speed, or 0 within `near`; it faces that way. Two numbers, so two elements, as `<jump>`. In an object's timer or a collision with another object.
- *`<aim object />`* stores the way to the target's middle; the next `<fire>`s go along it from the shooter's middle, at any angle, at the projectile's own speed; facing (or a heading, exactly) turns too. A key that moves, hops or jumps the shooter, a reset, or another aim replaces it. Same places as `<chase>`.
- The target is the nearest one of a name or group in play, so a pool of players, or one of several, works the same.
- Berserk: Otto chases the man ten times a second at 1.1 over the walls; every robot aims before it fires.

**Open.** Chasing round walls (pathfinding), leading a moving target, aiming only in 8 or 4 ways (Berzerk's robots fire along 8), a condition on distance ("when near, fire"), the Pac-Man strategies below.


## AI targeting (idea)

- Maze-game AI decomposes into a few strategies with parameters. The Pac-Man ghosts: Blinky chases directly; Pinky targets a few tiles ahead of the player's direction; Inky uses the player's and Blinky's positions to flank; Clyde chases when far and retreats to a corner inside a threshold. Sketch: `<ai targeting="direct" />`, `"ahead" lookahead=4`, `"flank" reference="blinky"`, `"proximity_flee" threshold=8 fallback="corner"`. Pathfinding is the engine's job. Other modes: predictive, random, territorial.
- Berserk's robots were the first need; the smallest step, a verb that sets a velocity (`<chase>`) or aims a `<fire>` (`<aim>`) from another object's position, is built (above).


## Paths and formations (Galaxian)

**What we looked for.** Galaxian's aliens fly in, hold a formation, then break away and dive: scripted motion, neither constant velocity nor physics, and the same script flown by many objects. The originals used stored tables of positions or velocity steps, math for loops, tiny command languages, and a script per entrance ending in an empty formation slot.

**What we looked at.** Four ways to write a path, all able to sit behind one interface ("where is it at time t"): a **table of steps** (move this far, then this far), a **parametric curve** (x and y as expressions of time; fits the evaluator), **Bezier** curves (facing from the derivative), and a **command sequence** (turtle: forward, turn, repeat).

**Chosen: the table of steps, built first because it is the simplest.**
- A named `<path>` in the game's `<paths>`, written once and flown by any object (`<follow path="...">`), as the sketch here had it. Its numbers are content: a `<speed>`, an optional `<start>`, then `<step>`s (an `<x>` and a `<y>`, relative to where the step began) and `<home />`.
- **Relative steps, not points.** The same dive is flown from any place in the formation, and a `<wrap />` in the middle of a step does not upset it, because what is left of a step is a distance, not a target. Galaxian's dive uses that: its steps add up to the window's height plus an alien's, which is exactly what the wrap takes off, so a diver that misses comes back from the top to its own place.
- **Home is the formation.** An object's `<position>` is its slot; `<home />` flies back to it from wherever the object is. A formation is then just the objects' places, and needs no grid of slots or "go to slot" verb.
- **A step can run commands** as it sets off (the follower's own, as on its timer), which is how a diver drops bombs on the way down without a timer that fires in formation too.
- **A path sets the velocity**, each frame, and the object moves, wraps and collides as anything else does; nothing about collisions changed.
- **`<stagger>` on `<follow object=...>`** sends a group one behind another, each waiting at the path's start: a line of aliens flying in. An object already on a path ignores another `<follow>`, so an alien's dive timer simply does nothing while it is still flying in.
- Rejected for now: absolute points (a dive would need a copy per slot), a `<wait>` leg, a delay before the first of a stagger (the five rows fly in at once, on five paths), aiming.

**Open.** Curves (a Bezier or a parametric leg) for smooth loops; steps that ease in and out; a formation that sways (home would have to move with it); a dive that aims at the player; Galaga's capture beam. The generator does not know paths yet.
