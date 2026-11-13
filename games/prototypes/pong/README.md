# pong, stand-alone

`games/descriptions/pong.xml` written by hand as a small SFML game that stands on its own: the example of what `xgecli --generate windows-cpp` is to write for a game of screens, texts, sounds, a kept choice and a computer player (plan task A5, step 5). It uses none of the engine's code.

| File | What |
|---|---|
| `source/pong.h`, `source/pong.cpp` | the game: paddles, ball, scores, the four screens, the computer's play, the kept choices |
| `source/main.cpp` | the window: keys in, 60 frames a second, the picture, the text, square-wave sounds |

- The screens are an `enum` and a stack; paused and game over draw the game still beneath them.
- A paddle moves while its key is down, so one held through a pause goes on moving.
- The computer moves the right paddle itself: easy keeps level with the ball, smart waits in the middle until the ball comes over the net.
- Who plays the right paddle and how well the computer plays are two numbers in `pong-kept.txt`.
- The font, `tuffy.ttf`, sits beside the program.

It is built as the `pong` program in `output/<config>/`, linked to SFML only. `xge/tests/engine/test_prototype_pong_full.cpp` checks what it does: the title's choices, the serve, bounces, returns, points, a pause, game over, the computer and the kept choices.
