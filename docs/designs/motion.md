# Motion

**Status:** Built: constant velocity, acceleration, thrust, headings, hop, straight timed jump, riding, wrapping.

## Principle

- A verb is a named behavior with parameters. A good one captures a *behavior space* (what a thing does), not an implementation. Shared words (bounce, die) name expectations and hide the machinery (detection, normals, restitution, clamping).
- Grow the set only when a game cannot be described without it; keep each verb a behavior. **Danger sign:** a verb that exists to mean "behave like game X" shows the parameters were not found.
- Test for any new family (jump, motion): can the language describe game A, B and C with the same words and different values, without a word that means one of them?


## What is built, and what forced it

| Verbs and tags | Forced by | Decision |
|---|---|---|
| `bounce`, `stick`, `die`, `reset`, `inc` | Pong, Breakout, Invaders | |
| `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, more verbs in object rules, more colors | Frogger | lives count down; hop is one instant step per press; wrap puts an object back only once fully off screen; riding and exceptions in [collisions](collisions.md) |
| `<line>`, `pixel`, `<acceleration>`, `<accelerate burn>`, `<stop />`, `slower`/`faster` | Lunar Lander | below |
| `<heading>`, `<turn>`, `<thrust>`, `<drag>`, `<hidden>`, `<release>`, `<fire>` along a heading, amount on `inc`/`dec` | Asteroids | below |
| `<deflect>` | Pong | [collisions](collisions.md) |
| `<timers>`, `<facing>`, `<jump>`, `<reverse />`, looks, `<reveal>`, key sets | a read of every game for workarounds | [timers](timers.md) |
| (none) | Space Race, Kaboom v1, Freeway, Depth Charge, Astrosmash | the vocabulary was already enough ([games-classic](games-classic.md)) |
| `<paths>`, `<follow>` with `<stagger>` | Galaxian | below |
| `<land />`, `<leap>`, `<climb>`, `<acceleration>` in a group | Donkey Kong | below |
| `<chase>`, `<aim>` | Berserk's Otto and robots | below |


## Gravity, thrust and landing (Lunar Lander)

- **Gravity is a property of the object**, not a world setting: `<acceleration>` added to velocity each frame before anything moves. A `gravity` world variable would pull everything; most games want some things to float. `<stop />` removes it, a reset gives it back.
- **Thrust is a held verb that names a variable to burn:** `<accelerate direction burn="fuel">` adds to velocity (not sets it) every frame the key is down, takes 1 off the variable, and does nothing at 0. Opposite thrusters cancel. Rejected: thrust as a plain rule on the key (needs "while held" and "until empty" spelled out).
- `<stop />` zeroes velocity, acceleration and held thrust so a landed lander stays landed.
- Landing vs crash: `slower`/`faster` ([collisions](collisions.md)).
- No world-space rotation in Lunar Lander: it has side thrusters; rotation came with Asteroids.


## Headings and turning (Asteroids)

- **Heading** is degrees clockwise from straight up (0 up, 90 right); direction is (sin h, -cos h). No `<heading>` leaves an object unchanged. `<turn>` and `<thrust>` are held like `<move>` and `<accelerate>` and are a load error without a heading. `<drag>` (0 up to, not including, 1) is the fraction of speed lost per frame after thrust, so thrust has a top speed.
- **Pictures are turned by the engine, once per whole-degree change, not by backends** (details in [pictures](pictures.md)). Rejected: rotating in each backend (four backends to change, and the pixel test would still need the turned picture; filtering would make drawn and tested pixels differ per library).
- **Pools, not spawning.** See [timers](timers.md). `<release object>N</release>` puts pool members in the middle of the object running the rule (a rock breaking); `<reveal>` puts them back where they started.
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


## Small value ideas

- Random speed with a dead zone: see [values-and-names](values-and-names.md) (`<sign>`).
- Sound is covered in [sound](sound.md).
