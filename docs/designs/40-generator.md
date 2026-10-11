# Generating a program

**Status:** Built: `--generate windows-cpp` covers every game. A generated program is the game written out as the engine's own data, played by the engine's own code on SFML; a test plays each one against its file, frame by frame.

## Approach

**What we looked for.** The format was chosen partly so a game could be transformed ([file-format](02-file-format.md)): a way to turn a game file into a program of its own, as a test of whether the description really is the game, and as the start of other targets. Later the author set what matters most: a generated game must play exactly as xgecli and xgegui play it, using the same code for the frame, physics, collisions, keys and sound, and differ only in having no XML to read and no choice of window and sound libraries (SFML only).

**What we looked at.**

| Option | Notes |
|---|---|
| G1. XSLT writes the rules out as C++ statements | The format's own tool; readable programs (`physics::bounce(ballVelocity, edge)`). Every verb, the frame order and the input handling written a second time, in the stylesheets. Built first (below), then replaced |
| G2. The program embeds the XML and links the whole engine | Plays the same by construction, but carries an XML library and the backend choosing: the engine with one game, not a program of its own |
| G3. The game as the engine's own data, the engine's runtime copied in | Read the file with the engine's reader at generation time and write what it read (`GameData`: the window, variables, `RawObject`s, `RawState`s, sounds, paths) as C++; the program builds `Game` from it and runs the engine's `Engine` on an `SFMLWindow` and `SFMLAudio` |

**Chosen: G3, written by C++ in xgecli (`cli/source/generate.cpp`).** What changed in the engine for it:
- `GameData` (`game_data.h`) is the game as written. `game_xml` reads a file into one (`readGame(file, backend)`), and `Game(GameData)` works it out and runs it, so nothing in `game.cpp` reads XML. `Game(file)` stays for the tests and is defined in `game_xml.cpp`.
- Engine's constructor that picks libraries by name moved to `engine_backends.cpp`; `engine.cpp` takes a `Window` and an `Audio` it is handed. The factories (`window.cpp`, `audio.cpp`) are only for picking.
- `game_expr::fixRandomSeed` lets a test load two copies of a game that draw the same random numbers.

What a program is: `source/<game>.cpp`, the game as data, one function per object, one per `<group>` (each member written as what it changes of the one before, so Space Invaders' 55 aliens are short), one per state, then `<game>Game()` which returns the `GameData`; `source/main.cpp`, which makes the `Game`, an `SFMLWindow` and an `SFMLAudio` (silence if there is no sound device, as xgecli does) and calls `Engine::loop`; `engine/`, the engine's files copied byte for byte from `lib/` (`engineFiles()`, mirrored in `data_engine` in `assets.cmake`): everything `Game` and `Engine` are made of, `game_expr` with exprtk for the expressions, `svg` with lunasvg, `window_sfml` and `audio_sfml`, and none of `game_xml`, `xml_*`, `xsd_lite`, the other backends or the factories; `assets/`, the font and pictures it uses; and a `CMakeLists.txt` that finds or fetches SFML 3, exprtk and lunasvg and compiles with the engine's own warnings. The data is written field by field, only what differs from a default, with small helpers (`value("...")`, `random(...)`, `verb("bounce")`, `make<RawLine>(...)`) written only when used; a game the engine will not load is refused, and a folder made for it removed.

Why it plays the same: after the data, everything is the same code. `test_generated_games.cpp` proves it for every game: the build generates each one (`tests.cmake`), compiles its data into `xgetest`, and plays it beside the game read from its file for 900 frames with the same keys and random seed, comparing every object's place, velocity, look, heading, variables and whether it is in play, and the state, each frame.

**Rejected.** Writing the expressions as C++ (no exprtk in the program): the engine evaluates them at load and some again while playing (a timer's `<random>`, a position that waits for a text's size), so the program would evaluate them its own way and could differ. Copying the Raw structures out of the XML in XSLT: `game_xml` already reads them, and a second reader would drift.

### The first target: rules as statements in XSLT (replaced 2026-10-11)

XSLT 1.0 stylesheets (libxslt with EXSLT, run by xgecli) wrote each game as a program a person might write on SFML: tunables, objects as SFML shapes, a function per screen and per object, each rule an `if` (generated Pong about 480 lines). It was grown from `games/pong_min.xml` to every game. Its verbs were first copies of the engine's; they were moved into the engine's `geometry.cpp` (shared by both), and the frame was rewritten in the engine's order with the sweep loop shared as `xge::playFrame`. Differences remained (thrust and turn applied as the key was read, a group's members never touching each other, `unless=` by shapes, key presses found by comparing frames), each a second copy of something the engine does, so the author asked for the engine's own code instead, and the stylesheets, `physics.h` and the libxslt dependency were removed. What it taught: a copy of the engine's behavior, however carefully kept, drifts; sharing arithmetic is not enough when the order, the input and the rules around it are written again.

**Open.** The generated data is long for a game with many groups of objects that differ (Berserk, Asteroids, about 2,600 lines). Other targets (another backend, another language, an 8-bit machine) would need the engine's runtime in that language, which is a port, not a generator. Built with GCC and Clang on Linux; MSVC not yet tried with this target.
