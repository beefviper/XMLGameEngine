# Generating a program

**Status:** Built: `xgecli --generate engine-cpp` covers every game. XSLT writes the game file out as the engine's own data, and the program plays it with the engine's own code on SFML. A test compares each game's data with what the engine reads, field by field, and plays the two side by side. Reference: [programs](../reference/programs.md#generating-a-program).

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
- **`merge.xsl`** works out the pieces and every `from=`, as `load/merge.cpp` does.
- **`data.xsl`** writes `source/<game>.cpp`: each tag becomes the line of C++ that makes what `load/xml.cpp` makes of it. A group is written as it is in the file: what it gives, a function per member, row or cell, and a line per cell naming its changes, so the data is about the file's size. Each cell is a call of its own, since inline cells overflowed a 1 MB Debug stack.
- **`modules/game_parts.h`** holds the small functions those lines call, where `load/xml`'s rules for combining parts live in C++.
- **`program.xsl`** writes `main.cpp`, `CMakeLists.txt` and a readme; **`engine.xml`** lists the engine files copied byte for byte from `xge/lib/` (none of the XML readers, other backends or factories).
- **Why it plays the same:** the data is checked, and after the data everything is the same code. `xgetest-generated` compares every field, then plays both for 900 frames with the same keys and seed, comparing every object each frame.
- Rejected: writing expressions as C++ (the engine works some out again while playing, and the two could differ); writing each member out whole (many times the file's size).

## What G1 taught

The first target wrote each game as a program a person might write (Pong about 480 lines). Its verbs moved into the engine's `play/geometry.cpp` and the sweep was shared, but differences remained (thrust applied as the key was read, members never touching each other). **A copy of the engine's behaviour, however carefully kept, drifts.** So the stylesheets write only what the file says.

## A readable game on the shared frame

- The aim: a generated game that reads like a program written for it and plays exactly the same by calling the engine's code for the work.
- **Built and kept:** the frame order moved out of `Game` into `play/frame.cpp` (`playOneFrame` asks a `FrameRules` what the game says), and the verbs into `play/verbs.cpp`. The engine and every generated game run on it.
- **The example, by hand:** `games/prototypes/pong_min/` is `pong_min.xml` written as a person would write it, its own `FrameRules` in about 150 lines, built as the `pong_min` program. `test_prototype_pong.cpp` plays it beside the engine for ten seeds of 3600 frames and they match every frame. Set aside once, it is back as what `windows-cpp` writes: the generator now writes these files but for their date, and where the two first differed the hand-written one took the generator's way (a `ballStart()`, `<centered />` as `windowWidth / 2 - size / 2`, a test per group).
- **The second example, by hand:** `games/prototypes/pong/` is `pong.xml` written first on the shared frame (about 530 lines, matching the engine every frame). The author read it and turned it down: too much engine in it. A stand-alone game should not mention the engine at all, nor carry its verbs and layers; it needs a few SFML shapes, simple collision and generated sounds, and should do the same things, not the same sums. It is now that: `pong.cpp` about 200 lines of plain rules (paddles, ball, screens, the computer), `main.cpp` the window, text, a square-wave tone and the drawing, linked to SFML only with `tuffy.ttf` beside it. `test_prototype_pong_full.cpp` checks what it does. Writing the first version found an engine fault: a move in a `<condition>` of an action was never let go.
- **How `windows-cpp` writes:** the game's numbers become constants, its objects accessors on one vector, and each `FrameRules` question a function made of the rules that answer it: an `if` per object or group, and in it an `if` per edge. Each verb is a file in `verbs/` with a template per place it can stand (`edge`, `touch`, `action`, `key`), so a new verb is a new file. A value is written as C++ in the order the engine works it out (`values.xsl`); a `<random>` lives in the object's start function, drawn by name as a reset draws it again.
- **What it leaves out:** the program compiles only the engine files in `runtime.xml`: the frame, the verbs' work, touching, numbers, random numbers, pictures and the SFML window. `game/object.cpp` holds what those need and `game/command.cpp` the printing and the verb list, so none of `Game`, the loader or XML comes along.
- The one-number operations a value is written with (`sign`, `sine`, `cosine`) are `play/numbers.h`, which the loader and a written-out game both call, so a serve comes out the same to the last bit.
- **How strict:** exact while it comes cheap. A difference that is only rounding is not chased: that game's check becomes what it does (serves, returns, paddles stop), not where every object is (the author's wish).
- **It taught:** values are the hard part. The old expression library regrouped constants, so C++ differed in the last bit; the engine's own `load/calc` now works a text exactly as written. The order of random draws (alphabetical by variable) still has to match.

## Where it goes next (chosen by the author)

- **The point of a description is to free a game from its platform,** so a target should write the game itself, standing alone, in that platform's own terms: a program that reads as if someone wrote it by hand over a weekend.
- `engine-cpp` (once called `windows-cpp`) stays as the exact target that covers every game, and gets no new design work: it is the engine copied over with the game's data in it.
- `windows-cpp` is the name for the new one: a generic, single-purpose looking Windows program on SFML, as the author put it. It grows beside `engine-cpp` game by game, checked by what each game does ([plan](../plan.md#a5-a-game-written-out-as-if-by-hand)). This is G1 again, now on purpose: the written game is its own code, and `engine-cpp` remains the exact copy.
- Rejected: making `engine-cpp` cheaper still by having XSLT write the XML tree for the engine's own reader. It plays the same, but it is further from a game anyone could read.

## Open

- `data.xsl` reads the file a second time: a new field in `load/xml.cpp` needs the same in `data.xsl`, and the data test fails until it has it.
- Other targets (another language, an 8-bit machine) need the runtime in that language: a port, not a generator.
- Built with GCC and Clang on Linux, and MSVC at `/W4` by `games-build-all.bat`.
- Still to change: `pong_min` and what the generator writes still call the engine's frame and verbs (built before the author's ruling). They move to the stand-alone shape of `games/prototypes/pong/`. Until then `windows-cpp` is held to the old proof: `xgetest-generated` plays each game it covers beside the engine, ten seeds, every object every frame (`test_written_games.cpp`). The written program has no hooks for it; the test drives the same `key()` and `frame()` its `main()` does.
- `windows-cpp` writes one game so far (`pong_min.xml`). For any other it lists every part it has no words for and writes nothing: what it covers is said as match patterns in `covers.xsl` and in each verb's file, so a new verb says where it may stand beside how it is written. The list doubles as the to-do for the next game.
