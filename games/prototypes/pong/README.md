# pong, stand-alone

`games/descriptions/pong.xml` written as a small SFML game that stands on its own: exactly what `xgecli --generate windows-cpp` writes for it (plan task A5, step 5), but for the files' date line. It uses none of the engine's code.

| File | What |
|---|---|
| `source/pong.h` | its numbers as constants, a `Thing` (where, how fast, how big), a struct on it where one counts (`Paddle`'s score, `Ball`'s hits), the `Computer`, and the class |
| `source/pong.cpp` | its rules: where things start, the keys and pads, the screens, a frame (keys held, moves, edges, meetings, the game's own tests), the computer's play, the kept choices |
| `source/main.cpp` | the window: keys and pads in, a step of the time since the last frame (at most a quarter of a second), the picture, the text, square-wave sounds |

- The screens are an `enum` and a stack; paused and game over draw the game still beneath them.
- The computer holds the right paddle's keys while one person plays: easy keeps level with the ball, smart waits in the middle until the ball comes over the net.
- How well the computer plays is kept in `pong-kept.txt`, two numbers.
- The font, `tuffy.ttf`, sits beside the program.

It is built as the `pong` program in `output/<config>/`, linked to SFML only. `test_generate.cpp` checks the generator still writes these files; `xge/tests/written/pong.h` holds what it must do, checked here by `test_prototype_pong_full.cpp` and on the generated copy by `test_written_games.cpp`.
