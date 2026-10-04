# 08. Timers, pools, enemies and key sets

**Status:** built (timers, facing fire, pools, `<reveal>`, key sets, commands that work in every context). Aiming and chasing, an event queue and `<goto state>` are open.

## Why (a read of every shipped game, 2026-10-03)

The same workarounds kept appearing, each standing in for something the language could not say:

- **No clock.** Frostbite's cold was a one-pixel sky-coloured object crossing the screen once a second so its edge rule could take a degree. Freeway and Space Race gave up timed rounds. No enemy could fire back. Kaboom's bomber was decoration, because nothing happens unless a key, a touch or a condition makes it.
- **No facing.** Berserk's man walks four ways but could only shoot up.
- **Copied key bindings** in every play state of a game with waves or rooms.
- **Commands a context ignored** (`<inc>`/`<dec>` in a condition, `<reset object>` in a collision), so Kaboom kept an invisible `tally` object only so a condition could do the reset a collision could not.

## Timers

- **Chosen: commands on a schedule, in the places that already hold commands.** `<timers>` on an object (a group's are every member's) or a state; each `<timer>` has `<every>` (repeats) or `<after>` (once), in seconds (a value), then commands. An object's timers count while it is shown and in play, a state's while it is current (the same rule animations use), so a pause, a menu or a dead enemy stops them with no extra words. A reset starts them over.
- **The interval is worked out again each round, after the commands run** (`framesLeft = -1`): `<every><random min="1" max="3" /></every>` waits a new time each round, which is what makes a bomber or an invader unpredictable without AI, and an expression can follow a variable the timer's commands change (a rate that speeds up). Expressions now see object variables live for this.
- "How much time has passed" is a timer that `<inc>`s a variable, shown by a bound `<number>` and tested by a condition: a visible clock from parts that exist. A separate time value in expressions was rejected: expressions are worked out at load, so it would read load time almost everywhere.
- **Frames, not wall time:** seconds become frames with `<framerate>` (`Game::framesFor`); a timer never waits less than one frame; a machine that cannot keep up slows the clock with the game. No way to show a timer's time left except a timer counting a variable down.
- Rejected for now: a global **event queue** with named events raised by rules and timers and handled in states (a second way to say "when X, do Y" next to rules and conditions; the likely next step if timers and conditions start chaining). Every shipped event is "after a while" or "when this touches that".

## Commands work where they make sense

Conditions and keys run `<inc>`, `<dec>`, `<become>`, `<reveal>`; collisions run `<reset object>`, `<reverse />`, `<become>`, `<reveal>`; an action can do a bare `<reset />` (object back to start), so a condition can send the player home through a `<trigger>`. Kaboom's `tally` and Frostbite's clock object are gone. The loader checks the names they use. The schema still does not say which context may hold which command ([01](01-vision-and-format.md)).

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

- Enemies use the same `<fire>` with pools: an enemy is a shooter with a facing and a timer; its shots are a pool. No aiming. Space Invaders (both), Demon Attack and Berserk's robots fire this way; the aliens' catch-all rule (`<collision><die /></collision>`) had to name the cannon's bullet so their own bombs do not kill them.
- Limits: an invader behind others fires through them (the arcade fires only the bottom one of a column); a Berserk robot fires along its group's facing whether or not the man is that way; nothing aims or chases. Needs a verb that sets a velocity or aims a `<fire>` from another object's position ([06](06-motion-and-verbs.md)).

## Key sets

- A named `<keys name="...">` set at the top of `<states>` holds `<input>`s; `<inputs keys="a b">` takes one or more (each over the one before) and the state's own `<input>`s go over them, key by key. One `button=` can name several keys, so W A S D and the arrows are one line each. Chosen over copying bindings.

## Open

Aiming (fire toward the player), only the front invader of a column firing, an event queue, `<goto state>`, mutable game-wide variables (object variables did the job in every game), a timer's time left on screen, timers whose numbers follow variables beyond the interval.
