# pong_min, written as code

A prototype of what `xgecli --generate windows-cpp` should write: `games/pong_min.xml` as a program that decides what happens, while the engine's own modules do it. Written by hand for now; the next step is XSLT writing it.

| File | What |
|---|---|
| `source/pong_min.cpp` | The game: its objects in file order, and its answers to `xge::FrameRules` (what happens at an edge, on a touch, on a key) |
| `source/main.cpp` | The loop: keys, a frame, the picture |

It is built (`pong_min_prototype`, in `output/<config>/`) from the engine's frame, verbs, geometry, collisions, object, pictures, random numbers and SFML window, and nothing else: no `game.cpp`, no expression library, no XML library. `tests/test_prototype_pong.cpp` plays it against the engine playing `pong_min.xml`, frame by frame. See [the design notes](../../docs/designs/40-generator.md#prototype-the-game-as-decisions-on-the-engines-modules).
