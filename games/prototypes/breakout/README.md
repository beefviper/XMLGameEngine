# breakout, stand-alone

`games/descriptions/breakout.xml` written as a small SFML game that stands on its own: exactly what `xgecli --generate windows-cpp` writes for it (plan task A5, step 6), but for the files' date line. It uses none of the engine's code.

| File | What |
|---|---|
| `source/breakout.h` | its numbers as constants, a `Thing` (where, how fast, how big), a struct on it where one counts or has looks (`Brick`, `Player`, `Ball`), the `Status` and the `Message`, and the class |
| `source/breakout.cpp` | its rules: where things start (the wall a colour at a time), the keys, pads and mouse, the screens, a frame (keys held, moves, edges, meetings, the game's own tests) |
| `source/main.cpp` | the window: keys, pads and the mouse in, a step of the time since the last frame (at most a quarter of a second), the picture, the text, tones that slide and change wave |

- The wall is a `std::vector<Brick>`: a brick knows its look, its worth and its sound, and goes from the list when the ball meets it.
- The screens are an `enum` and a stack; paused and game over draw the game still beneath them.
- The font, `tuffy.ttf`, sits beside the program.

It is built as the `breakout` program in `output/<config>/`, linked to SFML only. `test_generate.cpp` checks the generator still writes these files; `xge/tests/written/breakout.h` holds what it must do, checked here by `test_prototype_breakout.cpp` and on the generated copy by `test_written_games.cpp`.
