# Programs

How to run a game, and how to turn one into a program of its own. Building the engine is in [engine](engine.md#building).

## xgecli

```
xgecli                       # loads "pong"
xgecli breakout              # a bare name gets ".xml" added
xgecli path/to/pong.xml      # a name with an extension is used as given
xgecli pong -w sdl2 -x tinyxml2 -a none
xgecli galaxian --record     # play, then write what was played into the file as its <demo>
xgecli pong --generate engine-cpp -o out/pong
xgecli --check               # load every game, a line each: ok or what is wrong
xgecli pong frogger --check -x tinyxml2
xgecli pacman --frames 600 --seed 1  # play 600 frames with no window, then stop
xgecli galaxian --frames 3600 --keys galaxian.xml  # play the <demo> --record wrote back
xgecli pong --frames 600 --seed 1 --dump pong-state.xml --dump-every 60
xgecli pong --frames 80 --step 0.1  # eight seconds in steps of a tenth
xgecli pong --framerate 8           # in a window, at most 8 frames a second
xgecli pacman --frames 600 --keys start.xml --picture pacman.png  # draw the last frame
```

| Option | Means |
|---|---|
| *game*, `-g`, `--game` | the game, bare or with `-g` (not both). Default `pong` |
| `-w`, `--window` | `sfml3` (default), `raylib`, `sdl2`, `opengl`, `vulkan`, `direct3d9`, `direct3d10`, `direct3d11`, `direct3d12` (Windows only), `none` (no window: the game runs at its own size, nothing drawn, no keys) |
| `-x`, `--xml` | `xerces` (default), `tinyxml2`, `pugixml`, `rapidxml` |
| `-a`, `--audio` | `sfml3` (default), `raylib`, `sdl2`, `none` |
| `--record` | play from a new seed; when the window closes, write the play into the game file as its [`<demo>`](states.md#attract-mode) |
| `--check` | load the games given (several allowed; none: every game in `descriptions/`) as for play, with a null window and no sound, and print `pong.xml: ok` or what stopped it, a line each. Exits 1 if any failed. Takes `-x`; not `-w`, `-a`, `--record` or `--generate` |
| `--frames <n>` | play n steps (each `--step` long) as fast as they go, with no window and no sound, then print `played 600 frames of pacman.xml; the state is playing` and exit (1, with the message, when the game throws). The window and sound are `none` without saying; naming another is an error. Not with `--record`, `--check` or `--generate` |
| `--step <seconds>` | with `--frames`: the length of each step, above 0 and at most a quarter second (default a sixtieth) |
| `--framerate <n>` | in a window: at most n frames a second, in place of the game's `<framerate>`. Each frame plays the time that passed, so the game's speed stays the same. Not with `--frames` |
| `--seed <n>` | the random numbers start from n, from the moment the game loads, so the same command plays the same game every time. With or without `--frames`; not with `--check` or `--generate` |
| `--keys <file>` | play back the `<demo>` in the file, the file's root or a child of it (a game file `--record` wrote into): a new game from the demo's seed, each key pressed and let go at its recorded time. With `--frames` or in a window; not with `--seed`, `--record`, `--check` or `--generate`. A recording in which the game's own attract mode started and was stopped by a key does not play back the same, since that key was not recorded |
| `--dump <file>` | with `--frames`: write the game as it is into the file as XML at the end ([below](#the-dump)) |
| `--dump-every <n>` | with `--dump`: also every n frames on the way |
| `--picture <file>` | with `--frames`: draw the last frame into the file as a PNG, the game's size times its `<scale>`, with no window (`window/picture.h` draws it from the engine's own pictures) |
| `--generate <target>` | write the game out as a program instead of playing it ([below](#generating-a-program)) |
| `-o`, `--output <dir>` | where `--generate` writes (default `<game>-<target>`); an error without `--generate` |
| `-h`, `--help` | usage, listing the libraries this build has |

**Options.** A short option takes its value attached or spaced (`-gpong`, `-g pong`); a long one needs the space (`--game pong`, never `--game=pong`). Names are not case sensitive, and each option may be given once. Only libraries this build has can be named ([backends](engine.md#backends)). `--record`, `--check`, `--generate`, `--frames`, `--step`, `--framerate`, `--seed`, `--keys`, `--dump` and `--picture` are long only; `--record`, `--check`, `--generate` and `--frames` do not go together.

**Finding the game.** In order: as given; its file name in the working directory; `descriptions/` of the **games folder**.

- The games folder is the first folder holding both `descriptions/` and `assets/`, looked for in the working directory, the program's folder and the folder above, each itself and then its `games/`.
- The program then makes the games folder its working directory, so it runs from anywhere and asset paths (`assets/paddle.jpg`) are relative to it (`engine/folder.h`).

**Output.** It prints the file, window, XML and sound library it chose, one a line. A game that will not load (schema, an expression, a name it does not have) prints a message saying where, and exits.

### The dump

One `<moment>` each time it was written, inside a `<dump>` that names `games/xgedump.xsd` by its full path, so it validates wherever it lands. A moment holds the steps played so far and how many seconds they were, the state on top and its scene, the game's variables, and each object in play:

```xml
<moment>
  <frame>600</frame>
  <seconds>10</seconds>
  <state name="playing" />
  <object name="playing.paddle1" class="paddle" group="playing.paddles">
    <position><x>30</x><y>242</y></position>
    <velocity><x>0</x><y>0</y></velocity>
    <variables>
      <variable name="score">1</variable>
    </variables>
  </object>
</moment>
```

An object has `sprite=` when it shows a named look. A run that throws still closes the file, with the moments written before it.

## xgegui

The Qt 6 application, built only when Qt is found.

- `xgegui pong` opens a game; with none, a file dialog in `games/descriptions/`.
- The game is drawn beside a tree of its data, which can be edited while it runs. Play, Pause, Step and Reset drive it; Options picks the video, XML and sound libraries. A key pressed while paused is dropped: Play or Step does not play it.
- A game that will not load is shown as a message, and the program carries on.
- The game is drawn as large as fits, keeping its proportions. A bar under it shows the defined size, its scale and the percent shown, with smaller, 100% and bigger buttons.
- One window or two: the game can have its own window, opened by the chosen library.

Design: [gui](../designs/53-gui.md).

In C++ the libraries are chosen with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`.

## Generating a program

```
xgecli pong --generate engine-cpp                 # writes pong-engine-cpp/
xgecli pong --generate engine-cpp -o out/pong     # or into the folder named
```

On Windows, `scripts\shell\windows\xgegen.bat <generator> [game]` writes every game in `games/descriptions/` with one generator, or just the game named, each to `output\<config>\generated\<generator>\<game>` with the newest build's `xgecli` (Debug, else Release). Run bare, it prints its usage and the generators. A game that will not generate shows its error; for every game, a line of it each and the list of those not written at the end.

This writes a C++ program on SFML 3 that is **the engine with the game built in**, so it plays exactly as `xgecli` does. The engine first loads the game as for play (a game that will not load is not generated), then the XSLT stylesheets in `games/generators/engine-cpp/` (run by libxslt) write:

| Written | By | What |
|---|---|---|
| `source/<game>.cpp` | `tree.xsl` | the game file as a tree of C++ data (`xml/tree.h`), an entry per element, which one function (`pongGame()`) reads with the engine's own reader (`load/xml.cpp`) |
| `source/main.cpp`, `CMakeLists.txt`, `README.md`, `.gitignore` | `program.xsl` | makes the `Game`, an `SFMLWindow` and an `SFMLAudio` and runs `Engine::loop`; the build finds or fetches SFML 3 and lunasvg |
| `engine/` | listed in `engine.xml` | the engine's files, copied unchanged from `xge/lib/` of the repository `xgecli` was built from, less the XML libraries and the other libraries |
| `assets/`, `LICENSE` | `xgecli` | the pictures and font it uses (with a font's licence), the repository's licence |

`xgecli pong --generate web-asm` writes the same game as **one web page**: engine-cpp's tree of the game and the engine's files, with a window that is the page's canvas and sound that is its Web Audio, built with Emscripten into `output/pong.html`. The page holds everything (the WebAssembly, the game, its font and pictures) and plays when opened straight from the disk:

| Written | By | What |
|---|---|---|
| `source/<game>.cpp` | engine-cpp's `tree.xsl` | the game as a tree, as above |
| `source/main.cpp`, `source/page.html`, `CMakeLists.txt`, `README.md`, `.gitignore` | `web-asm/program.xsl` | makes the `Game`, a `WebWindow` and a `WebAudio` and plays a frame each time the browser draws (at most `<framerate>` a second); the page it is put in; the build, for `emcmake`, which fetches lunasvg |
| `engine/` | engine-cpp's `engine.xml`, less SFML, and `web-asm/engine.xml` | the engine's files, `window/web` and `audio/web` among them |
| `assets/`, `LICENSE` | `xgecli` | as above; the assets are built into the page |

```
xgecli pong --generate web-asm -o pong-web-asm
cd pong-web-asm
cmake -B build -G Ninja      # in a prompt set up by emsdk_env: EMSDK names the toolchain
cmake --build build          # output/pong.html
```

In the page the canvas fills the browser's window, the game's proportions kept. Keys, the mouse and pads work as in a window. Sound starts at the first key press or click, as browsers require. What the game keeps is saved in the browser's storage for the page.

`xgecli pong --generate windows-cpp` writes the game instead **as a person would write it**: a small program of its own on SFML 3 that uses none of the engine and never names it. Its numbers are constants, each object a `Thing` (where, how fast, how big), or a struct on it when it counts something (`struct Ball : Thing { int hits{ 0 }; }`), and its rules plain code in one `frame(elapsed)` (keys held, moves by the time passed, edges, meetings, the game's own tests) with a few small functions where they are used (`touching`, `deflect`, `computerHolds`). A grid of rows and columns whose cells meet something is a list of them (`std::vector<Brick> wall`), built a kind at a time and erased from as cells die. A picture is its rows of text (`rocketsPicture`), drawn a lit pixel at a time and, where the file says so, touched by its lit pixels only (`touchingPixels`). Its states are a stack of `Screen`s, each drawn by its own function; its sounds tones made in `main.cpp` (a square wave, or another wave or noise sliding in pitch, several notes one after another, notes named as hertz); what it keeps a small text file beside it (`pong-kept.txt`). The stylesheets are in `games/generators/windows-cpp/`:

| Written | By | What |
|---|---|---|
| `source/<game>.h`, `source/<game>.cpp` | `game.xsl`, with `kinds.xsl`, `names.xsl`, `values.xsl`, `grids.xsl` and a file per verb in `verbs/` | the game's numbers, class (`pong::Pong`) and rules |
| `source/main.cpp`, `CMakeLists.txt`, `README.md`, `.gitignore` | `program.xsl` | the window: keys, pads and the mouse in, a step of the time since the last frame (at most a quarter of a second), the picture and the sounds out; the build finds or fetches SFML 3 |
| (nothing: the game, reshaped) | `prepare.xsl`, run first | groups in groups made plain, each kind taking what it leaves out from the groups around it, so a river of logs and turtles is one list of logs |
| `assets/tuffy.ttf` | `xgecli` | the font, when the game has words |
| `LICENSE` | `xgecli` | the repository's licence |

`kinds.xsl` sorts the game's parts once for both targets: the things (what moves, meets or counts), the grids (rows and columns whose cells meet), the words and still pictures only drawn, the records (live variables, or more than one look), each variable a constant, a start value or a member, and what is kept.

It writes `pong_min.xml`, `pong.xml`, `breakout.xml`, `spacerace.xml`, `kaboom.xml`, `freeway.xml` and `frogger.xml` for now; `ctest --test-dir build/generated` writes and builds each under `output/<config>/generated/windows-cpp/<game>/`. Before writing, it walks the game; for any other game it writes nothing and lists every part it has no words for yet, a line each with the object it is in:

```
windows-cpp cannot write these parts of prismfall.xml yet; use engine-cpp for this game:
  <despawn> in <collision> (object 'beam')
  <fire> in <action> (object 'lens')
  <lockstep> in <collisions> (group 'shards')
  ...
```

`xgecli pong --generate web-js` writes the same game **as a person would write it for the browser**: one page, `<game>.html`, its rules in plain JavaScript on a canvas, readable by opening it in an editor. It is `windows-cpp` in another language, in the same shape (constants, a plain object per thing, an array per grid, start functions, screens as strings, `key()`, `mouse()`, one `frame(elapsed)`, a `draw` function per screen), in a module script so its names never meet the browser's (`top`), with the browser's own parts for the rest: Web Audio for the tones, `localStorage` for what is kept, the browser's sans-serif for words. It covers the same games, walked by `windows-cpp`'s `covers.xsl`; a game it cannot write it lists in the same way, with "use web-asm for this game". The stylesheets are in `games/generators/web-js/`:

| Written | By | What |
|---|---|---|
| `<game>.html` | `game.xsl`, with `windows-cpp`'s `kinds.xsl`, its own `names.xsl`, `values.xsl`, `grids.xsl` and a file per verb in `verbs/` | the game's numbers, things and rules, then the page: the canvas filling the window with the game's proportions, keys in (a game's key is kept from the page), a frame each time the browser draws with the time since the last (at most a quarter of a second) |
| `README.md` | `generate.xsl` | what the page is, and to open it |
| `LICENSE` | `xgecli` | the repository's licence |

`xgetest-generated` also generates each game `windows-cpp` covers and runs its checks (`xge/tests/written/<game>.h`) on it: what the game does, not every frame.

`xgetest-generated` (`cmake --build build --target xgetest-generated`, never in an ordinary build) generates every game, compares its compiled data with what `readGame` reads from the file field by field, and plays the two side by side frame by frame. Without libxslt, `xgecli` is built without `--generate` and says so. Design: [generator](../designs/40-generator.md).
