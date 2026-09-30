# 33. Astrosmash

**Status:** built (`games/astrosmash.xml`, `tests/test_astrosmash.cpp`); no new verbs

## Why this game

Astrosmash (Intellivision, 1981) is what the two most recent games would make if crossed: Kaboom's falling pieces, with their `<random>` speeds and heights, and Space Invaders' gun. It was picked to see whether a fixed set of pieces that fall, are shot, and fall again is enough for a game whose whole point is shooting them.

## The game as described

An 800 by 600 window. Eight rocks fall down fixed columns: four slow boulders and four fast pebbles, each with its own speed and its own starting height above the screen, drawn once when the game loads. A ship slides along the bottom (A and D, or the arrows) and fires one shot at a time straight up with space. A rock that is shot scores a point and goes back to the top of its column to fall again; a rock that reaches the ground, or lands on the ship, costs a life and goes back too. Twenty points win, five lives are lost and it is game over. Enter starts and restarts.

The two groups share the class `rocks`, so the shot has one rule for every kind of rock and each rock has the same three rules (ground, ship, shot).

## What it tests

| Question | How the file answers it |
|---|---|
| Can a rock be "destroyed" and come back? | Yes. The shot's rule is `<die />` and the rock's rule for the shot is `<inc variable="ship.score" />` then `<reset />`. `<die />` is for things that stay dead (Space Invaders' aliens); `<reset />` puts a rock back at the start of its fall. |
| Do the two rule directions for one pair both run? | Yes: the shot has a rule about `rocks` and each rock has one about `shot`, and both take effect in the frame the two touch. |
| Does an object that is not in flight interfere? | No: until it is fired, the shot is hidden and has its collisions off, so a rock passing where it sits does nothing. |

## Options considered

- **Rocks that split when hit, or that drift sideways.** Splitting needs an object created while the game runs, which the language cannot do. Drifting is just an `x` velocity and would work, but then a rock that leaves a side wall needs a rule, and it made the columns harder to reason about. Left out.
- **Differently scored rocks.** Needs an amount on `inc` ([21](21-frogger.md)).
- **A rock that reaches the ground costs nothing.** It makes the game unloseable by neglect; here a miss is what makes shooting matter.
- **Draw a new fall on every reset.** As for Kaboom: `<random>` is worked out once at load, so a rock repeats its own fall. The rhythm is different every launch but not within one.

## Approximations

Only vertical fall; no UFO, no bonus items, no ground, no sound; rectangles for everything; one shot in flight at a time.
