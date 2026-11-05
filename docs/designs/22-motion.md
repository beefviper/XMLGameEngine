# Motion

**Status:** Built: constant velocity, acceleration, thrust, headings, hops, timed jumps with an arc, pulls and seeking, riding, wrapping, altitude and shadows. Reference: [commands](../reference/commands.md#moving), [objects](../reference/objects.md#parts-of-an-object).

## Principle

- A verb is a named behaviour with parameters. A good one captures a *behaviour space* (what a thing does), not an implementation. Shared words (bounce, die) hide the machinery.
- Grow the set only when a game cannot be described without it. **Danger sign:** a verb that means "behave like game X" shows the parameters were not found.
- Test for a new family: can the language describe games A, B and C with the same words and different values?

## What forced each word

| Words | Forced by |
|---|---|
| `bounce`, `stick`, `die`, `reset`, `inc` | Pong, Breakout, Invaders |
| `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless` | Frogger |
| `<line>`, `pixel`, `<acceleration>`, `<accelerate burn>`, `<stop />`, `slower`/`faster` | Lunar Lander |
| `<heading>`, `<turn>`, `<thrust>`, `<drag>`, `<release>`, amounts on `inc`/`dec` | Asteroids |
| `<knock>`, `<mass>` | Trick Shot, Mario Bros. |
| timers, `<facing>`, `<jump>`, `<reverse />`, looks, `<reveal>` | a read of every game for workarounds |
| `<paths>`, `<follow>` | Galaxian |
| `<land />`, `<leap>`, `<climb>` | Donkey Kong |
| `<chase>`, `<aim>` | Berserk |
| `<pull>`, `<seek>`, diagonal hops, `<track path>` | Yars' Revenge, Q*bert |
| nothing new | Space Race, Freeway, Depth Charge, Astrosmash |

## Gravity and thrust

- **Gravity is a property of the object** (`<acceleration>`), not a world setting: most games want some things to float.
- **Thrust is a held verb that burns a variable:** `<accelerate burn="fuel">` adds every frame while held and stops at 0. Rejected: a plain rule on the key (needs "while held" and "until empty" spelled out).
- `<stop />` clears velocity, acceleration and held thrust, so a landed lander stays landed.

## Headings

- **Heading** is degrees clockwise from up. `<turn>` and `<thrust>` are held and need a heading. `<drag>` is the share of speed lost each frame, so thrust has a top speed.
- **Pictures are turned by the engine**, not backends: four backends would change, and the pixel test needs the turned picture anyway ([pictures](30-pictures.md)).

## Steps and jumps

- **`<hop>`** is one instant step per press, judged where it lands; never repeated while held.
- **`<jump>`** is a straight timed move that touches nothing in the air; refused if it would land off screen. `<arc>` lifts it on the way. No air control.
- **`<seconds>` in a `<hop>`** makes it fly as a jump does (Q*bert), sharing `startFlight`.
- **`<facing>`** follows the last move; `<fire>` leaves the middle of that side. Chosen over a heading in 90-degree steps, which would turn the picture.
- **Diagonal hops are direction words** (`up-left`), with one distance or `<x>` and `<y>`.
- **`<hunt>` with a `<hop>` inside** takes the hop landing nearest the target, run by a timer at the hunter's own pace. Rejected: a new `<pursue>` verb, `<chase>` with steps (chase is a velocity).

## Pulls and seeking

- **`<pull>` and `<seek>` are properties, not verbs:** they act every frame, as acceleration does; a verb run by a timer would make a smooth pull a staircase.
- `<seek>` turns the way of travel by at most so many degrees a frame, keeping speed, so a missile overshoots and comes round.
- Open: a pull that falls off with distance (a Spacewar sun), a range (Adventure's magnet), a turret that seeks with no velocity.

## Height and shadows

- Tennis needs a ball in the air over a court. **`<altitude>`** (`<start>`, `<rise>`, `<fall>`, `<bounce>`) raises only the drawing; the position stays the point on the ground, so touches and edges need nothing new. `<shadow object>` keeps an object under another.
- `height` is already a size, so the number is `altitude`.
- Rejected: the position as the drawn point (every ground rule would need the other), a third axis everywhere, the engine drawing shadows.
- Open: touches that depend on height by themselves, shadows that shrink, perspective.

## Motion models (idea)

- Three orders: **position-driven** (a paddle set by input), **constant velocity** (rules rewrite it), **accelerated** (velocity accumulates). The third fixes a frame order: acceleration and thrust, then edge rules, then hops, then the move.
- Public names for the camps: **kinematic** and **dynamic**. Many old games only imitate physics with tables and fixed arcs, so dynamic may need a "behave as if simulated" mode.
- Movement feel as data: animation-locked against free control; forgiveness as optional tolerances (grace after a ledge, a buffered jump).
