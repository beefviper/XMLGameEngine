# 30. Gem Catcher

**Status:** built (`games/gemcatcher.xml`, `tests/test_gemcatcher.cpp`); no new verbs

## Why this game

The earlier games were chosen to test the vocabulary: Frogger forced `hop`, `carry`, `wrap`, `dec` and `unless`; Space Race forced nothing and confirmed that the vocabulary was already enough for a second game. Gem Catcher was picked the other way round: a small game that is fun for a minute at a time, and that leans on a part of the format the shipped games barely touch, `<random>` ([03](03-expression-syntax.md)), combined with `<group>` ([29](29-groups.md)).

## The game as described

An 800 by 600 window. A basket slides along the bottom under held keys (A and D, or the arrow keys) and is stopped at the sides by `<stick />`. Six gems and three bombs fall down fixed columns, the bombs in the gaps between the gem columns. Touching the basket does one of three things to a piece: a gem adds one to the basket's `score`, a bomb takes one off its `lives`, and either way the piece is put back with `<reset />`. A piece that reaches the floor uncaught is also put back, and nothing is lost. A state condition ends the game at a score of 15 (`youwin`) or at no lives (`gameover`); space on either screen is a bare `<reset />`, which brings everything back to the menu.

The goal is an ordinary global `<variable>`, and the condition's `<atleast>` reads it, so changing how long a game lasts is one number.

## What it tests

| Question | How the file answers it |
|---|---|
| Can a group's shared description hold a `<random>`? | Yes. The gems' group gives the fall speed and the starting height as `<random>` ranges once, and each `<member>` says only its column. A member is read as an object of its own, so every member draws its own numbers. |
| Does a piece keep its own draw when it is reset? | Yes, and that is the limit: a `<random>` is worked out once at load, so a piece falls at the same speed from the same place every time. Different pieces have different speeds, so the pattern only repeats after a long while, and it is different on every launch. |
| Can one rule serve the whole set? | One `<collision object="basket">` in the group is all the gems need, and one in the bombs' group is all the bombs need. The basket has no rule about them at all. |
| Do the end screens need anything new? | No. They are two states like Space Race's win screens; the score is shown on both because the same object can be shown in several states. |

## Options considered

- **A piece per fall, created as the game runs (a bomber dropping things from where it is).** This is how the arcade game this is modelled on works, and it is not expressible: nothing in the language creates an object while the game runs, and `<fire>` only launches on a key press. The chosen shape is a fixed set of objects that fall, are reset and fall again. A dropper object that releases pieces on its own, or a spawn verb, would be a new verb and is left as an idea.
- **Draw a new speed on every reset.** It would make every fall different. It needs `<random>` to be worked out again at run time rather than at load, a change to when values are evaluated ([05](05-variables-and-evaluation-order.md)). Not done; the readme lists it as a known limit.
- **Missed gems cost a life.** The more usual rule, and it would use the same `<dec>` on the floor edge. It was left out because it makes the first minute frustrating, and catching only what you can reach is more fun. Changing it is one added `<dec variable="basket.lives" />` in the gems' floor rule.
- **Gems worth different amounts.** Needs an amount on `inc` ([21](21-frogger.md), ideas that came out of it).

## Approximations

No difficulty ramp, no bonus items, no sound, no high score, and no bomber. The basket is one rectangle. Pieces are a rectangle (gem) and a circle (bomb) rather than art.
