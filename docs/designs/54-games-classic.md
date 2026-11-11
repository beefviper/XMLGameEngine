# Games as tests: the first games

**Status:** Every game in `games/descriptions/` is built; most are played frame by frame in `xge/tests/games/test_<game>.cpp`. All were rewritten once with the newer vocabulary, closer to the originals in graphics, sound and play. The list: [the shipped games](../reference/games.md).

## What each game forced

| Game | Chosen to test | Forced (new) | Left out |
|---|---|---|---|
| Pong, Breakout, Space Invaders | the core: bounce, stick, die, fire, lockstep | `<deflect>`, bitmaps, animation, SVG sprites, enemy fire | Breakout's two-hit row and marching block (kept in tests) |
| Frogger | hazards, loops, riding, lives, one-step moves | `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, verbs in object rules, colours | crocodiles, lady frog, flies, speed-up |
| Space Race, Freeway | a second game on the same words | nothing | lives in Freeway (kept apart from Frogger) |
| Kaboom | `<random>` in a group; states that differ only in what they show | timers, `<facing>` fire, `<reset object>` in a rule | speed-up within a wave |
| Depth Charge | many charges against a clock | nothing | the charges' depth setting |
| Astrosmash | Kaboom's pieces plus a gun | nothing | levels, guided missiles, the UFO |
| Lunar Lander | gravity, limited thrust, landing by speed, pixel terrain | lines, pixel, acceleration, `burn`, `stop`, a condition on `speed` | scrolling close-up, abort, hard landings |
| Asteroids | facing, coasting, wrapping, breaking | heading, turn, thrust, drag, piece copies, despawn, a distance total, a thrusting pose | saucer shots breaking rocks |
| Pac-Man | a tile maze, steering, four ghosts | `<map>`, `<maze>`, `<go>`, `<hunt>`, `<wander>`, look timers, `<become from>` | later levels' schedules, other fruit |

## Frogger

| Need | Chosen | Rejected |
|---|---|---|
| Lives that end the game at zero | `dec` and `atmost` | counting deaths up (hides the number of lives) |
| One step per press | `hop`, queued and made at the start of the move | a held move stopped by a timer; a one-frame velocity (swept on the way) |
| Looping lanes | `wrap`: acts once fully off screen and heading out, keeps overshoot | `reset` at the far edge (pops in whole) |
| River kills unless on a log | `unless="logs"` | rule order |
| Diving turtles | two looks and a look timer; the turtle's `sprite="under"` rule drowns the frog | a separate hazard fading in |
| Bays that fill | the hedge is a hazard passed over on a bay; a bay has looks `empty` and `home` | a pad that dies |
| Time | a clock object's variable, counted down by a state timer | a timer on the frog |

Lanes are groups inside one `lanes` group that gives the `<wrap />`; each lays its cars out as columns spaced `(window.right + w) / n - w` round the loop.

## Kaboom

- The bucket stack is one object with three looks; a miss is `<become from="two" sprite="one">`. The eight waves are scenes, a `from=` chain each giving only its variables.
- The bomber `<reverse />`s on a random timer and `<fire>`s bombs from under him, so they trail him. A bomb on the ground resets every falling bomb.
- Open: eight scenes of one shape are a case for numbered bundles.

## Astrosmash and Lunar Lander

- Astrosmash: a big rock's rule `<spawn>`s two chips; worth is `self.worth` so one rule serves every size. A check in a rule once failed on `variable="self.worth"`; checks take `self` now.
- Lunar Lander: a landing is one rule about the pads, slower than `safespeed` and nearly upright (an `<any>` of two heading tests). The next lander is the same one reset, so fuel is copied out and back around the reset.

## Asteroids

- Rocks are one group with a group per size and shape; ship and shots share class `breaker`, so each size has one rule for being broken.
- A hit ship returns when the nearest hazard is over 150 away: `distance=` with `total="lowest"`.
- Two saucers, the small one `from=` the big with its timers replaced; they are one group piece `ufos` so rocks can break them.
- **The flame** is a `thrusting` pose over a two-frame animation. That needed looks with a `<heading>`: each look now keeps its own turned pictures. Chosen over a tethered flame object (needs its heading kept in step) and a `<become>` on the key (nothing puts it back).

## Space Invaders

- One group of 11 columns and 5 rows, rows changing the bitmaps. Invaders 2 is the same game from an SVG sprite sheet.
- Tests that used the game as a fixture load a frozen copy, `invaders_fixture.h`.
- Making the bullet thin showed `<fire>` placed its left edge at the shooter's middle; now centred.
- **The march speeds up** with one condition: a count of aliens at most `pace.next` sets the group's speed times 1.3 and halves `next`.
- **A hit holds the game** for a second: a state over it, still, popped by an `<after>` timer.
- Left: the march tempo following the speed (a sound is fixed when made), bombs only from a column's lowest alien, shields wearing down.

## Pac-Man

- The arcade's own maze, 28 by 31 tiles of 16 pixels. Walls are outlined through the middle of their edge tiles, as the arcade does.
- Each ghost's chase is a `<hunt>` variant (ahead, doubled from Blinky, shy within eight tiles). They leave the house by dot counts, with a four-second nudge.
- **Scatter and chase are looks of a `mode` object**, whose look timers run the first level's schedule; each switch turns ghosts round with `<trigger from="normal">`. Chosen over a mode verb: looks already are modes, and the schedule stays in the game.
- A ghost eaten is worth `status.worth`, doubled each time. READY! is a state over the still maze.
- Kept simple: one schedule for every level, one fruit, eyes that do not look where they go.

## Paddle and ball

- **Pong** follows Atari's arcade: square ball, dashed net, first to eleven, faster at the fourth and twelfth hit.
- **Breakout**: eight rows of fourteen, worth 7, 5, 3, 1, each colour an inner group with its worth and pitch; a `half` paddle look once the ball reaches the top.
- **Space Race**: twelve rows of debris as one 8 by 12 group; the win screen picks its words by its own conditions.
