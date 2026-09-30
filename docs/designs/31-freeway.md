# 31. Freeway

**Status:** built (`games/freeway.xml`, `tests/test_freeway.cpp`); no new verbs

## Why this game

Frogger needed five new verbs; Space Race and Kaboom needed none. Freeway (Activision, 1981) was picked as the cheapest possible check that the vocabulary Frogger left behind is a vocabulary and not a Frogger kit: it is Frogger's road without the river, played by two people at once. It also puts two independent players in one file, which only Pong and Space Race had done.

## The game as described

An 800 by 600 window on a grid of 50 pixel cells, twelve rows down. Row 0 is the far side, rows 1 to 10 are ten lanes of traffic (cars and trucks alternating, every lane with its own speed and neighbouring lanes going opposite ways), row 11 is the near side where both chickens start. Each chicken hops one lane for each key press (W and S for player 1, Up and Down for player 2). A chicken that reaches the far side scores a point and goes back to the start; a chicken that is hit goes back to the start and keeps what it has. The first to five crossings wins; the target is an ordinary global `<variable>`.

Each lane is a `<group>` ([29](29-groups.md)): the lane's shape, row, speed and `<wrap />` are written once and each member says only where it starts. Members are spaced by the same rule Frogger uses (a lane of things `size` wide repeats every window width plus `size`), and the starting positions in the file were worked out with that rule.

## What it tests

| Question | How the file answers it |
|---|---|
| Is the Frogger vocabulary enough for a second road game? | Yes. `hop`, `wrap`, `reset`, `inc` and an `atleast` condition are all it uses. |
| Can two players share one field of moving objects? | Yes. Each chicken is an object with its own `score`, its own two actions and its own two key bindings, and neither has any rule about the other, so they pass through each other. |
| How does a crossing score when a hop that would leave the window is refused? | The far side is an object (class `farside`) with collisions on, and the chicken has a rule for it. A hop into that row is judged where it lands ([readme](../readme.md), Collisions), so it scores at once; nothing has to reach the top edge. |
| Does a car waiting in a lane hit a chicken that hops into it? | Yes: an object that has just hopped counts as moving, so the pair is looked at even though the car is not. The reverse does not hold: a car and a chicken that are both still are never checked (a known limit, and the reason the tests keep a car moving when they drop it on a chicken). |

## Options considered

- **A timed round, as in the arcade game.** A couple of minutes, most crossings wins. There is no clock in the language (no time step at all: movement is per frame), so this has no spelling. First to `crossings` is the substitute, and it also gives a winner that is clear from the score alone.
- **Knock the chicken back a lane, not to the start.** This is closer to the arcade game and is possible today: a `<move direction="down">` in the traffic rule instead of `<reset />`. It was left out because being sent to the start makes the price of a hit very high near the far side and almost nothing near the start, which is unfair to whoever is winning; the change is one line.
- **Lives.** Frogger has them; Freeway does not, and adding them would need a second end condition. Left out on purpose so the two games differ.

## Approximations

Simple rectangles for the chickens and vehicles; no sound; no diagonal or sideways hops (Freeway itself has none); no different-looking lane surfaces.
