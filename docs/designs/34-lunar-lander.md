# 34. Lunar Lander

**Status:** built (`games/lunarlander.xml`, `tests/test_lunarlander.cpp`, `tests/test_lines_and_pixels.cpp`); new verbs and tags: `<line>`, `<type>pixel</type>`, `<acceleration>`, `<accelerate>`, `<stop />`, `<slower>`, `<faster>`

## Why this game

Lunar Lander (Atari, 1979) is drawn entirely from lines, and it asks three things the language could not yet say: a constant pull on an object (gravity), a thrust that is held and runs out, and a landing that is good or bad depending on how fast you touch. It also needs the ground to be touched only where it is really drawn, because a jagged moon inside a bounding box is mostly empty. That last need is the same one a fighting game has, so it was made general.

## The game as described

An 800 by 600 window. The moon's surface (two runs of peaks, slopes and a ledge) is one object drawn from fifteen `<line>`s, with a gap left for the pad, which is a short thick green line. The lander is thirteen lines. It falls under its own `<acceleration>`; the keys fire three thrusters (up, left, right), each of which adds to the velocity while held and burns one unit of `fuel` a frame, and does nothing once the fuel is gone. Touching the ground, the bottom edge, or the pad faster than `safespeed` wrecks it; touching the pad slower sets it down. Both end the game, through `lander.crashed` and `lander.landed`.

## What it tests

| Question | How the file answers it |
|---|---|
| Can a picture be described as data? | Yes: `<line>` tags inside a `<sprite>` are drawn once, when the game loads, into a bitmap that is transparent where nothing is drawn. |
| Does what is tested match what is drawn? | Yes: `<type>pixel</type>` on a collision looks at the same bitmap the backend shows. |
| Can gravity be data rather than a rule? | Yes: an object's `<acceleration>` is added to its velocity every frame. |
| Can a key be held for thrust and cost something? | Yes: `<accelerate direction= burn=>` adds while held and uses up a variable. |
| Can the same pair of objects have a good and a bad result? | Yes: `<slower>` and `<faster>` filter a rule by the speed of the object, so the pad has a rule for each. |

## Decisions

- **`line` is a sprite shape, not a separate kind of object.** A sprite may hold several `<line>`s and nothing else; they are rasterized into a `Bitmap` (Bresenham, a square brush of `<thickness>` pixels, transparent elsewhere), so every backend uploads the same pixels and a window is not needed to know the size. The size is carried in the sprite's parameters, so expressions such as `lander.width` work in tests.
- **Collision `type` is optional and defaults to the old behaviour.** `<type>box</type>` (the default) sweeps the bounding box or circle. `<type>pixel</type>` runs that same sweep first, and only when it reports a hit does it walk the path in half-pixel steps, then bisect, to the first step where a drawn pixel of one object lies on a solid pixel of the other. An object of another shape counts as solid over its whole shape (a circle analytically), so a pixel lander can meet a plain rectangle. Pixel is refused on text and images, which have no bitmap to test.
- **The edge reported comes from the relative motion**, as for a box, so `stick`, `bounce` and the rest keep working unchanged.
- **Lines are at least two pixels thick in the shipped game.** A one-pixel diagonal can cross another without sharing a pixel, which would let the lander through a slope.
- **Gravity is a property of the object, not a world setting.** A second object with no `<acceleration>` is not pulled, and `reset` restores the starting value.
- **Thrust burns a variable by name**, like the other verbs that name a variable; at zero (or less) the thrust is blocked.
- **`stop` zeroes velocity, acceleration and held thrust** so a landed lander stays landed; `reset` brings the pull back.
- **`slower` is speed below N, `faster` is speed at or above N**, taken once for the whole reaction, so the two rules for the pad can never both run or both be skipped. Neither is allowed on a screen-edge rule.

## Options considered

- **Only a box with a finer shape (a list of boxes).** Simpler, but it is a second description of the same picture that would drift from the lines.
- **Pixel testing for everything.** Costly for no gain in Pong; kept opt-in, and always behind the box sweep, so a pair far apart costs nothing extra.
- **A `gravity` world variable.** Would pull everything; most games want some things to float.
- **Thrust as a rule on the key.** It would need a way to say "while held" and "until empty" in a rule; a verb that names them is shorter and was what the input system already did for `move`.
- **Rotation.** The original spins its lander; this version has a side thruster instead, because the language has no rotation yet ([27](27-object-composition-and-shape.md)).

## Approximations

No rotation, no zoom, no score by landing place or fuel left, no sound, one pad, one shape of terrain. Only the lines of the terrain are solid, not the space under them, so a lander that somehow got below a line would fall freely (the bottom edge still ends the game). The window backends (SFML, raylib, SDL2) were written against their headers and compile-checked; the game was played frame by frame in the tests, not watched in a window.
