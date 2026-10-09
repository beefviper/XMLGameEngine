# Games as tests: the first games

**Status:** Every game in `games/` is built; most are played frame by frame in `tests/test_<game>.cpp`.

## At a glance

| Game | Chosen to test | Forced (new) | Left out |
|---|---|---|---|
| Pong, Breakout, Space Invaders | the core: bounce, stick, die, fire, lockstep blocks | `<deflect>` (Pong), bitmaps, animation, SVG sprites, enemy fire (Invaders 1 and 2) | |
| Frogger | hazards, loops, riding, lives, one-step moves | `dec`, `atmost`, `hop`, `wrap`, `ride`, `unless`, verbs in object rules, colors | diving turtles, crocodiles, a timer, speed-up (timers and looks would now allow the first three) |
| Space Race | a second game on the same vocabulary | nothing | |
| Kaboom | `<random>` in a `<group>`; states differing only in what they show | nothing at first; when reworked, timers, `<facing>` fire, `<reset object>` in a rule | bucket stack, speed-up inside a wave |
| Freeway | Frogger's road is a vocabulary, not a kit; two players | nothing | timed round (timers exist now, but "most crossings wins" needs a condition comparing two variables) |
| Depth Charge | limited ammunition without a countdown | nothing | several charges in flight, subs that fire back |
| Astrosmash | Kaboom's pieces plus a gun | nothing | splitting rocks (a pool and `<release>` would now do it, as in Asteroids) |
| Lunar Lander | gravity, held limited thrust, landing by speed, pixel terrain | lines, pixel, acceleration, accelerate/burn, stop, slower/faster | rotation (headings exist now), score by fuel |
| Asteroids | facing, coasting, shots from a nose, wrapping, breaking | heading, turn, thrust, drag, hidden, release, amounts | wrapping and expiring shots, saucer, waves, a safe respawn |
| Berserk, Demon Attack, Frostbite (written by another AI) | can the schema alone be enough for an author | (found gaps) then timers, facing, jump, looks, key sets; later chasing and aiming | |
| Pitfall!, Missile Command, Combat, Air-Sea Battle, Megamania | five Atari 2600 games picked because today's verbs already say them | nothing | see [below](games-atari.md#five-atari-2600-games) |


## Frogger

- A 48-pixel grid (13 by 14): score row, home pads between hedges, five river lanes, grass, five road lanes, pavement. Everything is a rectangle drawn 6 pixels in from its cell, so positions are small sums of `cell` and `inset`. Drawn in file order (scenery first, frog last). Lanes are `<group>`s ([objects](objects.md)).
- Needs and what was chosen:

| Need | Chosen | Rejected |
|---|---|---|
| Lives that end the game at zero | `dec` and condition `atmost` | counting deaths up to 3 (hides the number of lives in the condition); comparison operators; a lives object |
| One step per press, however long held or paused | `hop` queued by the press, made at the start of the next move | a held `move` stopped by a timer; a flag on `<input>`; a one-frame velocity (swept along the way) |
| Looping lanes | `wrap`: acts only once fully off screen and still heading that way, keeps overshoot | `reset` at the far edge (pops in whole, snaps to the start); teleport on touching an edge; duplicates at both ends |
| Riding a log | `ride` ([collisions](collisions.md)) | |
| River kills unless on a log | `unless="logs"` on the rule ([collisions](collisions.md)) | |
| Scenery colors | more named colors | `#rrggbb` literals (probably where it ends up) |
| A frog sitting in a home | a green frog under each pad; the pad `<die />`s | show/hide verbs, swapping a sprite (looks later, [pictures](pictures.md)) |

- A frog ridden off the side dies at the edge by an ordinary `edge="horizontal"` rule. The river is one wide static object. One point per frog home (an amount on `inc` came with Asteroids); a filled home can be entered again.


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

- An 800 by 600 window; a moon of 15 `<line>`s with a gap for a short thick green pad; a lander of 13; gravity from its `<acceleration>`; three thrusters burn `fuel`; touching the ground or pad at or above `safespeed` wrecks it, slower sets it down; both end the game through `lander.crashed` / `lander.landed`. Decisions are in [collisions](collisions.md), [motion](motion.md), [pictures](pictures.md). Only the lines are solid, not the space under them (the bottom edge ends the game). Not done: rotation (the original spins), zoom, score by fuel left, several pads or terrains.


## Asteroids

- A four-line triangle ship (turn, thrust, fire), four big rocks that break into two mediums, then two smalls, then nothing (2, 5, 10 points), three ships, wrapping on all four edges, win when none remain. Rocks are pools released where the shot hit; shots are a pool fired along the heading. Decisions in [motion](motion.md), [timers](timers.md). The ship returns at once at the centre.


## Space Invaders 1 and 2

- Invaders 1 was a grid of identical rectangles; redrawn with three alien kinds (squid, two rows of crab, two of octopus) as a group of three grids of two named bitmaps animated on a one-second interval (since one group of 11 columns and 5 rows whose rows change the bitmaps, [objects](objects.md)), a bitmap cannon and a thin bullet. Making the bullet thin showed `<fire>` put its left edge, not its middle, at the middle of the shooter; now centered (this moved Depth Charge's and Astrosmash's shots by half their width). Invaders 2 is the same game drawn from an SVG sprite sheet with three-frame animations a half second apart ([pictures](pictures.md)). In both, and in Demon Attack, the aliens fire back from pools on random timers, and the cannon has three lives.
- Moving the game to three kinds broke tests that borrowed it as a block fixture, so those load `tests/invaders_fixture.h`, a frozen copy of the first game.
