# The engine

How a game is loaded and played, the libraries behind it, how to build it, and where each part of the code lives.

## Loading

1. **Parse and validate** (`load/xml`). If the file names a schema it is validated: Xerces fully ("strong"); the other XML libraries with a small built-in validator for the XSD the schema uses ("weak", `load/xsd`).
2. **Work out the pieces and every `from=`** on the XML (`load/merge`).
3. **Read** the tags into a `GameData` (`game/data.h`), a group as one raw object per member. A generated program starts here, with its `GameData` compiled in.
4. **Lay out states and scenes** (`layOutScenes`, `game/scenes.cpp`): owned objects named by path, every name written out in full.
5. **Evaluate** (`load/expr`, `load/calc`): every value worked out (`<random>` drawn), objects, states and sounds built, every name checked. Nothing needs a window yet.
6. **Open the window and sound** (`Engine`): sounds made into samples, sizes measured, size-dependent positions finished, the first state pushed. If sound will not start the game plays silently, with a warning.

## Each frame

`Engine::step` reads the keys (or the demo's, or the computer's) and runs the current state's bindings for each press and release, then its `on="hold"` bindings. Then `playOneFrame` (`play/frame.cpp`):

1. animations and blinking count;
2. timers count, and those due run;
3. paths set their followers' velocities; maze walkers choose; held moves on cells step;
4. climbing, grip, acceleration, held thrust, turns and swings, pulls and seeking, altitudes;
5. screen-edge rules for what is moving;
6. **the move**: hops are made, carried things lent their carrier's motion, everything moves (a swing turns through it), and each touch found on the way runs its rules;
7. sticks hold, carried things are placed, `update="live"` numbers are worked out again (`Game::follow`), tethers and shadows follow, light and dark worked out, poses picked;
8. conditions: the state's, then its current scene's;
9. copies made join, despawned ones go.

Then the sounds asked for play, loops start or stop, and the frame is drawn: what a `<show state>` shows still, then what is in play, except objects not `<drawn>`.

An object is **in play** when it is not out of play (`<active>false</active>`, dead, despawned) and belongs to the current state, its current scene, or the game, or is shown by the state. Movement is pixels per frame and timers count frames; there is no time step.

## Backends

| Job | Interface | Libraries |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml/document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keys, mouse, pads | `Window` (`window/window.h`) | SFML 3 (default), raylib, SDL2, OpenGL (GLFW); `xgegui`'s Qt renderer |
| Sound | `Audio` (`audio/audio.h`) | SFML 3 (default), raylib, SDL2, none |

- `Game` and `Engine` see only the interfaces. One factory per kind (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`) knows every library.
- **A build has some, not all**: SFML 3 and Xerces by default; `-DXGE_WITH_<NAME>=ON` adds one, `-DXGE_ALL_BACKENDS=ON` or `-DBUILD_TESTING=ON` all. A library's code compiles only behind `XGE_WITH_<NAME>`. Each factory says what it has (`available()`, `availableBackends()`, `defaultBackend()`, `name()`); `create()` of one not built throws, naming those that are.
- **Pictures** are drawn by the engine, so a window library only shows a bitmap; only text and image files are its own.
- **Sound** is made by the engine (`synthesize()`, `audio/sound.h`), so it is the same whichever library plays it. `Game` never makes a noise: `<play>` asks, and `Engine` hands the frame's sounds to its `Audio`. Window and sound libraries are chosen apart; any goes with any.
- `Engine::replaceWindow()` swaps the window of a running game (the old destroyed first), `Engine::replaceAudio()` the sound, `Engine::silence()` stops what plays, `Engine::pump()` keeps a paused window responsive.

Design: [backends](../designs/50-backends.md), [gui](../designs/53-gui.md).

## Building

```
cmake -B build                      # SFML 3 and Xerces
cmake --build build --parallel
output/Debug/xgecli frogger
cmake -B build -DBUILD_TESTING=ON   # every backend, and the tests
output/Debug/xgetest                # Catch2 filters work: "[kaboom]"
cmake --build build --target xgetest-generated && output/Debug/xgetest-generated
```

| Target | What |
|---|---|
| `xgelib` | the engine (`xge/lib/`), static by default (`-DXGE_BUILD_SHARED=ON` for shared) |
| `xgecli`, `xgegui` | the programs (`xge/cli/`, `xge/gui/`; the second only when Qt 6 is found) |
| `xgetest`, `xgetest-generated` | the tests; the second generates every game and is never in an ordinary build |
| `xgedata` | copies `games/` beside the programs |

- **Output.** Everything to run lands in `output/<Config>/`: the programs, a copy of `games/` with the schema, and the libraries in `libraries/` (found through a manifest on Windows, a relative run path elsewhere). The folder can be copied anywhere.
- **Dependencies** are found through vcpkg or the system, or fetched and built (`FORCE_LOCAL_<NAME>` forces a fetch). Qt 6 and libxslt are only found, never fetched. CMake modules are in `scripts/cmake/`.
- **Warnings** stay at zero under a strict set on GCC, Clang and MSVC (`xge_warnings()`, `platform.cmake`).

Design: [build-and-layout](../designs/51-build-and-layout.md).

## Source map

| Part | Files | What |
|---|---|---|
| reading | `load/xml.cpp` | tags into a `GameData` (`readGame()`) |
| | `load/reading.cpp` | what every reader shares: a child, an attribute, a value, `fail` |
| | `load/merge.cpp` | pieces and `from=` on the XML (`layOut`, `addWaiting`), as `merge.xsl` does |
| | `load/expr.cpp` | values, `<formula>`, building `Object`s and `State`s, pictures (`buildPicture`) |
| | `load/calc.cpp` | arithmetic in text, compiled once into steps |
| | `load/check.cpp` | load-time checks of names, and of where commands may go (each verb's places) |
| | `load/xsd.cpp`, `load/spelling.cpp` | the weak validator; "did you mean" (`didYouMean`, `listOf`) |
| the game | `game/game.cpp` | the interpreter: objects, the state stack, scenes, conditions, resets, paths, spawns, the answers to `FrameRules` |
| | `game/scenes.cpp` | `layOutScenes`, states `from=` others |
| | `game/command.cpp` | raw command tags into typed `Command`s (`makeCommand`, by the verb), printing them; `operationShape` |
| | `verbs/` | a file per verb: its name, the places it may go, `read`, `make`, `check` and `run`; `verb.h` says what a verb is, `all.cpp` lists them |
| | `game/object.*`, `game/states.*`, `game/data.h`, `game/random.cpp` | the data model; the one random source |
| | `game/cells.cpp`, `game/cards.cpp`, `game/think.cpp` | maps of cells and board games; cards; the board-game search |
| the frame | `play/frame.cpp` | `playOneFrame`: the order of a frame and the moving |
| | `play/geometry.cpp`, `play/collision.cpp` | touching (box, circle, swept, pixel walk, `contactOf`) and the verbs' arithmetic |
| | `play/verbs.cpp`, `play/executor.cpp` | the verbs' work on objects; running a command by its verb, and the work that needs the game |
| | `play/maze.cpp` | walking a map's tiles |
| pictures | `picture/bitmap.cpp`, `svg.cpp`, `image.cpp`, `font.cpp`, `color.cpp` | pixels, flips and turns; SVG with lunasvg; picture sheets cut and shrunk (`<sheets>`); fonts and the 8x8 fallback; colour names |
| loop and libraries | `engine/engine.cpp`, `backends.cpp`, `keys.cpp`, `folder.cpp` | the loop (`loop()`, or `step()` and `render()`), choosing libraries, key names, the games folder and `findSchema` |
| | `window/*`, `audio/*`, `xml/*` | each kind's interface and factory, a file per library; `audio/sound.cpp` is the synthesizer |
| programs | `xge/cli/source/` | `cli.cpp` (options, finding the game), `generate.cpp` (`--generate`), `main.cpp` |
| | `xge/gui/source/` | `main_window`, `game_session`, `game_stage`, `game_view`, `qt_window`, `key_queue`, `options_dialog`, `session_options`, `app_settings` (`xgegui.ini`), `inspector`, `theme` |
| generator | `games/generators/windows-cpp/` | `generate.xsl`, `merge.xsl`, `data.xsl`, `program.xsl`, `text.xsl`, `engine.xml`, `modules/game_parts.h` |
| tests | `xge/tests/` | one `test_<topic>.cpp` per area and per game (played frame by frame). `played.h` is shared, `invaders_fixture.h` a frozen Space Invaders, `scratch.h` a scratch name per run. `xgetest "[.speed]"` prints load and frame times |
