# Time

**Status:** Decided, not built. The work is the [delta time](../plan.md#delta-time) section of the plan.

## What we want

A game plays the same whatever the frame rate: at 8 frames a second, at 60, at 144 or 240, and when the frames come unevenly. Being tied to the frame rate was never intended; it is how the first games happened to be written.

## Decided by the author

- **A real step of the time that passed.** Each frame the engine measures the time since the last one and plays one step of that length. No fixed step behind the scenes, and no splitting a step into smaller ones.
- **Everything in the engine respects it:** movement, timers, animations, jumps, swings, cell ticks, held keys.
- **A game file writes per-second numbers.** `<velocity><x>300</x>` is 300 pixels a second.
- **Games that move in ticks** (Space Invaders' block, Snake) are still played and drawn every frame; only their own tick decides when they move.
- **Moved games behave about the same,** not pixel for pixel. Speeds can be tuned once it all works.

## Units

| Kind | Words | Written as | From a per-frame number |
|---|---|---|---|
| speeds | `<velocity>`, `<move>` held, a path's speed, `<chase>`, `<aim>`, `<hunt>`/`<go>`/`<wander>` off cells, `<seek>`'s most | pixels a second | times 60 |
| speed gained | `<acceleration>`, `<thrust>`, `<pull>`, `<jump>`'s pull, `<rise>`, `<fall>` | pixels a second, each second | times 3,600 |
| turning | `<turn>` held, `<seek>`'s degrees, a spin | degrees a second | times 60 |
| shares | `<drag>`, `<friction>`, `<grip>`, a `<surface>`'s | share lost each second, 0 up to 1 | `1 - (1 - share)` to the 60th power |
| counts | an `<inc>` or `<dec>` on a held key | added each second | times 60 |
| times | timers, animations, `<hop>`, `<swing>`, cell speeds | seconds, as today | unchanged |

- Live numbers read and set the same units: `ball.velocity.x` is pixels a second.
- A share is "lost each second" so that 0 is still none, more is still more, and the range check stays. `<drag>0.02</drag>` a frame is `0.70` a second.

## The sums

With steps of any length, the sums must give the same answer however a second is cut up.

- **Constant pull:** the position moves by `velocity * t + acceleration * t * t / 2`, then the velocity gains `acceleration * t`. A jump is the same height at any rate.
- **Shares:** what is kept over a step is `(1 - lost)` to the power `t`.
- **Easing toward a wanted speed** (`<grip>`): the same power, on the gap.
- **Drag with a pull** is not exact; the pull's sum is done first, then the share. Close enough at any rate a game is played at.

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
- **A recorded demo** played with `--frames` gets the steps it is told, so it is exact. In a window it is the open point: [below](#open).

## How the games are moved

For a while a game says which units it is in: `<game time="seconds">`. One without it is in the old units, and the engine plays it with its motion clock in sixtieths of a second, so every unmoved game already goes by time passed and plays as before at 60.

- Moving a game is multiplying its numbers by the table above and adding the attribute.
- Speeds kept in a game's own variables cannot be found by a script; they are found by reading the game and by its tests.
- When every game and every test's own game is moved, the attribute and the old clock are removed.

## What we looked at

- **A fixed step with per-second units,** the loop as it was: repeatable for free and no change to the sums. Rejected by the author: it is not real delta time, and a game would still be bound to its step.
- **Per-frame units kept, converted at load:** a number read or set while playing cannot know its units. Rejected.
- **Splitting a long step into short ones:** hides the problem and costs time when the machine is already slow. Rejected by the author.

## Open

- **A recorded demo.** A recording is only the same game if it is played back on the same steps. Either a demo keeps the length of every step, or recording and playback both run at one constant step kept in the demo, the one place a fixed step would remain. The author: the shipped demo is dropped for now and new ones are recorded once this is chosen.
- Whether a window with no `<framerate>` follows the screen's refresh or stays at 60.
- The exact sum for drag together with a pull, if a game ever shows the difference.
