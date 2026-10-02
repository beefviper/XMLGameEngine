# 41. Asteroids

**Status:** built (`games/asteroids.xml`, `tests/test_asteroids.cpp`); new verbs and tags: `<heading>`, `<drag>`, `<hidden>`, `<turn>`, `<thrust>`, `<release>`; `<fire>` now takes a group and follows the heading; `<inc>`/`<dec>` take an amount

## Why this game

Asteroids (Atari, 1979) needs what Lunar Lander ([34](34-lunar-lander.md)) left out: a ship that turns and is pushed along the way it faces, shots that leave its nose, a ship that coasts and slowly loses speed, a screen with no edges, and rocks that break into smaller rocks. Pixel collisions and `<wrap />` already existed; everything else about facing and breaking is new.

## The game as described

An 800 by 600 window. The ship is a triangle of four lines. Left and right turn it, up thrusts along the way it faces, space fires. Four big rocks drift in; a shot breaks a big rock into two medium ones, a medium into two small ones, a small one into nothing. A big rock is worth 2, a medium one 5 and a small one 10, each one `<inc variable="ship.score">N</inc>`. Three ships; a rock touching the ship costs one and the ship starts again in the middle. All rocks gone is a win.

## What it tests

- An object with a `<heading>` (degrees clockwise from straight up) and a picture that follows it.
- A held `<turn>` and a held `<thrust>` in an object's `<action>`.
- `<drag>`: a fraction of the speed lost each frame, so thrust has a top speed.
- `<fire>` aimed along the heading, from a pool of shots, several in flight.
- Hidden pools of objects and a `<release>` verb that brings some of them into play where another object was.
- Pixel collisions that follow the rotated picture, and wrapping on all four edges.

## Decisions

- **Heading is degrees clockwise from up**, so 0 is up and 90 is right; the direction is (sin h, -cos h). An object with no `<heading>` is unchanged.
- **Pre-rotated pictures.** A sprite of lines on an object with a heading is drawn at 72 headings (every 5 degrees) when the game loads (`rasterizeTurned`), all the same square size and turned about the middle of the lines. The object shows the nearest one, so every backend, which already redraws from the object's bitmap, needed no change, and pixel collisions use the picture that is shown.
- **`<turn>` and `<thrust>` are held like `<move>` and `<accelerate>`**; `<thrust>` can `burn` a variable. Using either on an object with no `<heading>` is an error when the game loads.
- **`<drag>` is a fraction from 0 up to (not including) 1**, applied to the velocity once a frame after thrust.
- **Pools, not spawning.** A `<group>` of rocks is built at load and hidden (`<hidden>true</hidden>`). `<release object="group">N</release>` in a collision rule puts the first N out-of-play members back, centered on the object running the rule, at their own starting velocity. The same idea gives shots: `<fire object="shots" />` picks the first member whose collisions are off.
- **`<inc>` and `<dec>` take an amount** (`<inc variable="ship.score">5</inc>`; a bare one is still 1). Scoring first repeated `<inc />`, which showed the need.
- **A hidden object does not count** for a condition's `remaining`, so "no rocks left" is a plain condition.
- **A collision `<reset />` restores the heading** as well as the position; `<stop />` also clears turn and thrust.

## Options considered

- **Rotating at draw time in each backend.** Four backends to change, and the pixel test would still need the turned picture; pre-rotating once is shared by all.
- **A real runtime spawn verb.** Needs objects created while running; a pool needs nothing new in the object list and the tests can see every member.
- **Wrapping shots and a shot lifetime.** Not done; shots die at the screen edge.

## Approximations

The ship comes back at the middle straight away, with no wait for a safe spot. A pool member has the same velocity every time it is released. One wave only, no flying saucer, no sound. Headings are rounded to 5 degrees.
