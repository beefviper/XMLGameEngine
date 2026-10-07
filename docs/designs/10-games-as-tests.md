# 10. Games as tests of the vocabulary

**Status:** all 15 games are built in `games/`. Most are played frame by frame in `tests/test_<game>.cpp`; Pong is covered by `test_sound` and `test_deflect`, Space Invaders by `test_bitmap_sprites`, Space Invaders 2 by `test_svg_sprites`, Demon Attack by `test_ai_games`, Breakout by the input and format tests.

Games are picked to *test* the vocabulary: add a verb only when the game cannot be said without it, otherwise record that the vocabulary was enough. Each game is an approximation of the arcade original (rectangles for art, no attract mode).

## At a glance

| Game | Chosen to test | Forced (new) | Left out |
|---|---|---|---|
| Pong, Breakout, Space Invaders | the core: bounce, stick, die, fire, lockstep grids | `<deflect>` (Pong), bitmaps, animation, SVG sprites, enemy fire (Invaders 1 and 2) | |
| Frogger | hazards, loops, riding, lives, one-step moves | `dec`, `atmost`, `hop`, `wrap`, `carry`, `unless`, verbs in object rules, colors | diving turtles, crocodiles, a timer, speed-up (timers and looks would now allow the first three) |
| Space Race | a second game on the same vocabulary | nothing | |
| Kaboom | `<random>` in a `<group>`; states differing only in what they show | nothing at first; when reworked, timers, `<facing>` fire, `<reset object>` in a rule | bucket stack, speed-up inside a wave |
| Freeway | Frogger's road is a vocabulary, not a kit; two players | nothing | timed round (timers exist now, but "most crossings wins" needs a condition comparing two variables) |
| Depth Charge | limited ammunition without a countdown | nothing | several charges in flight, subs that fire back |
| Astrosmash | Kaboom's pieces plus a gun | nothing | splitting rocks (a pool and `<release>` would now do it, as in Asteroids) |
| Lunar Lander | gravity, held limited thrust, landing by speed, pixel terrain | lines, pixel, acceleration, accelerate/burn, stop, slower/faster | rotation (headings exist now), score by fuel |
| Asteroids | facing, coasting, shots from a nose, wrapping, breaking | heading, turn, thrust, drag, hidden, release, amounts | wrapping and expiring shots, saucer, waves, a safe respawn |
| Berserk, Demon Attack, Frostbite (written by another AI) | can the schema alone be enough for an author | (found gaps) then timers, facing, jump, looks, key sets | chasing, aiming |

## Frogger

- A 48-pixel grid (13 by 14): score row, home pads between hedges, five river lanes, grass, five road lanes, pavement. Everything is a rectangle drawn 6 pixels in from its cell, so positions are small sums of `cell` and `inset`. Drawn in file order (scenery first, frog last). Lanes are `<group>`s ([03](03-objects-groups-and-storage.md)).
- Needs and what was chosen:

| Need | Chosen | Rejected |
|---|---|---|
| Lives that end the game at zero | `dec` and condition `atmost` | counting deaths up to 3 (hides the number of lives in the condition); comparison operators; a lives object |
| One step per press, however long held or paused | `hop` queued by the press, made at the start of the next move | a held `move` stopped by a timer; a flag on `<input>`; a one-frame velocity (swept along the way) |
| Looping lanes | `wrap`: acts only once fully off screen and still heading that way, keeps overshoot | `reset` at the far edge (pops in whole, snaps to the start); teleport on touching an edge; duplicates at both ends |
| Riding a log | `carry` ([05](05-collisions.md)) | |
| River kills unless on a log | `unless="logs"` on the rule ([05](05-collisions.md)) | |
| Scenery colors | more named colors | `#rrggbb` literals (probably where it ends up) |
| A frog sitting in a home | a green frog under each pad; the pad `<die />`s | show/hide verbs, swapping a sprite (looks later, [07](07-pictures-and-text.md)) |

- A frog carried off the side dies at the edge by an ordinary `edge="horizontal"` rule. The river is one wide static object. One point per frog home (an amount on `inc` came with Asteroids); a filled home can be entered again.

## Kaboom

- An 800 by 600 window, a bucket stopped at the sides by `<stick />`, three buckets, three waves. Each wave is a state (`wave1`, `wave2`, `wave3`) with its own bomber and its own pool of ten bombs, faster each time and worth 1, 2, then 3 a catch; a score condition moves on at `wave2at` (15) and `wave3at` (45), and `goal` (90) wins. The arcade game has no end. The waves share one `<keys>` set.
- The Mad Bomber paces the rooftop, `<reverse />`s on a timer with a random wait, and `<fire>`s a bomb from his pool every `dropN` seconds; he has `<facing>down</facing>`, so bombs leave from under him and come in trails and clusters that follow him. A caught bomb `<die />`s back into the pool. A bomb on the ground does `<reset object="bombsN" />`: every falling bomb at once, so one miss costs one bucket. The tests play wave one with an automatic player and check the last wave is harder.
- History: first a looser game (Gem Catcher), then the arcade rules with bombs in six fixed columns, `<random>` speeds drawn once, and a `tally` object a condition turned into the explosion (a rule could not reset other objects then). Reworked with timers when those came.
- Showed: a group's shared part can hold `<random>` and each member still draws its own numbers; keys held across a state change keep moving the bucket.
- Rejected: a new speed on every reset (`<random>` re-drawn at runtime), a stack of buckets that loses its top (the count is a number), speed rising within a wave (three fixed speeds stand in).

## Freeway

- 800 by 600, 50-pixel cells, twelve rows: far side, ten lanes (cars and trucks alternating, each lane its own speed and direction), near side. Two players share the road, each chicken with its own score, two actions and two key sets (W/S and Up/Down), no rule about the other. First to `crossings` (global variable) wins. Lane spacing follows Frogger's rule (window width plus object size).
- The far side is an object (`class="farside"`) the chicken has a rule for; a hop into that row is judged where it lands, so it scores at once. A car hits a chicken that hops into it because a hopped object counts as moving; two stationary objects are never checked.
- Rejected: a timed round (there was no clock; timers exist now, but picking the winner by most crossings needs a condition that compares two variables), knocked back one lane instead of to the start (closer to the arcade, one `<move>` line; sending to the start makes a hit far too costly near the far side and costs nothing near the start), lives (kept different from Frogger).

## Depth Charge

- Ship stopped by `<stick />`, one charge falling at a time, nine submarines in three lanes looping with `<wrap />`, a charge hit sinks both (`<die />`), win when `<remaining>0</remaining>` on `class="subs"`.
- **Limited ammunition without a countdown on every shot:** count the other way. `ship.charges` starts at the number of misses allowed and the floor rule does `<dec>` and `<die />`; a hit does not touch it, so a good player never runs out. Eight wasted charges end the game.
- `<fire>` copies the projectile's own `<velocity>` (positive y falls) and starts inside the ship, leaving in a few frames; no ship rule about the charge, so it is not a collision.
- Rejected: counting every shot (would need `<dec>` in the ship's fire action; object actions cannot run `<inc>`/`<dec>`), several charges in flight (a different, easier game), submarines worth different amounts (an amount on `inc` exists now).

## Astrosmash

- Eight rocks (four slow, four fast) fall fixed columns with `<random>` speeds and heights; a ship fires one shot at a time. A shot kills (`<die />` on the shot, `<inc>` then `<reset />` on the rock: `<die />` is for things that stay dead). A rock reaching the ground or the ship costs a life. 20 points win, five lives lost is game over. Both groups share class `rocks`.
- A hidden, collision-less shot does not interfere with rocks passing where it sits.
- Rejected: splitting rocks (there was no way then; pools and `<release>` now do it in Asteroids), drifting (needs wall rules, harder columns), a free miss (unloseable by neglect), differently scored rocks (an amount on `inc` exists now).

## Lunar Lander

- An 800 by 600 window; a moon of 15 `<line>`s with a gap for a short thick green pad; a lander of 13; gravity from its `<acceleration>`; three thrusters burn `fuel`; touching the ground or pad at or above `safespeed` wrecks it, slower sets it down; both end the game through `lander.crashed` / `lander.landed`. Decisions are in [05](05-collisions.md), [06](06-motion-and-verbs.md), [07](07-pictures-and-text.md). Only the lines are solid, not the space under them (the bottom edge ends the game). Not done: rotation (the original spins), zoom, score by fuel left, several pads or terrains.

## Asteroids

- A four-line triangle ship (turn, thrust, fire), four big rocks that break into two mediums, then two smalls, then nothing (2, 5, 10 points), three ships, wrapping on all four edges, win when none remain. Rocks are pools released where the shot hit; shots are a pool fired along the heading. Decisions in [06](06-motion-and-verbs.md), [08](08-timers-and-enemy-behavior.md). The ship returns at once at the centre.

## Space Invaders 1 and 2

- Invaders 1 was a grid of identical rectangles; redrawn with three alien kinds (squid, two rows of crab, two of octopus) as a group of three grids of two named bitmaps animated on a one-second interval, a bitmap cannon and a thin bullet. Making the bullet thin showed `<fire>` put its left edge, not its middle, at the middle of the shooter; now centered (this moved Depth Charge's and Astrosmash's shots by half their width). Invaders 2 is the same game drawn from an SVG sprite sheet with three-frame animations a half second apart ([07](07-pictures-and-text.md)). In both, and in Demon Attack, the aliens fire back from pools on random timers, and the cannon has three lives.
- Moving the game to three kinds broke tests that borrowed it as a grid fixture, so those load `tests/invaders_fixture.h`, a frozen copy of the first game.

## Games written by another AI (Berserk, Demon Attack, Frostbite)

The author asked another AI to write three games from `xgedef.xsd` alone. All three passed the schema and loaded, none could be played (a blank screen first). Every error was a command the schema accepts where the engine does nothing with it:

- **Colors** like `white` or `#00ff00` (a color is a `color.` name; anything else is transparent). **Key names** the engine lacks (`button="fire"` reads as `Unknown` and never runs).
- **`<move>` / `<fire>` in an `<input>`** (they only mean something in an object `<action>`), **`<inc>`/`<dec>` in a `<condition>`** and **`<reset object>` in a `<collision>`** (fixed later by [08](08-timers-and-enemy-behavior.md)).
- **A variable with no owner** (`<inc variable="score">` does nothing; score and lives live on the player), **a spare projectile that starts enabled** (never fired; it flew off with nothing to put it away).
- Lesson: the vocabulary was enough (no tag was added), but the schema cannot tell an author a command is in the wrong place. Making the loader refuse unknown buttons and misplaced commands, and warn about unowned variables, would turn each mistake into a load error naming the line. **Done (2026-10-07)** at the author's request, as errors rather than warnings: unknown keys and colors, a command where it does nothing, and an `<inc>`/`<dec>` of a variable no object has each stop the load with what to write instead, and a misspelt name gets a "did you mean". Every shipped game still loads. Where a command works is a table in `game_expr.cpp` beside the dispatchers in `command_executor.cpp`; they must agree.
- `tests/test_ai_games.cpp` plays Demon Attack from title to end screen; unknown colors are checked by a test that reads every `<color>` and `<background>`.

### Frostbite

- Rewritten from scratch: a snowy shore and igloo along the top, four rows of drifting floes in alternating directions, Bailey jumping between rows with `<jump>`. Landing on a white row turns it blue (`<become>`) and adds an igloo block (`<reveal>`); four blue rows turn white again (a counting condition); water drowns unless on a floe; cold takes a degree a second (a state timer); geese push him along (`<carry />`); a fish is a bonus on a timer; the finished igloo opens, and walking in wins.

### Berserk (rewritten after Stern's Berzerk, 1980)

- Arcade facts: walls kill the man and robots; robots shoot once the score is high enough, only when lined up, and chase; he shoots in eight directions, one shot at a time; each room has exits in all four walls; next room is a different maze, entered from the opposite side; robot 50 points, 10 more each for clearing the room; Evil Otto (bouncing face, cannot be shot, passes walls) comes if you linger, slow while robots remain, as fast as the man after. (From web pages and a manual; the details not on the pages are least sure.)
- Built: four rooms as `<state>`s, each a maze of 73 by 49 characters as a `<bitmap>` at scale 8 (584 by 392; walls are `pixel`) with four exits as gaps, its own robots (3, 4, 5, 6), pushed in order, the fourth leading back to the first. The mazes came from a throwaway generator (random walls kept only if every cell stays reachable; seeds 11, 23, 37, 41); to change a room, edit its `<row>`s by hand, keep the first three cells of the left exit's row open and every robot in a cell with room to walk.
- The man has four looks (`<become>` in his actions) and `<facing>`, a box against pixel walls, one shot in flight (one `bullet` object). Robots patrol corridors and `<reverse />` at walls and exits; each fires on a timer from a shared pool of four, along its group's facing (a group is the robots of one axis facing one of its two ways); the wait is a `<random>` whose ends use `player.depth`, so the second lap fires faster. Otto is hidden until a room timer reveals him (14 seconds less depth, minimum 6, or at once when the room's robots are gone); one speed.
- Every rule that costs a life is the man's own (walls, shots, Otto, robots): a rule's `<reset />` puts only that object back, while `<reset object="player" />` in someone else's rule would reset his score and lives too (**`<reset object>` resets every variable**, not just position; this cost a morning). Exits add to `player.exited`; a room condition resets what it used, adds to `player.depth` and pushes the next room. A per-room `tally` object pays the room bonus when it reaches the robot count and is reset when the man leaves. An extra man every 2000 points from `player.lifepoints`. Pause is P or Escape.
- Differences: no chasing or aiming (the biggest), four directions not eight, the man always enters at the left exit, walls do not kill robots (a `class="wall"` die rule would kill every patrol at once), Otto has one speed. Speeds, sizes (a 584 by 440 window) and waits were chosen without watching the game played: **tune them first.** The author found the first Berserk "absolutely horrible". Open: chase/aim verb, eight directions, entering opposite the exit (a reset to one of several named places), Otto speeding up, robots dying on walls and each other, robots not firing until the score is high enough (waits that shorten with score need only an expression).
