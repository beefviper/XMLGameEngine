# Generating a program

**Status:** Built: `xgecli --generate windows-cpp` covers every game. XSLT writes the game file out as the engine's own data, and the program plays it with the engine's own code on SFML. A test compares each game's data with what the engine reads, field by field, and plays the two side by side. Reference: [programs](../reference/programs.md#generating-a-program).

## What we looked for

The format was chosen partly so a game could be transformed with XML's own tools: XSD to check it, XSLT to turn it into a program of its own. The rule set later: **a generated game must play exactly as xgecli and xgegui play it**, using the same code for the frame, physics, collisions, keys and sound, differing only in having no XML to read and no choice of libraries. And it stays XSLT.

## What we looked at

| Option | Verdict |
|---|---|
| G1. XSLT writes the rules out as C++ statements | Readable, but every verb and the frame order written a second time. Built first, drifted, replaced |
| G2. Embed the XML and link the whole engine | Plays the same, but is the engine with one game, and no transform at all |
| G3. C++ writes the engine's data, runtime copied in | Plays the same, but no XSLT. Built and replaced the same day |
| **G4. XSLT writes the engine's data, runtime copied in** | Chosen. A second reader, in XSLT, kept honest by a test against `readGame` |
| G5. xgecli dumps XML, XSLT templates it into C++ | Uses the tool without the point of it |

## How it runs

- xgecli first loads the game as it would to play it, so a broken game never reaches the stylesheets. Then `generate.xsl` runs with libxslt and xgecli copies what it lists.
- **`merge.xsl`** works out the kit and every `from=`, as `load/merge.cpp` does.
- **`data.xsl`** writes `source/<game>.cpp`: each tag becomes the line of C++ that makes what `load/xml.cpp` makes of it. A group is written as it is in the file: what it gives, a function per member, row or cell, and a line per cell naming its changes, so the data is about the file's size. Each cell is a call of its own, since inline cells overflowed a 1 MB Debug stack.
- **`modules/game_parts.h`** holds the small functions those lines call, where `load/xml`'s rules for combining parts live in C++.
- **`program.xsl`** writes `main.cpp`, `CMakeLists.txt` and a readme; **`engine.xml`** lists the engine files copied byte for byte from `xge/lib/` (none of the XML readers, other backends or factories).
- **Why it plays the same:** the data is checked, and after the data everything is the same code. `xgetest-generated` compares every field, then plays both for 900 frames with the same keys and seed, comparing every object each frame.
- Rejected: writing expressions as C++ (the engine works some out again while playing, and the two could differ); writing each member out whole (many times the file's size).

## What G1 taught

The first target wrote each game as a program a person might write (Pong about 480 lines). Its verbs moved into the engine's `play/geometry.cpp` and the sweep was shared, but differences remained (thrust applied as the key was read, members never touching each other). **A copy of the engine's behaviour, however carefully kept, drifts.** So the stylesheets write only what the file says.

## A readable game on the shared frame (set aside)

- The aim: a generated game that reads like a program written for it and plays exactly the same by calling the engine's code for the work.
- **Built and kept:** the frame order moved out of `Game` into `play/frame.cpp` (`playOneFrame` asks a `FrameRules` what the game says), and the verbs into `play/verbs.cpp`. The engine and every generated game run on it.
- **Tried and removed:** Pong written by hand as its own `FrameRules` (about 150 lines), matching the engine for ten seeds of 3600 frames. In the history up to 7163ea2.
- **It taught:** values are the hard part. The old expression library regrouped constants, so C++ differed in the last bit; the engine's own `load/calc` now works a text exactly as written. The order of random draws (alphabetical by variable) still has to match.

## Open

- `data.xsl` reads the file a second time: a new field in `load/xml.cpp` needs the same in `data.xsl`, and the data test fails until it has it.
- Other targets (another language, an 8-bit machine) need the runtime in that language: a port, not a generator.
- Built with GCC and Clang on Linux, and MSVC at `/W4` by `games-build-all.bat`.
