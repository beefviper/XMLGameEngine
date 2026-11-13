# pong_min, written by hand

`games/descriptions/pong_min.xml` written as a person would write it: the example of what `xgecli --generate windows-cpp` is to write (plan task A5). It decides what happens; the engine's own small functions do it.

| File | What |
|---|---|
| `source/pong_min.cpp` | the game: its numbers as constants, its objects in the file's order, and its answers to `xge::FrameRules` (what happens at an edge, on a touch, on a key) |
| `source/main.cpp` | the loop: keys, a frame at 60 a second, the picture, on SFML |

It is built as the `pong_min` program in `output/<config>/`. `xge/tests/engine/test_prototype_pong.cpp` plays it beside the engine playing `pong_min.xml` for ten seeds of 3,600 frames and checks every object every frame.
