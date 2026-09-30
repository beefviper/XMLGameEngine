# 32. Depth Charge

**Status:** built (`games/depthcharge.xml`, `tests/test_depthcharge.cpp`); no new verbs

## Why this game

Depth Charge (an arcade game of 1977) is the earliest game of the ones tried so far, and it looks as if it needs an ammunition count and a dropper, neither of which exists. It is Space Invaders' gun turned around, and this file is the test of whether "you have limited shots" can be said with what is there.

## The game as described

An 800 by 600 window. A ship crosses the surface (A and D, or the arrows, stopped at the sides by `<stick />`) and drops a depth charge with space. Nine submarines cruise in three lanes below it, each lane its own speed and direction, looping with `<wrap />`. Only one charge can be falling at a time. A charge that hits a submarine sinks it (`<die />` on both) and adds one to the ship's `sunk`; a charge that falls to the sea floor is wasted. Eight wasted charges end the game (`gameover`); sinking all nine submarines wins (`youwin`). Enter starts and restarts, P or Escape pauses.

## What it tests

| Question | How the file answers it |
|---|---|
| Can "limited ammunition" be said without a counter that goes down on every shot? | Yes, by counting the other way: the ship's `charges` starts at the number of misses allowed and the floor rule on the charge does `<dec variable="ship.charges" />` and `<die />`. A hit does not touch it, so a good player never runs out. |
| Can a projectile be fired downward? | Yes. `<fire />` copies the projectile's own `<velocity>` and puts it at the shooter's top-centre, so a charge with a positive `y` velocity falls. It starts inside the ship's rectangle and leaves it within a few frames; nothing in the file gives the ship a rule about the charge, so it is not a collision. |
| Can a win condition wait for all of a class? | Yes: `<remaining>0</remaining>` on `class="subs"`, the same test that ends Space Invaders and Frogger's pads. A sunk submarine is hidden, and the count only looks at what is still in play. |
| Can the end screens show numbers from a running game? | Yes: the `sunk` text is shown in `youwin` and `gameover` as well, as in Kaboom. |

## Options considered

- **A limited number of charges in all, hits included.** The natural reading of "ammunition", and what a dropper would use. It needs a `<dec />` in the ship's fire action, and commands in an object action are limited to `move`, `hop` and `fire` today. Counting misses does the same job for a player who is not missing.
- **Several charges in flight.** `<fire />` allows one projectile per object and every charge would need its own object and key; the game would be a different, easier one. One at a time is also how the arcade game paced itself.
- **A timer for the round.** No clock in the language; see [31](31-freeway.md).
- **Submarines at different depths worth different amounts.** Needs an amount on `inc` ([21](21-frogger.md)).

## Approximations

Three lanes at fixed depths (the arcade game had a random depth per pass); one kind of submarine; no mines, no explosions, no sound; a rectangle for the charge.
