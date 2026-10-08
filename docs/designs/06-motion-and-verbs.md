# 06. Motion and the verb vocabulary

**Status:** built: constant velocity, acceleration, thrust, headings, hop, straight timed jump, riding, wrapping, deflect, paths of straight steps (Galaxian), standing on platforms, a fixed-arc leap and ladders (Donkey Kong). chasing and aiming (`<chase>`, `<aim>`). Ideas: air control beyond none, curved paths and moving formations, AI targeting beyond straight at a target.

## Principle

- A verb is a named behavior with parameters. A good one captures a *behavior space* (what a thing does), not an implementation. Shared words (bounce, die) name expectations and hide the machinery (detection, normals, restitution, clamping).
- Grow the set only when a game cannot be described without it; keep each verb a behavior. **Danger sign:** a verb that exists to mean "behave like game X" shows the parameters were not found.
- Test for any new family (jump, motion): can the language describe game A, B and C with the same words and different values, without a word that means one of them?

## What is built, and what forced it

| Verbs and tags | Forced by | Decision |
|---|---|---|
| `bounce`, `stick`, `die`, `reset`, `inc` | Pong, Breakout, Invaders | |
| `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, more verbs in object rules, more colors | Frogger | lives count down; hop is one instant step per press; wrap puts an object back only once fully off screen; riding and exceptions in [05](05-collisions.md) |
| `<line>`, `pixel`, `<acceleration>`, `<accelerate burn>`, `<stop />`, `slower`/`faster` | Lunar Lander | below |
| `<heading>`, `<turn>`, `<thrust>`, `<drag>`, `<hidden>`, `<release>`, `<fire>` along a heading, amount on `inc`/`dec` | Asteroids | below |
| `<deflect>` | Pong | [05](05-collisions.md) |
| `<timers>`, `<facing>`, `<jump>`, `<reverse />`, looks, `<reveal>`, key sets | a read of every game for workarounds | [08](08-timers-and-enemy-behavior.md) |
| (none) | Space Race, Kaboom v1, Freeway, Depth Charge, Astrosmash | the vocabulary was already enough ([10](10-games-as-tests.md)) |
| `<paths>`, `<follow>` with `<stagger>` | Galaxian | below |
| `<land />`, `<leap>`, `<climb>`, `<acceleration>` in a group | Donkey Kong | below |
| `<chase>`, `<aim>` | Berserk's Otto and robots | below |

## Gravity, thrust and landing (Lunar Lander)

- **Gravity is a property of the object**, not a world setting: `<acceleration>` added to velocity each frame before anything moves. A `gravity` world variable would pull everything; most games want some things to float. `<stop />` removes it, a reset gives it back.
- **Thrust is a held verb that names a variable to burn:** `<accelerate direction burn="fuel">` adds to velocity (not sets it) every frame the key is down, takes 1 off the variable, and does nothing at 0. Opposite thrusters cancel. Rejected: thrust as a plain rule on the key (needs "while held" and "until empty" spelled out).
- `<stop />` zeroes velocity, acceleration and held thrust so a landed lander stays landed.
- Landing vs crash: `slower`/`faster` ([05](05-collisions.md)).
- No world-space rotation in Lunar Lander: it has side thrusters; rotation came with Asteroids.

## Headings and turning (Asteroids)

- **Heading** is degrees clockwise from straight up (0 up, 90 right); direction is (sin h, -cos h). No `<heading>` leaves an object unchanged. `<turn>` and `<thrust>` are held like `<move>` and `<accelerate>` and are a load error without a heading. `<drag>` (0 up to, not including, 1) is the fraction of speed lost per frame after thrust, so thrust has a top speed.
- **Pictures are turned by the engine, once per whole-degree change, not by backends** (details in [07](07-pictures-and-text.md)). Rejected: rotating in each backend (four backends to change, and the pixel test would still need the turned picture; filtering would make drawn and tested pixels differ per library).
- **Pools, not spawning.** See [08](08-timers-and-enemy-behavior.md). `<release object>N</release>` puts pool members in the middle of the object running the rule (a rock breaking); `<reveal>` puts them back where they started.
- `<inc>`/`<dec>` take an amount (scoring first repeated `<inc />`); a hidden object does not count toward `remaining`; a collision `<reset />` restores the heading; `<stop />` clears turn and thrust.
- Not done: shots that wrap and expire, a safe wait before the ship returns, a saucer, more than one wave.

## Steps, jumps and facing

- **`<hop>`**: one instant step per key press, judged where it lands. Not repeated while held, not resumed after a pause.
- **`<jump>`**: a straight timed move (`<distance>` over `<seconds>`) that touches nothing in the air and meets what it lands on; refused if it would land off screen. No arc, no air control. Exactly Frostbite's "land on the ice or in the water".
- **`<facing>`** (up/down/left/right, follows the last move, hop or jump, never turns the picture): `<fire>` leaves the middle of that side at the projectile's own speed. An object with no facing fires from its top as before. Chosen over a heading in 90 degree steps, which would turn the picture.

## Motion models (idea: names)

- Orders of motion: **position-driven** (a paddle set by input), **constant velocity** (rules rewrite velocity; Pong, bullets), **accelerated** (velocity accumulates; asymmetric up/down; grounded vs airborne; Donkey Kong onward). Everything up to the second fits a rule-driven engine; the third is built for one case per object (above) and fixes a frame order: acceleration and held thrust, then edge rules, then hops, then move and collisions.
- Public names for the two camps: **kinematic** and **dynamic** (leading candidate; finer labels such as event-driven vs integrated, analytic vs integrated, constraint-based, ballistic kept for docs). Rejected as misleading: Newtonian vs not, impulse-based as a top split.
- Many old games only *imitate* physics with tables and fixed arcs, so dynamic probably needs a mode "behave as if simulated" next to real integration.
- **Movement feel as data:** animation-locked (an input starts a fixed sequence; input during it is ignored or queued; movement as a puzzle of preparation) vs free control (input adjusts velocity every frame; forgiving). Forgiveness as separate optional tolerances (assistant suggestions, not decisions): grace period after leaving a ledge, a buffered jump press, auto ledge grab, pull toward a landing spot.

## Standing, leaping and climbing (Donkey Kong)

**What we looked for.** Donkey Kong: Jumpman and the barrels fall and stand on girders, a barrel rolls off a girder's open end onto the next, Jumpman jumps a barrel in a fixed arc he cannot steer, and climbs ladders through the girders.

**What we looked at.** (a) Standing as a property of the object (a "solid" flag on the platform, every falling thing stopped by it): a world rule, the same objection as a world gravity. (b) Standing as a collision verb, `<land />`, on the faller's rule for what it stands on. (c) Reusing `<stop />`: it takes the pull away, so nothing walks off an edge and falls. For the jump: (d) the straight `<jump>` with more children; (e) a new verb that gives an upward speed under the object's own pull. For ladders: (f) a collision rule with the ladder (it only runs while something moves, so a man resting on a ladder would lose it); (g) a held action that looks for the ladder itself.

**Chosen: (b), (e), (g).**
- *`<land />`* acts only on the faller's bottom edge against the other's top, and only when not going up; it puts it on the top, stops the fall and marks it on the ground until the next frame's landing. The pull keeps acting, so the object is in a touch every frame and walks off an open end on its own. Platforms are solid from above only (the usual one-way platform): walked past at the ends, jumped up through.
- *`<leap>height</leap>`*: the speed up is `sqrt(2 * pull * height)`, so the author gives the height, not a speed. Only from the ground. Air control is the first rung of the ladder below, *none*: the way across is kept until the landing, which takes up whatever keys are held then. The loader refuses a leap on an object with no pull down or no `<land />` rule.
- *`<climb direction class>step</climb>`*: held like a move; `Game::applyClimbing` finds an object of the class under the climber's middle with its feet between that object's top and bottom, gets on (from the ground only), lines it up, and moves it; no pull, no landing, no walking off until an end. A ladder's top is the upper platform's top and its bottom the lower platform's top.
- A move key now changes only its own axis (it used to recompute both, which stopped a fall whenever a walking key was pressed).
- `<acceleration>` in a `<group>`, so a pool of barrels can fall.

**Open.** Sloped girders (a platform at an angle: land on a line, not a box top). Barrels that take a ladder at random. Points for jumping a barrel (something has to notice the man passed over it). The hammer. Dying from too long a fall. Other rungs of air control (steer, brake, reverse) and a held, variable-height jump.

## Arcing jump (idea; the fixed arc is built as `<leap>`)

- **Two types** in the first plan: *static* (same height, length, arc and time every time; Castlevania; parameters height, width, time) and *dynamic* (height from how long the button is held, length from run speed, steering in the air; Super Mario Bros.; parameters max hold time, max height, upward acceleration, whether the player can slow, stall or go backward in the air).
- **Refinement (second batch): a jump model plus an air-control ladder**, per axis. The model is the vertical profile at takeoff (fixed, variable with button held, cut short on release). Horizontal air control is a ladder: *none* (trajectory fixed at takeoff), *steer*, *accelerate*, *brake* (reduce but not reverse), *reverse* (the Mario ledge trick: jump out, come back, land on the ledge above), *full*. Mega Man can stall but not reverse.
- **Movement as capabilities:** describe the ground (how input becomes speed: instant, or acceleration with friction) and the air (horizontal control from the ladder, vertical control fixed or variable, reversal allowed) rather than naming a jump type. A fixed-arc game says the air has no control; a free game says full.
- Open: are height, distance and duration separate inputs or consequences of a profile (start speed and pull: better for fixed, unavoidable for variable); jump as a verb or part of a movement component ([03](03-objects-groups-and-storage.md)); how an animation-locked jump interacts with states. Grounded vs airborne and a jump impulse now exist (`<land />`, `<leap>`); the rest of the ladder does not.

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

## Small value ideas

- Random speed with a dead zone: see [02](02-values-variables-and-names.md) (`<sign>`).
- Sound is covered in [09](09-sound.md).
