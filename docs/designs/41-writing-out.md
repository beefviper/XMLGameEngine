# Writing a game out

**Status:** a running record, kept as each game is written out. The suggestions are ideas. Reference: [generating a program](../reference/programs.md#generating-a-program).

## Why this matters most

- The description is the point. A game file should play on any platform. The engine is for testing the games and playing them, and is not the only way to run them (the author, after `pong.xml` was written out).
- So a change to the language is also weighed by how easily `windows-cpp` and `web-js` can write the game out as plain code, and not only by how easily the engine can run it.
- The gap list from `covers.xsl` is the measure: a part that is hard to write out shows up there first.

## What made Pong easy to write out

Pong had been written by hand on the engine's frame a month earlier, at about 530 lines that read as engine code. These changes since then are what let XSLT write it in the shape a person would:

| Change | What it gave the written game |
|---|---|
| Every number per second ([time](29-time.md)) | moving is `position += velocity * seconds`, with no frame counts to carry |
| No arithmetic in strings ([arithmetic](03-arithmetic.md)) | a value is a tree, so XSLT writes `a + b * c`, bracketing only where precedence needs it, with no parser |
| One name per attribute; no reserved names | each covered part is a plain match pattern (`collision[@edge]/bounce`), with nothing split or guessed |
| Conditions holding conditions | already shaped like `if` inside `if`: `computerHolds()` came out as written |
| Seats, `<people>`, `<computer seat>` | "the computer holds seat 2's keys while fewer play" is exactly two bools and one function |
| An object's start values; objects in states | an object that starts again becomes a `startBall()` function |
| A file per verb | a target's `verbs/` mirrors the engine's, so a new verb is a new file in each |

The largest single step was not a language change. It was dropping the frame-for-frame match with the engine: once the written game only had to do the same things, it could be plain code.

## What was still hard

- **What a part is for.** The file does not say which objects move or meet, which are only drawn, and which need a struct of their own. `kinds.xsl` works this out from use. It holds for Pong; Breakout's groups and looks will test it.
- **Centred words.** A centre needs the words' size, which only the window knows, so `main.cpp` measures before drawing.
- **What the engine does without being told.** A target has to copy each of these, and nothing in the file shows them:
  - a state's conditions stop at the first that holds, but every matching touch rule runs;
  - a refused press stops the verbs after it;
  - live numbers are worked out once a frame;
  - a reset also clears what is in flight.

## Suggestions

Each is an idea, to be weighed when the game that needs it is written out.

- **A page of what a target must do.** This would sit beside "What a window must do" in the [engine reference](../reference/engine.md), and list every behaviour above that a written game must copy. It would turn hidden rules into a checklist and show which ones are worth making explicit in the file.
- **Anchors** ([values-and-names](04-values-and-names.md), idea). `anchor="center"` would replace the common `windowWidth / 2 - size.x / 2` with a word that each platform writes in its own way.
- **A new verb should be easy to write out.** When adding one, ask whether it can be written as about ten lines of plain code. If it needs a subsystem (sweeps, pixel collisions, the board search), say so in its design. Those stay `engine-cpp` and `web-asm` only until a target needs them.
- **Regular groups.** Rows times columns become one loop over an array. Per-member exceptions and numbered names (`x.N`) each need special code, so prefer a look or a variable on the member.
- **Count the gaps.** A long test could run every game through `covers.xsl` and count how many games each gap blocks, giving a ranked to-do for the readable targets.

## Open

- Whether `kinds.xsl`'s guesses should become words in the file. So far, inferring them has been enough, and that keeps the file free of code-shaped hints.
