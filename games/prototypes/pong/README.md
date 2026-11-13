# pong, as a person would write it

`games/descriptions/pong.xml` written as a person would write it: the second example of what `xgecli --generate windows-cpp` is to write (plan task A5, step 5), a game of four states, texts, sounds, a choice kept from one game to the next and a computer player. It decides what happens; the engine's own small functions do it.

| File | What |
|---|---|
| `source/pong.cpp` | the game: its numbers, its objects in the file's order, each state's keys, the computer's play, and its answers to `xge::FrameRules` |
| `source/pong.h` | its class, `pong::Pong` |
| `source/main.cpp` | the loop on SFML: keys, a frame, the texts measured and placed, the sounds, the picture |

How it is written:

- The states are an `enum` and a stack; the state on top is played, and paused and game over draw the game still beneath them.
- A key goes to the state it is pressed in, and is let go there; when the state changes, what held keys were doing stops, and a paddle key still down moves its paddle again (after a pause).
- The computer holds F11 or F12, keys of its own, as the file says, so it goes through the same keys a player's do.
- A text is placed once the window has measured it (`place()`), since its size is the font's.
- What is kept (who plays the right paddle, how well the computer plays) is two numbers in `pong-kept.txt`.

It is built as the `pong` program in `output/<config>/`. `xge/tests/engine/test_prototype_pong_full.cpp` plays it beside the engine playing `pong.xml` for five seeds of 9,000 frames (the title, play, a pause, game overs, a game for two) and checks every object every frame.
