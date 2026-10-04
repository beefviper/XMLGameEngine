# XMLGameEngine

XMLGameEngine is a VGDL (video game description language) written in XML, plus a C++ engine that loads a game description and runs it. A whole game (window, variables, sounds, objects, screens) lives in one `.xml` file, checked against an XSD. The file is declarative: no loops, no `if`, no function calls. Behavior comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<stick />`, `<die />`, `<hop direction="up">`, ...) that the engine knows how to carry out.

The target is to describe the 2D non-scrolling games of the late 1970s and early 1980s with a small, well-chosen vocabulary, and to grow that vocabulary only when a real game cannot be described without a new word.

## Games that run today

| File | Game |
|---|---|
| `games/pong.xml` | Pong, with a menu, pause and game-over screens, 8-bit sounds for the walls, the paddles, a point, the start and the end, and a ball that comes off a paddle at an angle set by where it hit |
| `games/breakout.xml` | Breakout (six rows of bricks, one color a row, as a group of grids; the game is won when no brick is left and lost when the ball falls) |
| `games/spaceinvaders.xml` | Space Invaders (three kinds of alien drawn from ASCII bitmaps, two animation frames a second apart, as a group of grids of individually named objects; the game is won when none are left; the aliens drop bombs back, three hits and the game is lost) |
| `games/spaceinvaders2.xml` | Space Invaders again, drawn from an SVG sprite sheet (`assets/Space Invaders Color Sprites.svg`): the same game and group of grids, with three-frame animations for the three kinds of alien, a ship and a bolt cut out of the sheet by `<svg>` sprites, and the aliens firing the sheet's plasma bolts back |
| `games/frogger.xml` | Frogger: lives, one-step hops, looping lanes, riding logs, a river that kills unless you are on one |
| `games/spacerace.xml` | Space Race, two players, first to two points |
| `games/kaboom.xml` | Kaboom!: the Mad Bomber paces the rooftop, changing direction when he likes, and drops bombs as he goes; catch them in three waves, each faster and worth more; a miss sets off every falling bomb and costs a bucket |
| `games/freeway.xml` | Freeway, two players: hop a chicken across ten lanes of traffic, first to five crossings wins |
| `games/depthcharge.xml` | Depth Charge: drop one charge at a time on submarines in three lanes; only misses use up your charges |
| `games/astrosmash.xml` | Astrosmash: shoot falling rocks before they land; fall speeds and starting heights are `<random>` |
| `games/lunarlander.xml` | Lunar Lander: land gently on the pad with fuel to spare; every picture is drawn from `<line>`s and collisions follow the drawn pixels |
| `games/asteroids.xml` | Asteroids: turn, thrust along the way you face and coast, shoot rocks that break into smaller rocks; the screen wraps on every side |
| `games/berserk.xml` | Berserk, after the arcade game Berzerk: four mazes of electrified walls with robots that patrol and shoot back, a man who shoots the way he faces, Evil Otto if you linger, exits in every outer wall (see [design 10](docs/designs/10-games-as-tests.md); first written by another AI from the schema alone, see [design 10](docs/designs/10-games-as-tests.md)) |
| `games/demonattack.xml` | Demon Attack: slide along the bottom and shoot the demons, which bounce from side to side and fire down at you (first written by another AI) |
| `games/frostbite.xml` | Frostbite: jump Bailey between the snowy shore and four rows of drifting ice; every white row he lands on turns blue and adds a block to his igloo; the water, the cold and the snow geese are against him; finish the igloo and walk in |

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

The one rule of the format: an attribute names or picks something (`name`, `class`, `edge`, `button`, `state`, ...); everything else is element content. Numbers may be plain arithmetic over named values such as `window.width.center` or `ball.radius`, evaluated by exprtk, or the same arithmetic written as `<equation>` (steps) or `<formula>` (nested) tags, which leave nothing in a string for a tool to parse.

## How it works

1. An XML library parses and validates the file (Xerces by default, with full XSD validation; TinyXML2, PugiXML or RapidXML with a built-in validator for the subset of XSD used here).
2. exprtk evaluates every value once. Objects, variables and states are built.
3. A window library draws and reads the keyboard (SFML 3 by default; Raylib, SDL2 or OpenGL), and a sound library plays the game's sounds (SFML 3 by default; Raylib, SDL2, or none). The sounds are written in the game file as notes and made into samples by the engine, like the beeps of an old 8-bit machine: `<sound name="wall" wave="square"><note pitch="A3">0.04</note></sound>`, played by `<play sound="wall" />`.
4. Each frame: keys run the current state's bindings, objects move, collisions are swept and their rules run, conditions are checked, the sounds asked for are played, and the frame is drawn.

Collisions are swept, so fast small objects cannot skip over thin ones. States form a stack (menu, playing, paused, game over). Keys are bound to named actions on objects, not to movement, so remapping one key is one edit.

## Dependencies

* Xerces-C   https://github.com/apache/xerces-c
* exprtk     https://github.com/ArashPartow/exprtk
* SFML 3     https://github.com/SFML/SFML
* lunasvg    https://github.com/sammycage/lunasvg (draws SVG sprites inside the engine; it is not a backend)
* Optional backends: Raylib, SDL2 (with SDL2_image and SDL2_ttf), OpenGL (GLFW, the same one Raylib is built on), TinyXML2, PugiXML, RapidXML
* Qt 6 (Widgets) for XGEGUI; found, never fetched
* Catch2 for the tests

Each dependency is found through vcpkg or the system, or fetched and built when it is missing. The `FORCE_LOCAL_<NAME>` options force a fetched copy.

## Build and run

```
cmake -B build
cmake --build build
output/Debug/XGECLI frogger
```

Everything needed to run a game lands in `output/` at the top of the repository (ignored by git), one folder per configuration: `output/Debug`, `output/Release`. Each holds the programs (`XGECLI`, `XGEGUI`, `XGETEST`), the `games/` and `assets/` folders copied next to them, and a `libraries/` folder with every DLL the programs load (the `.so` files on Linux) and Qt's plug-ins, so the folder can be run or copied elsewhere as it is. On Windows the programs find `libraries/` through a manifest: each program names a private assembly called `libraries`, and `libraries/libraries.manifest`, written after each build, lists the DLLs in it. A Ninja or Makefile build with no `CMAKE_BUILD_TYPE` is a Debug build. The build directory keeps only the compiler's and linker's own files.

A bare name gets `.xml` added; the file is looked for in the current directory, then in `games/`. `XGECLI` finds the games and assets in the working directory, next to the program, or one folder above it, so it can be started from anywhere. With no argument it runs Pong. The libraries are chosen on the command line: `-w` the window library, `-x` the XML library and `-a` the sound library (`XGECLI pong -w raylib -a sdl2`); Xerces and SFML 3 when they are not given.

The build makes three things: `XGELIB`, the engine as a library (static by default; `-DXGE_BUILD_SHARED=ON` for a shared one), `XGECLI`, the command line program above, and `XGEGUI`, the Qt application (built only when Qt 6 is found: `vcpkg install qtbase[widgets]`): the game on the left, drawn by Qt, and on the right play, pause and step controls over a tree of the game's data with editors for its values. `XGEGUI pong` runs a game (File > Options picks the video library, the XML parser and the sound library, the Qt renderer, Xerces and SFML 3 to start, and can change the video or sound library while a game is loaded; the game waits while the dialog is open). Every video library but the Qt renderer draws in a window of its own, so picking one (or View > Game in Its Own Window) splits XGEGUI in two: the controls in one window and the game in the other. The question asked before that can be turned off, and the setting is kept in `xgegui.ini` next to the program. With no argument it opens a file dialog in `games/`. It finds `games/` and `assets/` the same way `XGECLI` does, so it can be started from anywhere. `XGETEST` is the test suite and `XGEDATA` copies the games and assets next to the programs, in `output/Debug` or `output/Release`. Each project has its own folder with `source/` and `include/` in it: `lib/`, `cli/` and `gui/`.

Tests are opt-in: configure with `-DBUILD_TESTING=ON`.

## Status

The engine is still growing. It has no arcing jump (`hop` is a single step and `<jump>` a straight, timed one), no aiming, and no scrolling; all movement is pixels per frame, and timers count frames. Objects can act on their own on timers (enemies fire back), face a way, change their look, and states share named key sets. Sound is short effects and little tunes made from notes, not music or sound files. The full list of known limits is in [docs/readme.md](docs/readme.md).

## Documentation

* [docs/readme.md](docs/readme.md): how the engine works today, verb by verb.
* [docs/designs/00-designs.md](docs/designs/00-designs.md): the design decisions, the options considered, and ideas not built yet.
* [AGENTS.md](AGENTS.md) and [docs/agents/notes.md](docs/agents/notes.md): notes for AI coding agents and new contributors.
