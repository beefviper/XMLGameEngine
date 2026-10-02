# XMLGameEngine

XMLGameEngine is a VGDL (video game description language) written in XML, plus a C++ engine that loads a game description and runs it. A whole game (window, variables, objects, screens) lives in one `.xml` file, checked against an XSD. The file is declarative: no loops, no `if`, no function calls. Behavior comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<stick />`, `<die />`, `<hop direction="up">`, ...) that the engine knows how to carry out.

The target is to describe the 2D non-scrolling games of the late 1970s and early 1980s with a small, well-chosen vocabulary, and to grow that vocabulary only when a real game cannot be described without a new word.

## Games that run today

| File | Game |
|---|---|
| `games/pong.xml` | Pong, with a menu, pause and game-over screens |
| `games/breakout.xml` | Breakout |
| `games/spaceinvaders.xml` | Space Invaders (the aliens are a grid of individually named objects; the game is won when none are left) |
| `games/frogger.xml` | Frogger: lives, one-step hops, looping lanes, riding logs, a river that kills unless you are on one |
| `games/spacerace.xml` | Space Race, two players, first to two points |
| `games/kaboom.xml` | Kaboom!: catch the falling bombs in three waves, each faster and worth more; a miss sets off the wave and costs a bucket; fall speeds and starting heights are `<random>` |
| `games/freeway.xml` | Freeway, two players: hop a chicken across ten lanes of traffic, first to five crossings wins |
| `games/depthcharge.xml` | Depth Charge: drop one charge at a time on submarines in three lanes; only misses use up your charges |
| `games/astrosmash.xml` | Astrosmash: shoot falling rocks before they land; fall speeds and starting heights are `<random>` |
| `games/lunarlander.xml` | Lunar Lander: land gently on the pad with fuel to spare; every picture is drawn from `<line>`s and collisions follow the drawn pixels |
| `games/asteroids.xml` | Asteroids: turn, thrust along the way you face and coast, shoot rocks that break into smaller rocks; the screen wraps on every side |

## What a game file looks like

```xml
<object name="ball">
  <sprite>
    <circle>
      <radius>ball.radius</radius>
      <color>color.green</color>
    </circle>
  </sprite>
  <position>
    <x>window.width.center - ball.radius</x>
    <y>window.height.center - ball.radius</y>
  </position>
  <velocity>
    <x><random min="-7" max="7" /></x>
    <y><random min="-3" max="3" /></y>
  </velocity>
  <collisions>
    <enabled>true</enabled>
    <collision edge="vertical"><bounce /></collision>
    <collision edge="left">
      <inc variable="paddle2.score" />
      <reset />
    </collision>
  </collisions>
</object>
```

The one rule of the format: an attribute names or picks something (`name`, `class`, `edge`, `button`, `state`, ...); everything else is element content. Numbers may be plain arithmetic over named values such as `window.width.center` or `ball.radius`, evaluated by exprtk.

## How it works

1. An XML library parses and validates the file (Xerces by default, with full XSD validation; TinyXML2, PugiXML or RapidXML with a built-in validator for the subset of XSD used here).
2. exprtk evaluates every value once. Objects, variables and states are built.
3. A window library draws and reads the keyboard (SFML 3 by default; Raylib, SDL2 or OpenGL).
4. Each frame: keys run the current state's bindings, objects move, collisions are swept and their rules run, conditions are checked, and the frame is drawn.

Collisions are swept, so fast small objects cannot skip over thin ones. States form a stack (menu, playing, paused, game over). Keys are bound to named actions on objects, not to movement, so remapping one key is one edit.

## Dependencies

* Xerces-C   https://github.com/apache/xerces-c
* exprtk     https://github.com/ArashPartow/exprtk
* SFML 3     https://github.com/SFML/SFML
* Optional backends: Raylib, SDL2 (with SDL2_image and SDL2_ttf), OpenGL (GLFW, the same one Raylib is built on), TinyXML2, PugiXML, RapidXML
* Qt 6 (Widgets) for XGEGUI; found, never fetched
* Catch2 for the tests

Each dependency is found through vcpkg or the system, or fetched and built when it is missing. The `FORCE_LOCAL_<NAME>` options force a fetched copy.

## Build and run

```
cmake -B build
cmake --build build
XGECLI frogger
```

A bare name gets `.xml` added; the file is looked for in the current directory, then in `games/`. The build copies the games and assets into the build directory; `XGECLI` finds them in the working directory, next to the program, or one folder above it (where Visual Studio puts the program, in `build/Debug`), so it can be started from anywhere. With no argument it runs Pong. The XML and window libraries are chosen in C++ (`cli/source/main.cpp` uses Xerces and SFML 3); there is no command-line switch yet.

The build makes three things: `XGELIB`, the engine as a library (static by default; `-DXGE_BUILD_SHARED=ON` for a shared one), `XGECLI`, the command line program above, and `XGEGUI`, the Qt application (built only when Qt 6 is found: `vcpkg install qtbase[widgets]`): the game on the left, drawn by Qt, and on the right play, pause and step controls over a tree of the game's data with editors for its values. `XGEGUI pong` runs a game (File > Options picks the video library and the XML parser, the Qt renderer and Xerces to start, and can change the video library while a game is loaded; the game waits while the dialog is open). Every video library but the Qt renderer draws in a window of its own, so picking one (or View > Game in Its Own Window) splits XGEGUI in two: the controls in one window and the game in the other. The question asked before that can be turned off, and the setting is kept in `xgegui.ini` next to the program. With no argument it opens a file dialog in `games/`. It finds `games/` and `assets/` in the working directory, next to the program, or one folder above it (where Visual Studio puts the program, in `build/Debug`), so it can be started from anywhere. `XGETEST` is the test suite and `XGEDATA` copies the games and assets next to the programs. Each project has its own folder with `source/` and `include/` in it: `lib/`, `cli/` and `gui/`.

Tests are opt-in: configure with `-DBUILD_TESTING=ON`.

## Status

The engine is still growing. It has no gravity or acceleration, no arcing jump (`hop` is a single step), no scrolling, and no sound; all movement is pixels per frame. The full list of known limits is in [docs/readme.md](docs/readme.md).

## Documentation

* [docs/readme.md](docs/readme.md): how the engine works today, verb by verb.
* [docs/designs/00-designs.md](docs/designs/00-designs.md): the design decisions, the options considered, and ideas not built yet.
* [AGENTS.md](AGENTS.md) and [docs/agents/notes.md](docs/agents/notes.md): notes for AI coding agents and new contributors.
