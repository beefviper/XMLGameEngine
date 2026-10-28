# Timers

**Status:** Built. An event queue is open.

## Why (a read of every shipped game, 2026-10-03)

The same workarounds kept appearing, each standing in for something the language could not say:

- **No clock.** Frostbite's cold was a one-pixel sky-coloured object crossing the screen once a second so its edge rule could take a degree. Freeway and Space Race gave up timed rounds. No enemy could fire back. Kaboom's bomber was decoration, because nothing happens unless a key, a touch or a condition makes it.
- **No facing.** Berserk's man walks four ways but could only shoot up.
- **Copied key bindings** in every play state of a game with waves or rooms (key sets, [input](08-input.md#key-sets)).
- **Commands a context ignored** (`<inc>`/`<dec>` in a condition, `<reset object>` in a collision), so Kaboom kept an invisible `tally` object only so a condition could do the reset a collision could not.


## Timers

- **Chosen: commands on a schedule, in the places that already hold commands.** `<timers>` on an object (a group's are every member's) or a state; each `<timer>` has `<every>` (repeats) or `<after>` (once), in seconds (a value), then commands. An object's timers count while it is shown and in play, a state's while it is current (the same rule animations use), so a pause, a menu or a dead enemy stops them with no extra words. A reset starts them over, so does a push of the state (2026-10-28: an `<after>` in a state goes off once a visit, not once a game; a pop back to a state lets its timers carry on, so a pause does not restart them), and so (since 2026-10-19) does coming back into play by `<fire>`, `<release>`, `<reveal>` or `<spawn>`: a burst or shot that dies on a timer used to come back with whatever was left of its last count (Asteroids' saucer shots, Missile Command's bursts). Its animation starts over too.
- **The interval is worked out again each round, after the commands run** (`framesLeft = -1`): `<every><random><min>1</min><max>3</max></random></every>` waits a new time each round, which is what makes a bomber or an invader unpredictable without AI, and an expression can follow a variable the timer's commands change (a rate that speeds up). Expressions now see object variables live for this.
- "How much time has passed" is a timer that `<inc>`s a variable, shown by a bound `<number>` and tested by a condition: a visible clock from parts that exist. A separate time value in expressions was rejected: most values are worked out at load, so it would read load time almost everywhere.
- **Frames, not wall time:** seconds become frames with `<framerate>` (`Game::framesFor`); a timer never waits less than one frame; a machine that cannot keep up slows the clock with the game. A timer's time left is shown by naming it (below).
- **Look timers (Pac-Man):** `<timer sprite="blue">` counts only while its object shows that look, and starts over whenever a `<become>` gives it that look, even the one it already shows. Frightened ghosts turn back after six seconds, and a second pellet starts the six again, with no variables. `<after>0</after>` is allowed: it goes off on the next frame (a ghost starts hunting the moment it turns normal).
- Rejected for now: a global **event queue** with named events raised by rules and timers and handled in states (a second way to say "when X, do Y" next to rules and conditions; the likely next step if timers and conditions start chaining). Every shipped event is "after a while" or "when this touches that".

- **Time left (2026-10-24):** an object's `<timer name="level">` is also the object's number `energy.level`, its seconds left rounded up to whole seconds, written by `Game::showTimeLeft` each frame it counts. Need: Megamania's energy was a variable a second timer took down and a condition watched, three parts for one countdown; a game clock or a fuse on screen had the same three. Chosen over a new name form (`energy.timer.level`, or a `<left timer>` text piece): as an ordinary number of the object, text, conditions, checks and values read it with nothing new. Whole seconds rounded up, because the numbers shown are what it is for (a fractional one prints six decimals) and a countdown reads 3, 2, 1. Only an object's timers: a state or a scene has no object to hold the number, and a sprite-less object of the state counts while that state is current, as the state's timers do. Open: setting it (a `<set>` is overwritten the next frame it counts; making it a load error, or making it move the timer, are both possible), time left finer than a second (a bar that slides smoothly).

## Commands work where they make sense

Conditions and keys run `<inc>`, `<dec>`, `<become>`, `<reveal>`; collisions run `<reset object>`, `<reverse />`, `<become>`, `<reveal>`; an action can do a bare `<reset />` (object back to start), so a condition can send the player home through a `<trigger>`. Kaboom's `tally` and Frostbite's clock object are gone. The loader checks the names they use. Since 2026-10-27 the schema says which place takes which command, in nine groups ([file-format](02-file-format.md#commands-in-groups)).
