# Time

**Status:** Built. Every game, test game and stand-alone program plays one step a frame of the time that passed. Reference: [engine](../reference/engine.md).

## What we want

A game plays the same whatever the frame rate: at 8 frames a second, at 60, at 144 or 240, and when the frames come unevenly. Being tied to the frame rate was never intended; it is how the first games happened to be written.

## Decided by the author

- **A real step of the time that passed.** Each frame the engine measures the time since the last one and plays one step of that length. No fixed step behind the scenes, and no splitting a step into smaller ones.
- **Everything in the engine respects it:** movement, timers, animations, jumps, swings, cell ticks, held keys.
- **A game file writes per-second numbers.** `<velocity><x>300</x>` is 300 pixels a second.
- **Games that move in ticks** (Space Invaders' block, Snake) are still played and drawn every frame; only their own tick decides when they move.
- **Moved games behave about the same,** not pixel for pixel. Speeds can be tuned once it all works.

## Units

| Kind | Words | Written as |
|---|---|---|
| speeds | `<velocity>`, `<move>` held, a path's speed, `<chase>`, `<aim>`, `<hunt>`/`<go>`/`<wander>` off cells, `<seek>`'s most | pixels a second |
| speed gained | `<acceleration>`, `<thrust>`, `<pull>`, `<jump>`'s pull, `<rise>`, `<fall>` | pixels a second, each second |
| turning | `<turn>` held, `<seek>`'s degrees, a spin | degrees a second |
| losses | `<drag>`, `<grip>`, a `<surface>`'s | half-life: seconds to lose half the speed (0: at once) |
| counts | an `<inc>` or `<dec>` on a held key | added each second |
| times | timers, animations, `<hop>`, `<swing>`, cell speeds | seconds |

- Live numbers read and set the same units: `ball.velocity.x` is pixels a second.
- A loss is a half-life, chosen by the author over a share lost each second: strong grips (Speedway's tyres, 0.03 seconds) are a share so near 1 that a float cannot write it, while a half-life is exact at any strength. Longer is weaker, the other way from a share.
- `<friction>` is not a loss over time: it is how hard a still surface holds back at a touch, and stays as it is.

## The sums

With steps of any length, the sums must give the same answer however a second is cut up.

- **Constant pull:** the position moves by `velocity * t + acceleration * t * t / 2`, then the velocity gains `acceleration * t`. A jump is the same height at any rate.
- **Losses:** a half-life `h` is read as a rate `k = ln 2 / h` (0 is an infinite rate); what is kept over a step is `e^(-k t)`.
- **Easing toward a wanted speed** (`<grip>`): the same, on the gap.
- **Drag with a pull** is the exact sum of `v' = a - k v` over the step (`k` the drag's rate): Asteroids' thrust then came out 6% short at 8 steps a second, and the same at every rate with it. The sweep goes the step's true mean speed.

## Things that count time

- **Seconds, not frames.** A timer, an animation's picture, a hop, a swing and a held key's repeat each hold seconds left as a number, not a count of frames.
- **What is owed is paid.** A timer of 0.05 seconds in a step of 0.2 goes off four times, in order, and carries what is left over into the next step. An animation skips to the picture it should be on, running the commands of each it passed.
- **Ticks.** A `<stepped />` object and a cell piece's `<go>` keep their own clock. A long step makes each tick it owes, one after another. This is the tick's own rule, not the physics cut into pieces.

## Long steps

- **Touches between objects** are already swept across the whole move, and the rest of the step carries on after each touch. A long step needs nothing new there.
- **Screen edges** were checked by position after the move; they join the sweep, or a fast thing in a long step leaves the window between checks.
- **Anything that heads for a point** (a path's corner, a `<chase>`, a `<hunt>`) stops at the point and spends the rest of the step on what comes next, never past it.
- **The longest step.** A stall (the window dragged, a breakpoint) is not one giant step: a step is at most a quarter of a second, and below four frames a second the game slows down instead. The number is the author's to change.
- Rules and conditions are still asked once a step, so at 8 frames a second a game reacts 8 times a second.

## Staying repeatable

- **Tests give the step.** `Game` is stepped with a length, so a test that plays 600 steps of a sixtieth plays the same game every time, and the same tests can be run at other lengths and at uneven ones.
- **`xgecli --frames`** plays steps of a sixtieth unless told otherwise.
- **The board-game search** counts moves, not time.
- **A recorded demo** keeps its keys' times, not its steps. Played back, a step is split at each key inside it, so a key lands on time whatever the window's steps; on its own steps it is exact, and on others it drifts, slowly for most games.

## What we looked at

- **A fixed step with per-second units,** the loop as it was: repeatable for free and no change to the sums. Rejected by the author: it is not real delta time, and a game would still be bound to its step.
- **Per-frame units kept, converted at load:** a number read or set while playing cannot know its units. Rejected.
- **Splitting a long step into short ones:** hides the problem and costs time when the machine is already slow. Rejected by the author.
- **For demos, every step's length kept, or one constant step:** exact, but a line per step in the game file, or a fixed step after all. Rejected by the author: an attract-mode demo only has to look like play, so key times alone, with steps split at a key, were chosen. Measured over 30 seconds of random keys on steps of 9 to 24 ms, 37 of 59 games stayed within 8 pixels of the recording and most stayed the same game for all of it; Space Invaders and Pac-Man drifted within seconds.

## Open

- Recording the shipped games' demos again.
- Whether a window with no `<framerate>` follows the screen's refresh or stays at 60.
