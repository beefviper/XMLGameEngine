# XMLGameEngine

XMLGameEngine is a VGDL (video game description language) written in XML, plus a C++ engine that loads a game description and runs it. A whole game (window, variables, sounds, objects, screens) lives in one `.xml` file, checked against an XSD. The file is declarative: no loops, no `if`, no function calls. Behavior comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<stick />`, `<die />`, `<hop direction="up">`, ...) that the engine knows how to carry out.

The target is to describe the 2D non-scrolling games of the late 1970s and early 1980s with a small, well-chosen vocabulary, and to grow that vocabulary only when a real game cannot be described without a new word.

## Games that run today

| File | Game |
|---|---|
| `games/descriptions/pong.xml` | Pong, with a menu, pause and game-over screens, 8-bit sounds for the walls, the paddles, a point, the start and the end, and a ball that comes off a paddle at an angle set by where it hit |
| `games/descriptions/pong_min.xml` | Pong with nothing but the game: two paddles and a ball, no menu, score, text or sound; a point puts the ball back in the middle and serves it again, to either side within 30 degrees of level. The smallest game, the one `--generate windows-cpp` was grown on |
| `games/descriptions/breakout.xml` | Breakout (a grey top row of bricks that takes two hits, cracking at the first, then six rows, one color a row, as one group of 9 columns and 6 rows, each row changing the color; the game is won when no brick is left and lost when the ball falls) |
| `games/descriptions/spaceinvaders.xml` | Space Invaders (three kinds of alien drawn from ASCII bitmaps, two animation frames a second apart, as one group of 11 columns and 5 rows of individually named objects, the rows changing the picture; the game is won when none are left; the aliens drop bombs back, three hits and the game is lost) |
| `games/descriptions/spaceinvaders2.xml` | Space Invaders again, drawn from an SVG sprite sheet (`assets/Space Invaders Color Sprites.svg`): the same game and group of columns and rows, with three-frame animations for the three kinds of alien, a ship and a bolt cut out of the sheet by `<svg>` sprites, and the aliens firing the sheet's plasma bolts back |
| `games/descriptions/frogger.xml` | Frogger: lives, one-step hops, looping lanes, riding logs, a river that kills unless you are on one |
| `games/descriptions/spacerace.xml` | Space Race, two players, first to two points |
| `games/descriptions/kaboom.xml` | Kaboom!: the Mad Bomber paces the rooftop, changing direction when he likes, and drops bombs as he goes; catch them in three waves, each faster and worth more; a miss sets off every falling bomb and costs a bucket. The bucket follows the mouse, or a gamepad's stick, as the Atari's paddle |
| `games/descriptions/freeway.xml` | Freeway, two players: hop a chicken across ten lanes of traffic, first to five crossings wins |
| `games/descriptions/depthcharge.xml` | Depth Charge: drop one charge at a time on submarines in three lanes; only misses use up your charges |
| `games/descriptions/astrosmash.xml` | Astrosmash: shoot falling rocks before they land; fall speeds and starting heights are `<random>` |
| `games/descriptions/lunarlander.xml` | Lunar Lander: land gently on the pad with fuel to spare; every picture is drawn from `<line>`s and collisions follow the drawn pixels |
| `games/descriptions/asteroids.xml` | Asteroids: turn, thrust along the way you face and coast, shoot rocks that break into smaller rocks; the screen wraps on every side |
| `games/descriptions/berserk.xml` | Berserk, after the arcade game Berzerk: four mazes of electrified walls with robots that patrol and shoot at you, a man who shoots the way he faces, Evil Otto coming for you if you linger, exits in every outer wall (see [games-classic](docs/designs/54-games-classic.md); first written by another AI from the schema alone, see [games-classic](docs/designs/54-games-classic.md)) |
| `games/descriptions/demonattack.xml` | Demon Attack: slide along the bottom and shoot the demons, which flap about the sky and bomb you; three waves, the second splitting its demons in two, the third taking two hits (rewritten from the first, which another AI wrote) |
| `games/descriptions/frostbite.xml` | Frostbite: jump Bailey between the snowy shore and four rows of drifting ice; every white row he lands on turns blue and adds a block to his igloo; the water, the cold and the snow geese are against him; finish the igloo and walk in |
| `games/descriptions/donkeykong.xml` | Donkey Kong: climb six girders past the barrels Kong throws, jumping them or dodging up the ladders, to save Pauline; each rescue Kong throws faster, three rescues win |
| `games/descriptions/galaxian.xml` | Galaxian: a fleet flies in on curving paths and settles into formation, then one alien at a time dives at your ship, weaving and dropping bombs, and loops back to its place from the top of the screen; the best score and a table of the best five, with initials, last from one game, and one run, to the next; two players can take turns, and left on the title it plays itself |
| `games/descriptions/pitfall.xml` | Pitfall!: run Harry through four jungle screens, the scenes of one state built from a kit of pieces, past rolling logs, a tar pit, a crocodile pond, a fire and the scorpions in the tunnel, after a treasure on each, in three minutes with three lives |
| `games/descriptions/missilecommand.xml` | Missile Command: missiles chase the nearest of six cities; steer a sight, and the base aims a counter-missile that bursts there and takes every missile flying into it; three waves |
| `games/descriptions/combat.xml` | Combat, two players: tanks that drive along the way they point and turn on the spot, among walls, one shell in the air each; first to five hits |
| `games/descriptions/airseabattle.xml` | Air-Sea Battle, two players: swing a gun and fire at four lanes of planes going round the sky; most hits in two minutes wins, and the end screen compares the scores to say who |
| `games/descriptions/megamania.xml` | Megamania: three waves of hamburgers, cookies and bugs wrap round the sides and drop bombs while your energy runs down; shoot each wave before it is gone, three ships |
| `games/descriptions/atlantis.xml` | Atlantis: Gorgon ships cross the sky over the city, a lane lower each pass, and ray it from the lowest; three gun posts fire at their own angles; three waves, the later ones the first changed (`from=`, named timers and conditions) |
| `games/descriptions/pacman.xml` | Pac-Man, on a smaller maze of its own: the maze is a `<map>` whose rows draw it in wall characters; Pac-Man `<go>`es where the keys say, the four ghosts `<hunt>` him each their own way, and a power pellet turns them blue (looks and timers that follow a look); eat every dot to win; a siren plays under the game, and tunes with a bass for the start and the endings |
| `games/descriptions/trickshot.xml` | Trick Shot, pool after the Atari 2600 game: steer a cue tip into the white ball and knock the six others into the pockets in two minutes; balls `<knock />` one another by their `<mass>`, slowed by the cloth |
| `games/descriptions/mariobros.xml` | Mario Bros.: jump up and bump the floor under a shellcreeper, a `<knock>` that goes through the floor and turns it over, then kick it off; one on its feet costs a life |
| `games/descriptions/adventure.xml` | Adventure, two rooms of the Atari 2600 game, the castle and the field east of it, as scenes: walk into the key, the sword or the chalice to `<carry />` it and Space to `<drop />` it; the key opens the castle gate, the sword slays the dragon, the chalice carried into the hall wins; the dragon sleeps until the hero comes near |

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

## Saying a thing once

What many objects or states share is written once, under a name, higher up in the file, and each place below takes it and says only what is different about it. It keeps a game file short without making it look like a program.

* **Leave out what is nothing.** An object that never moves has no `<velocity>`, and one that never collides has no `<collisions>`.
* **Groups.** A `<group>` writes the shared parts of its members once: a list of members, or a block of columns and rows where a row, a column or a single cell changes what it needs to (Breakout's rows of colors, Space Invaders' rows of aliens). A group can hold groups (Frogger's lanes), and a group's name means every member in it.
* **Named sets in `<states>`.** At the top of `<states>`, before the first `<state>`, named `<keys>`, `<shows>` and `<conditions>` sets hold what several states bind, show and check. A state takes one with `set="name"` and adds its own; a set can take a set written before it.
* **States and scenes own their objects.** A `<state>` is a mode (title, playing, paused, game over) and holds the objects it uses; a pause can draw the game still underneath it (`<show state="playing" />`). A state played in several places holds them as `<scenes>`: shared pieces sit in a kit at the top, and each scene takes what it needs and changes what differs (a speed, a position, a sprite), or starts as a copy of an earlier scene (`from=`). `<enter scene="room2" />` moves on (or `<enter way="east" />`, where each scene says where its ways lead, so an exit is written once in the kit), what the hero carries goes along, and a scene left stays as it was. Inside a scene a plain name means its own (`robots`), and a path reaches anything (`playing.room2.robots`).

See [docs/readme.md](docs/readme.md#group) and [scenes](docs/readme.md#scenes) for the details.

## How it works

1. An XML library parses and validates the file (Xerces by default, with full XSD validation; TinyXML2, PugiXML or RapidXML with a built-in validator for the subset of XSD used here).
2. exprtk evaluates every value once. Objects, variables and states are built.
3. A window library draws and reads the keyboard (SFML 3 by default; Raylib, SDL2 or OpenGL), and a sound library plays the game's sounds (SFML 3 by default; Raylib, SDL2, or none). The sounds are written in the game file as notes and made into samples by the engine, like the beeps of an old 8-bit machine: `<sound name="wall" wave="square"><note pitch="A3">0.04</note></sound>`, played by `<play sound="wall" />`; one with `<loop />` plays over and over until `<silence sound="..." />` or, played from a key, until the key is let go (Asteroids' and Lunar Lander's engines).
4. Each frame: keys run the current state's bindings, objects move, collisions are swept and their rules run, conditions are checked, the sounds asked for are played, and the frame is drawn. The order of a frame and the moving live in one place (`xge/lib/play/frame.cpp`), and what each verb does in another (`xge/lib/play/verbs.cpp`); the engine only decides from the file which of them to call, so a generated game can make the same calls as plain code.

Collisions are swept, so fast small objects cannot skip over thin ones. States form a stack (menu, playing, paused, game over). Keys are bound to named actions on objects, not to movement, so remapping one key is one edit.

## Dependencies

* Xerces-C   https://github.com/apache/xerces-c
* exprtk     https://github.com/ArashPartow/exprtk
* SFML 3     https://github.com/SFML/SFML
* lunasvg    https://github.com/sammycage/lunasvg (draws SVG sprites inside the engine; it is not a backend)
* Optional backends, not built unless asked for ([Backends](#backends)): Raylib, SDL2 (with SDL2_image and SDL2_ttf), OpenGL (GLFW, the same one Raylib is built on, and SDL2_image and SDL2_ttf for its pictures and text), TinyXML2, PugiXML, RapidXML
* Qt 6 (Widgets) for xgegui; found, never fetched
* libxslt (with libxml2) for `xgecli --generate`; found, never fetched (`vcpkg install libxslt`, or `libxslt1-dev` on Linux). Without it `xgecli` builds and plays games as before and says so when asked to generate
* Catch2 for the tests

Each dependency is found through vcpkg or the system, or fetched and built when it is missing. The `FORCE_LOCAL_<NAME>` options force a fetched copy. Only the libraries of the backends that are built are looked for at all.

## Build and run

```
cmake -B build
cmake --build build
output/Debug/xgecli frogger
```

Everything needed to run a game lands in `output/` at the top of the repository (ignored by git), one folder per configuration: `output/Debug`, `output/Release`. Each holds the programs (`xgecli`, `xgegui`, `xgetest`), the `games/` folder (the game files in `games/descriptions/`, their pictures and font in `games/assets/`, and the generators) and the schema, `xgedef.xsd`, copied next to them, and a `libraries/` folder with every DLL the programs load (the `.so` files on Linux) and Qt's plug-ins, so the folder can be run or copied elsewhere as it is. On Windows the programs find `libraries/` through a manifest: each program names a private assembly called `libraries`, and `libraries/libraries.manifest`, written after each build, lists the DLLs in it. A Ninja or Makefile build with no `CMAKE_BUILD_TYPE` is a Debug build. The build directory keeps only the compiler's and linker's own files.

A bare name gets `.xml` added; the file is looked for in the current directory, then in `games/descriptions/`. `xgecli` finds the `games/` folder in the working directory, next to the program, or one folder above it, and runs from there, so it can be started from anywhere. With no argument it runs Pong. The libraries are chosen on the command line: `-w` the window library, `-x` the XML library and `-a` the sound library (`xgecli pong -w raylib -a sdl2`); Xerces and SFML 3 when they are not given.

The build makes three things: `xgelib`, the engine as a library (static by default; `-DXGE_BUILD_SHARED=ON` for a shared one), `xgecli`, the command line program above, and `xgegui`, the Qt application (built only when Qt 6 is found: `vcpkg install qtbase[widgets]`): the game on the left, drawn by Qt, and on the right play, pause and step controls over a tree of the game's data with editors for its values. `xgegui pong` runs a game (File > Options picks the video library, the XML parser and the sound library, the Qt renderer, Xerces and SFML 3 to start, and can change the video or sound library while a game is loaded; the game waits while the dialog is open). Every video library but the Qt renderer draws in a window of its own, so picking one (or View > Game in Its Own Window) splits xgegui in two: the controls in one window and the game in the other. The question asked before that can be turned off, and the setting is kept in `xgegui.ini` next to the program. With no argument it opens a file dialog in `games/descriptions/`. It finds `games/` the same way `xgecli` does, so it can be started from anywhere. `xgetest` is the test suite and `xgedata` copies the games and assets next to the programs, in `output/Debug` or `output/Release`. Each project has its own folder: `xge/lib/` (the engine, a folder for each part of it), `xge/cli/` and `xge/gui/`, with the tests in `xge/tests/` and the schema, `xge/xgedef.xsd`, beside them.

`xgecli pong --generate windows-cpp` does not play the game: it writes it out as a program of its own on SFML 3, `pong-windows-cpp/` (`-o` names another folder). XSLT stylesheets (`games/generators/windows-cpp/`) turn the game file into C++ data in `source/pong.cpp`, the data the engine reads from it, and everything that plays it (the frame, physics and collisions, keys, pictures and sounds) is the engine's own code, copied unchanged into `engine/`: only reading XML and choosing among libraries are left out, so it plays exactly as `xgecli` and `xgegui` do, which the tests check for every game, field by field and frame by frame. The font and pictures go into `assets/`; its `CMakeLists.txt` uses installed SFML 3, exprtk and lunasvg or fetches them. It builds into its own `build/`, and the built game goes to `output/`, both left out of a repository by the `.gitignore`. See [docs/readme.md](docs/readme.md#generating-a-program).

Tests are opt-in: configure with `-DBUILD_TESTING=ON` and run `output/Debug/xgetest`. The generated games have a test program of their own, `xgetest-generated`, which an ordinary build leaves out: building it (`cmake --build build --target xgetest-generated`) generates every game and compiles each one in, and it plays each beside the engine playing the file. On Windows, `scripts\shell\windows\games-run-all.bat [config]` generates every game into `output/<config>/generated/`, builds each one with MSVC at `/W4` and runs those tests (`games-generate-all.bat`, `games-build-all.bat` and `games-test-all.bat` run one step each).

### Backends

A default build has one of each: SFML 3 for the window and the sound, and Xerces for the XML. That is all a game needs, and the least to find, fetch and compile. The others are there to choose between and to compare against, and are built when asked for:

```
cmake -B build -DXGE_WITH_RAYLIB=ON -DXGE_WITH_TINYXML2=ON     # these two as well
cmake -B build -DXGE_ALL_BACKENDS=ON                           # every one
cmake -B build -DBUILD_TESTING=ON                              # every one, and the tests
```

The options are `XGE_WITH_SFML3`, `XGE_WITH_RAYLIB`, `XGE_WITH_SDL2`, `XGE_WITH_OPENGL` (windows; all but OpenGL bring their sound library, and OpenGL needs SDL2's image and font libraries, which it brings in) and `XGE_WITH_XERCES`, `XGE_WITH_TINYXML2`, `XGE_WITH_PUGIXML`, `XGE_WITH_RAPIDXML` (XML). At least one window and one XML backend must be on. The tests play the same games through every backend and compare them, so `BUILD_TESTING` turns them all on. `xgecli` and `xgegui` only offer what was built: asking for another says so (`the raylib window library is not built into this program (built with: sfml3)`), and a default build's Options dialog has one entry in each dropdown.

## Status

The engine is still growing; version 1 is non-scrolling games, and a camera for scrolling ones is planned for version 2. Jumps are a single step (`hop`), a straight, timed line (`<jump>`) and a fixed arc with no steering in the air (`<leap>`); enemies can `<chase>` and `<aim>`, straight at their target, with no lead; all movement is pixels per frame, and timers count frames. Objects can act on their own on timers (enemies fire back), face a way, change their look, and states share named sets of keys, shows and conditions. Sound is short effects and little tunes made from notes, not music or sound files. The full list of known limits is in [docs/readme.md](docs/readme.md).

## Documentation

* [docs/readme.md](docs/readme.md): how the engine works today, verb by verb.
* [docs/designs/00-designs.md](docs/designs/00-designs.md): the design decisions, the options considered, and ideas not built yet.
* [AGENTS.md](AGENTS.md) and [docs/agents/notes.md](docs/agents/notes.md): gotchas for AI coding agents and new contributors.
