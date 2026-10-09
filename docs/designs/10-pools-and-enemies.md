# Pools, enemy fire and key sets

**Status:** Built.

## Pools instead of spawning

- Nothing in the language creates an object while the game runs. Instead a `<group>` of members is built at load and **hidden** (`<hidden>true</hidden>`); things come into play from the pool:
  - `<fire object="shots" />`: first member whose collisions are off, from the shooter's nose or facing side; a group of four is four shots in flight. A shot is put away by `<die />`.
  - `<release object="rocks">N</release>`: into play centered on the object running the rule, at their own starting velocity (a rock breaking).
  - `<reveal object>N</reveal>`: back where they started (an igloo built a block at a time, a door, a fish that returns).
- **Why pools:** need no new object list at runtime, the tests see every member, and the pool size caps shots in flight as arcade games did. A pool member has the same velocity each release; with all in flight a timer that fires does nothing.
- Rejected for now: a runtime spawn verb (objects created while running). Fixed-size pools cover splitting rocks, a bomber dropping where he is (`<fire>` from his facing side), and an explosion where something died (a `<release>` from a pool in its rule). A spawn verb is needed only for counts with no upper bound.
- Show/hide as verbs were an earlier idea; `<reveal>` and `<die />` are that pair in pool form.
- **Fixed sets that cycle:** things that fall and come back (Kaboom, Astrosmash) are a fixed set reset to the top. `<random>` is drawn once at load, so a piece repeats its own fall; the rhythm differs per launch, not within one.


## Enemies fire back

- Enemies use the same `<fire>` with pools: an enemy is a shooter with a facing and a timer; its shots are a pool. No aiming. Space Invaders (both) and Demon Attack fire this way (Berserk's robots now `<aim>` first); the aliens' catch-all rule (`<collision><die /></collision>`) had to name the cannon's bullet so their own bombs do not kill them.
- Limits: an invader behind others fires through them (the arcade fires only the bottom one of a column); a Berserk robot aims at the man through walls (its shot dies on the wall). Aiming and chasing: [chase-and-paths](24-chase-and-paths.md#chasing-and-aiming-berserk).


## Key sets

- A named `<keys name="...">` set at the top of `<states>` holds `<input>`s; `<inputs keys="a b">` takes one or more (each over the one before) and the state's own `<input>`s go over them, key by key. One `button=` can name several keys, so W A S D and the arrows are one line each. Chosen over copying bindings.


## Open

Only the front invader of a column firing, an event queue, `<goto state>`, mutable game-wide variables (object variables did the job in every game), a timer's time left on screen, timers whose numbers follow variables beyond the interval.
