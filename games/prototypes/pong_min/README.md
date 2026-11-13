# pong_min, stand-alone

`games/descriptions/pong_min.xml` written as a small SFML game that stands on its own: exactly what `xgecli --generate windows-cpp` writes for it (plan task A5), but for the files' date line. It uses none of the engine's code.

| File | What |
|---|---|
| `source/pong_min.h` | its numbers as constants, a `Thing` (where, how fast, how big) and the class with one `Thing` an object |
| `source/pong_min.cpp` | its rules: where the ball starts, which keys are down, and a frame (keys held, moves, edges, meetings) |
| `source/main.cpp` | the window: keys in, 60 frames a second, the picture out |

It is built as the `pong_min` program in `output/<config>/`, linked to SFML only. `test_generate.cpp` checks the generator still writes these files; `xge/tests/written/pong_min.h` holds what it must do, checked here by `test_prototype_pong.cpp` and on the generated copy by `test_written_games.cpp`.
