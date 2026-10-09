# Timers

**Status:** Built. An event queue and `<goto state>` are open.

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

Conditions and keys run `<inc>`, `<dec>`, `<become>`, `<reveal>`; collisions run `<reset object>`, `<reverse />`, `<become>`, `<reveal>`; an action can do a bare `<reset />` (object back to start), so a condition can send the player home through a `<trigger>`. Kaboom's `tally` and Frostbite's clock object are gone. The loader checks the names they use. The schema still does not say which context may hold which command ([vision](01-vision.md)).
