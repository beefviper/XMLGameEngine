# 30. Kaboom

**Status:** built (`games/kaboom.xml`, `tests/test_kaboom.cpp`); no new verbs

## Why this game

The earlier games were chosen to test the vocabulary: Frogger forced `hop`, `carry`, `wrap`, `dec` and `unless`; Space Race forced nothing and confirmed that the vocabulary was already enough for a second game. Kaboom! (Activision, 1981) was picked the other way round: a small game that is fun for a minute at a time, and that leans on parts of the format the shipped games barely touch: `<random>` ([03](03-expression-syntax.md)) inside a `<group>` ([29](29-groups.md)), and several states that differ only in what they show.

It first shipped as a looser game called Gem Catcher (gems to catch, bombs to dodge, a miss that cost nothing). It was renamed and rewritten to follow the arcade rules once it was clear how close it was.

## The game as described

An 800 by 600 window. A bucket slides along the bottom under held keys (A and D, or the arrows) and is stopped at the sides by `<stick />`. Bombs fall down six fixed columns and every bomb is to be caught. A caught bomb scores and starts its fall again (`<inc />`, `<reset />`). A bomb that reaches the floor does three things: `<dec />` on the bucket count, `<inc />` on a separate `tally.missed`, and `<reset />`. A state condition on the tally then resets the whole wave's group of bombs (the explosion) and the tally itself, so a miss costs one bucket however many bombs were in the air, as in the arcade game. Three buckets; with none left, `gameover`.

The bombs come in **waves**, each a `<state>` that shows its own group: `playing` (wave 1), `wave2`, `wave3`. The groups differ only in color, fall speed range and what a catch is worth: one `<inc />` in the first, two in the second, three in the third. A condition on the score pushes the next wave (10 points, then 30), and the third wave ends in `youwin` at `goal` points. All three thresholds are ordinary global `<variable>`s. The arcade game has no end; this one has a goal so that a run can finish.

## What it tests

| Question | How the file answers it |
|---|---|
| Can a group's shared description hold a `<random>`? | Yes. Each group gives the fall speed and the starting height as `<random>` ranges once, and each `<member>` says only its column. A member is read as an object of its own, so every member draws its own numbers. |
| Can one event (a miss) act on many objects? | Not from a rule, which only acts on its own object. The rule records the event in a variable on a helper object, `tally`, and a state condition watching that variable does the `<reset object="bombsN" />` and clears the tally with `<reset object="tally" />`. A condition's commands can reach any object; a collision rule's cannot. |
| Can a score be worth more as the game goes on? | Yes, by repeating `<inc />`: commands in a rule run in order, so two tags are two points. No amount on `inc` is needed, though one would be shorter ([21](21-frogger.md)). |
| Does a state that differs only by what it shows work? | Yes. The three wave states repeat the same key bindings and differ in which bombs they show and which group their explosion resets. A key held across the change keeps moving the bucket, because continuous bindings are resumed ([10](10-input-and-actions.md)); the tests check it. |
| Does a piece keep its own draw when it is reset? | Yes, and that is the limit: a `<random>` is worked out once at load, so a bomb falls at the same speed from the same height every time. Different bombs have different speeds, so the pattern only repeats after a long while, and it is different on every launch. |

## What it cannot do

**The Mad Bomber does not drop the bombs.** In the arcade game a bomber runs along the top and each bomb starts wherever he is. Nothing in the language creates an object, or starts one, where another object is: `<reset />` returns a bomb to its own starting place, and `<fire />` launches only on a key press. So here the bombs fall from fixed columns, and the bomber is scenery that paces along the top and turns at the sides with `<bounce />`. A spawn verb, or a projectile that starts from another object's position when a condition fires, would close the gap and is left as an idea.

## Options considered

- **A piece per fall, created as the game runs.** See above; this is the same limit and would also give an endless game. The chosen shape is a fixed set of objects that fall, are reset and fall again.
- **Draw a new speed on every reset.** It would make every fall different. It needs `<random>` to be worked out again at run time rather than at load, a change to when values are evaluated ([05](05-variables-and-evaluation-order.md)). Not done; the readme lists it as a known limit.
- **A stack of three buckets that loses its top one.** Would need commands that hide another object. The count is shown as a number instead.
- **Speed that rises inside a wave.** Needs acceleration, or a velocity that a condition can change. Three fixed speeds stand in for it.

## Approximations

No endless play, no bucket stack, no sound, no high score, and the bomber is decoration. The bucket is one rectangle and the bombs are circles rather than art.
