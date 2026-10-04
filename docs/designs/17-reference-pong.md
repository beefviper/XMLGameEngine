# 17. Reference Pong in plain C++

**Status:** a working prototype used as the yardstick for the XML engine; several of its techniques are not in the engine yet

## Purpose

While the XML engine was being rebuilt, a plain C++ Pong (SFML 3, single file) was developed as the reference: each piece of behavior is identifiable as something that could become a declarative verb. The author's stated aim is for it to serve as the reference implementation for the XML engine.

## Structure choices

- **Named tunables.** Every value a designer might change is a named variable at the top of the file: window size, paddle width and height, ball radius, speeds, framerate, title, and so on. Inside the code these fill in `paddle.width`, the video mode, and so on. This maps directly to a game file's `<variables>` block. Magic numbers were pulled up into that list.
- **One test point for optional output.** Drawing text depends on a font loading. Rather than testing everywhere, the font loader returns no font if none is found, and one helper (`drawScore`-style) is the single place that decides whether to draw text at all. Optional visuals are collected into a single drawables list.
- **Generic per-object helpers** (`move`, `collide`, `score`, `draw`) instead of many `somethingCollision` / `updateSomething` / `handleSomething` functions per named object.
- **Font search paths** live in a function at the bottom of the file so the top stays a list of knobs.
- `<random>` instead of `rand()`.

## Collision techniques

- **Swept collision against a moving frame of reference:** the ball and the paddle are both moving, so the test uses the relative velocity, expanding the paddle by the ball's radius and finding the earliest time of entry in the range 0 to 1 (the slab method). This stops fast objects tunnelling through thin ones.
- **Broad phase must sweep both objects,** not just one; a fast paddle can skip past a slow ball.
- **Push-out on penetration** along the contact normal, plus reflecting the direction.
- **Variable bounce:** the exit angle depends on where the ball hits the paddle and on the paddle's motion; the ball speeds up on each hit and there is a speed cap. The angle from where it hits is now in the engine as `<deflect>` ([46](46-paddle-deflect.md)); the paddle's motion and the speed-up are not.
- **Frame-time cap.** A maximum delta time (50 ms) so dragging the window cannot let the ball skip through a paddle.
- **Wall bounces** clamp the position back inside the window after flipping, otherwise a ball still overlapping the wall re-triggers every frame and jitters along the edge.
- A version with a small paddle "wiggle" (limited horizontal movement in and out) and forward/back speed effects existed; hitting the top or bottom of a paddle pushes the ball to the front face and sends it out at a steep angle.

Review of this file by other tools raised: a dangling-reference risk in the drawables list (addresses of objects in vectors), tightening the top or bottom edge hit so only real face hits count, and a note that nothing was declared "reference quality" until it had run for a while. Untested claims at the time: the swept-math versions had not been compiled by the assistant.

## Polish note

A hand-tuned nudge (for example shifting the left score four pixels when the score is exactly `1`, because the thin glyph looks too close to the center line) is the sort of unprincipled detail that gives a real game extra polish. If this is ever wanted declaratively, it would be a per-glyph or per-value offset in the text vocabulary.

## SFML 3 migration notes

- `sf::VideoMode` takes a `Vector2u`; `pollEvent()` returns `std::optional<sf::Event>`, and events are checked with `event->is<T>()` or `getIf<T>()`.
- `sf::FloatRect::intersects()` became `findIntersection()`, which returns an optional rectangle (`sf::Rect<T>`, so it works for integer rects too).
- `getLocalBounds()` returns a rect with `.position` and `.size`, not `.left/.top/.width/.height`.
- `sf::Text` has no default constructor and needs the font at construction.
- Default origin is the top-left for shapes; a circle's visual center needs the origin set to its radius.
- Smaller local models tend to write SFML 2 code; point them at the SFML 3 docs.

## Sources

- "Building a Pong game with SFML 3" (2026-09-23).
- "SFML Pong game displaying blank screen" (2026-09-24), the long refinement of `main.cpp`.
- "Fixing pong collision and ball sticking issues" (2026-07-13), "Refactoring SFML Pong game code" (2026-07-11).
- "SFML 3 intersects function migration", "SFML 3 default object origin" (2026-09-26).
