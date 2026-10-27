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
| `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, more verbs in object rules, more colors | Frogger | lives count down; hop is one instant step per press; wrap puts an object back only once fully off screen; riding and exceptions in [collisions](20-collisions.md) |
| `<line>`, `pixel`, `<acceleration>`, `<accelerate burn>`, `<stop />`, `slower`/`faster` | Lunar Lander | below |
| `<heading>`, `<turn>`, `<thrust>`, `<drag>`, `<hidden>` (now `<active>`), `<release>`, `<fire>` along a heading, amount on `inc`/`dec` | Asteroids | below |
| `<deflect>` | Pong | [collisions](20-collisions.md) |
| `<knock>`, `<mass>`, `<drag>` in a group, circles against circles | Trick Shot, Mario Bros. | [collisions](20-collisions.md#contacts-and-knocks-2026-10-16) |
| `<timers>`, `<facing>`, `<jump>`, `<reverse />`, looks, `<reveal>`, key sets | a read of every game for workarounds | [timers](09-timers.md) |
| (none) | Space Race, Kaboom v1, Freeway, Depth Charge, Astrosmash | the vocabulary was already enough ([games-classic](54-games-classic.md)) |
| `<paths>`, `<follow>` with `<stagger>` | Galaxian | below |
| `<land />`, `<leap>`, `<climb>`, `<acceleration>` in a group | Donkey Kong | below |
| `<chase>`, `<aim>` | Berserk's Otto and robots | below |
| `<pull>`, `<seek>`, diagonal `<hop>`s, `<track path>` | the 2600 gap review: Yars' Revenge, Q*bert | [pulls and seeking](#pulls-seeking-and-diagonal-hops-2026-10-17) |


## Gravity, thrust and landing (Lunar Lander)

- **Gravity is a property of the object**, not a world setting: `<acceleration>` added to velocity each frame before anything moves. A `gravity` world variable would pull everything; most games want some things to float. `<stop />` removes it, a reset gives it back.
- **Thrust is a held verb that names a variable to burn:** `<accelerate direction burn="fuel">` adds to velocity (not sets it) every frame the key is down, takes 1 off the variable, and does nothing at 0. Opposite thrusters cancel. Rejected: thrust as a plain rule on the key (needs "while held" and "until empty" spelled out).
- `<stop />` zeroes velocity, acceleration and held thrust so a landed lander stays landed.
- Landing vs crash: `slower`/`faster` ([collisions](20-collisions.md)).
- Lunar Lander first had side thrusters (`<accelerate>` left and right) and no rotation; since the 2026-10-19 rewrite it turns with `<heading>` and `<turn>` and its one engine is a `<thrust burn="fuel">` along the heading, as the arcade game did. `<accelerate>` stays for things that push along an axis.


## Headings and turning (Asteroids)

- **Heading** is degrees clockwise from straight up (0 up, 90 right); direction is (sin h, -cos h). No `<heading>` leaves an object unchanged. `<turn>` and `<thrust>` are held like `<move>` and `<accelerate>` and are a load error without a heading. `<drag>` (0 up to, not including, 1) is the fraction of speed lost per frame after thrust, so thrust has a top speed.
- **Pictures are turned by the engine, once per whole-degree change, not by backends** (details in [pictures](30-pictures.md)). Rejected: rotating in each backend (four backends to change, and the pixel test would still need the turned picture; filtering would make drawn and tested pixels differ per library).
- **Pools, not spawning.** See [timers](09-timers.md). `<release object>N</release>` puts pool members in the middle of the object running the rule (a rock breaking); `<reveal>` puts them back where they started.
- `<inc>`/`<dec>` take an amount (scoring first repeated `<inc />`); a hidden object does not count toward `remaining`; a collision `<reset />` restores the heading; `<stop />` clears turn and thrust.
- Not done: shots that wrap and expire, a safe wait before the ship returns, a saucer, more than one wave.


## Steps, jumps and facing

- **`<hop>`**: one instant step per key press, judged where it lands. Not repeated while held, not resumed after a pause.
- **`<jump>`**: a straight timed move (a distance, its content, over `<seconds>`) that touches nothing in the air and meets what it lands on; refused if it would land off screen. No arc, no air control. Exactly Frostbite's "land on the ice or in the water".
- **`<facing>`** (up/down/left/right, follows the last move, hop or jump, never turns the picture): `<fire>` leaves the middle of that side at the projectile's own speed. An object with no facing fires from its top as before. Chosen over a heading in 90 degree steps, which would turn the picture.


## Pulls, seeking and diagonal hops (2026-10-17)

- Need: the author asked for "pull towards something" as a fundamental, a heat-seeking missile that turns to its target, diagonal hops (Q*bert, 8-way) and a track an input moves along from 0 to 1.
- **`<pull object>` and `<seek object>` are properties, not verbs.** Each runs every frame (`pullAndSeek` in play/frame, after the frame's acceleration), as `<acceleration>` and `<drag>` do; a verb run by a timer, as `<chase>` is, would make a smooth pull or turn a staircase. Both look for the nearest of a name or group in play, as `<chase>` and `<aim>` do. Looked at: a `<homing>` with a speed of its own (the missile's velocity already says it), a pull that falls off with distance (inverse square: open, for a Spacewar sun), a range beyond which nothing pulls (open: Adventure's magnet).
- `<seek>` turns the way the object goes, keeping its speed, by at most so many degrees a frame (`turnedToward` in play/geometry), so a missile overshoots and comes round; its heading follows. Still, nothing turns.
- **Diagonal hops are direction words**, `up-left` and the rest, which pick (an attribute), with the step either one distance both ways or `<x>` and `<y>` (Q*bert's cubes are wider than they are tall). `Direction` stays four ways: a hop keeps the whole step and faces the up or down of it. Moves need nothing new: two keys held already go diagonally.
- **`<track path>`** puts the middle along a `<path>`'s legs by distance, from its `<start>`; the axis still says how far, 0 to 1. Kept on the input side ([input](08-input.md)) rather than a new kind of path.
- Open: keys moving along a track (a held key stepping the fraction), a pull's range and fall-off, seeking with a turning heading but no velocity (a turret: `<aim>` does that once), facing in eight ways.


## Hops that take time, and hunting by hops (2026-10-18)

- Need: Q*bert's hops are seen in the air, not one instant step, and Coily hops down the pyramid after Q*bert.
- **`<seconds>` in `<hop>`, chosen** (the author said `<time>`; `<seconds>` matches `<jump>`): the hop then flies as a jump does, sharing its code (`startFlight`): touches nothing in the air, a second press waits for the landing, refused off screen. Without it a hop is the instant step it was.
- **`<hunt object>` with a `<hop>` inside, chosen:** each time it runs, takes the one of the four hops (diagonal when the hop has `<x>`/`<y>`, else straight) whose landing is nearest the target, ties going up, then left. A verb run by a timer, so the hunter keeps its own pace; no maze needed. Looked at: a new verb `<pursue>` (it is still hunting, only by hops), and `<chase>` with steps (chase is a velocity).
- Open: an arc for a hop, a look that follows the hunt's direction (Coily's picture does not turn). A key pressed mid-hop no longer changes the look or plays the sound ([input](08-input.md#a-press-that-cannot-be-done-2026-10-18)).

## Motion models (idea: names)

- Orders of motion: **position-driven** (a paddle set by input), **constant velocity** (rules rewrite velocity; Pong, bullets), **accelerated** (velocity accumulates; asymmetric up/down; grounded vs airborne; Donkey Kong onward). Everything up to the second fits a rule-driven engine; the third is built for one case per object (above) and fixes a frame order: acceleration and held thrust, then edge rules, then hops, then move and collisions.
- Public names for the two camps: **kinematic** and **dynamic** (leading candidate; finer labels such as event-driven vs integrated, analytic vs integrated, constraint-based, ballistic kept for docs). Rejected as misleading: Newtonian vs not, impulse-based as a top split.
- Many old games only *imitate* physics with tables and fixed arcs, so dynamic probably needs a mode "behave as if simulated" next to real integration.
- **Movement feel as data:** animation-locked (an input starts a fixed sequence; input during it is ignored or queued; movement as a puzzle of preparation) vs free control (input adjusts velocity every frame; forgiving). Forgiveness as separate optional tolerances (assistant suggestions, not decisions): grace period after leaving a ledge, a buffered jump press, auto ledge grab, pull toward a landing spot.


## Small value ideas

- Random speed with a dead zone: see [values-and-names](04-values-and-names.md) (`<sign>`).
- Sound is covered in [sound](33-sound.md).


## Height and shadows (2026-10-17)

- Need: a ball in the air over a court (Tennis), things thrown or jumping over others seen from above; the author: "add whatever you think you need".
- **Chosen:** `<altitude>` with `<start>`, `<rise>`, `<fall>` and `<bounce>`, each optional. The position stays the point on the ground (so what it touches is under it, and edges and rules need nothing new); the drawing is raised by the altitude (`drawObject`). `altitude` and `rise` are live numbers, so `<set>` hits a ball up and a rule's `<check>` lets a high one go past; a bounce that would rise less than one frame's fall stops, so it comes to rest. `<shadow object=>` keeps an object under another, middle to middle, in play while it is.
- The names: `height` is already an object's size (`ball.height`), so the number is `altitude`.
- Looked at: making the position the drawn point and keeping the ground point apart (every rule about the ground would need the other one), a third axis in `Vector2f` (a change everywhere for one game), drawing the shadow by the engine (one more kind of picture; an object is simpler and can be any sprite).
- Open: collisions that depend on height by themselves (now a `<check>`), a shadow that shrinks as the thing rises, perspective (higher means smaller).
