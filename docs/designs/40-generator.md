# Generating a program

**Status:** Built: `xgecli --generate engine-cpp` covers every game. XSLT writes the game file out as a tree of C++ data, and the program reads it with the engine's own reader and plays it with the engine's own code on SFML. A test plays each generated game beside the engine's. Reference: [programs](../reference/programs.md#generating-a-program).

## What we looked for

The format was chosen partly so a game could be transformed with XML's own tools: XSD to check it, XSLT to turn it into a program of its own. The rule set later: **a generated game must play exactly as xgecli and xgegui play it**, using the same code for the frame, physics, collisions, keys and sound, differing only in having no XML library and no choice of libraries. And it stays XSLT.

## What we looked at

| Option | Verdict |
|---|---|
| G1. XSLT writes the rules out as C++ statements | Readable, but every verb and the frame order written a second time. Built first, drifted, replaced |
| G2. Embed the XML and link the whole engine | Plays the same, but is the engine with one game, and no transform at all |
| G3. C++ writes the engine's data, runtime copied in | Plays the same, but no XSLT. Built and replaced the same day |
| G4. XSLT writes the engine's data, runtime copied in | A second reader, in XSLT, kept honest by a test against `readGame`. Built and kept until the second reader cost more than it gave |
| **G6. XSLT writes the file as a tree, the engine's reader reads it** | Chosen. One reader; the program carries it and reads its tree at start |
| G5. xgecli dumps XML, XSLT templates it into C++ | Uses the tool without the point of it |

## How it runs

- xgecli first loads the game as it would to play it, so a broken game never reaches the stylesheets. Then `generate.xsl` runs with libxslt and xgecli copies what it lists.
- **`tree.xsl`** writes `source/<game>.cpp`: every element a `TreeElement` (`xml/tree.h`) with its name, attributes, trimmed text and children, each list a static array. `viewOfTree` shows it as an `XmlNode`, and `readGame` (`load/xml.cpp`) reads it as it reads a file: pieces, `from=` and pictures are worked out there.
- **`program.xsl`** writes `main.cpp`, `CMakeLists.txt` and a readme; **`engine.xml`** lists the engine files copied byte for byte from `xge/lib/`, the reader among them (none of the XML libraries, other backends or factories).
- **Why it plays the same:** the reader is the engine's, and after it everything is the same code. `xgetest-generated` plays both for 900 frames with the same keys and seed, comparing every object each frame.
- **Lines are 0** in the tree: XSLT 1.0 cannot say a node's line, and the file was checked before it was written out, so the reader never has a fault to place.
- **The second reader it replaced** (`data.xsl` writing a line of C++ per tag, `merge.xsl`, `game_parts.h`, about 6,000 lines with its test) had to change with every field `load/xml.cpp` gained. It went once `windows-cpp` took over being the readable target.

## What G1 taught

The first target wrote each game as a program a person might write (Pong about 480 lines). Its verbs moved into the engine's `play/geometry.cpp` and the sweep was shared, but differences remained (thrust applied as the key was read, members never touching each other). **A copy of the engine's behaviour, however carefully kept, drifts.** So the stylesheets write only what the file says.

## A readable game on the shared frame, then on its own

- The first aim: a generated game that reads like a program written for it and plays exactly the same by calling the engine's code for the work.
- **Built and kept for the engine:** the frame order moved out of `Game` into `play/frame.cpp` (`playOneFrame` asks a `FrameRules` what the game says), and the verbs into `play/verbs.cpp`.
- **Tried for `windows-cpp`:** `pong_min` and then `pong` by hand on that frame, matching the engine every frame (`pong` about 530 lines). The author read it and turned it down: too much engine in it. A stand-alone game should not mention the engine at all, nor carry its verbs, `Object` and layers; it needs a few SFML shapes, simple collision and generated sounds, and should do the same things, not the same sums. Writing it found an engine fault: a move in a `<condition>` of an action was never let go.
- **Now:** both prototypes stand alone, linked to SFML only. `games/prototypes/pong_min/` is exactly what the generator writes (but for the date); `games/prototypes/pong/` is the next one to write (about 300 lines of rules, 190 of window, text and square-wave tones, `tuffy.ttf` beside it).
- **How `windows-cpp` writes:** the game's numbers become constants in the header, each object a `Thing` (position, velocity, size) named as in the file, and an object that starts again (a `<reset />`, its own variables) a start function whose locals are its variables, a `<random>` drawn anew each time. `key()` only notes which keys are down; `frame(seconds)` is the keys held (a `<move>` per held key), every thing moving by its velocity times the seconds passed (`main` measures them, as the engine does), the edges (an `if` per edge, or one when the rule does the same at each; a group a loop over its members) and the meetings (`touching()`, then the verbs). Small functions (`sine`, `sign`, `touching`, `deflect`) are written in only when used. Each verb is a file in `verbs/` with a template per place it can stand (`held`, `edge`, `touch`), so a new verb is a new file.
- **How it is checked:** what it does, not every frame. A game's checks are a function template in `xge/tests/written/<game>.h`, run on the prototype and on the generated copy; `test_generate.cpp` holds the generated text to the prototype and that nothing in it names the engine.
- **It taught:** values were the hard part while it matched the engine: the old expression library regrouped constants, and the order of random draws had to match. Standing alone, a value is written as the file nests it and the game draws its own random numbers.

## Where it goes next (chosen by the author)

- **The point of a description is to free a game from its platform,** so a target should write the game itself, standing alone, in that platform's own terms: a program that reads as if someone wrote it by hand over a weekend.
- `engine-cpp` (once called `windows-cpp`) stays as the exact target that covers every game, and gets no new design work: it is the engine copied over with the game's data in it.
- `windows-cpp` is the name for the new one: a generic, single-purpose looking Windows program on SFML, as the author put it. It grows beside `engine-cpp` game by game, checked by what each game does ([plan](../plan.md#a5-a-game-written-out-as-if-by-hand)). This is G1 again, now on purpose: the written game is its own code, and `engine-cpp` remains the exact copy.
- Since nobody reads `engine-cpp`'s game, it became the file as a tree for the engine's own reader (G6), first turned down as further from a game anyone could read.

## Open

- Other targets (another language, an 8-bit machine) need the runtime in that language: a port, not a generator.
- Built with GCC and Clang on Linux, and MSVC at `/W4` by `games-build-all.bat`.
- Not yet in `windows-cpp`: colours other than white, a `<move>` for something with its own `<velocity>`, a touch rule on a group, and the deflect off a top or bottom is untried by any game.
- `windows-cpp` writes one game so far (`pong_min.xml`). For any other it lists every part it has no words for and writes nothing: what it covers is said as match patterns in `covers.xsl` and in each verb's file, so a new verb says where it may stand beside how it is written. The list doubles as the to-do for the next game.
