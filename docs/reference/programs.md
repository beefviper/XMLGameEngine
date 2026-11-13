# Programs

How to run a game, and how to turn one into a program of its own. Building the engine is in [engine](engine.md#building).

## xgecli

```
xgecli                       # loads "pong"
xgecli breakout              # a bare name gets ".xml" added
xgecli path/to/pong.xml      # a name with an extension is used as given
xgecli pong -w sdl2 -x tinyxml2 -a none
xgecli galaxian --record     # play, then write what was played into the file as its <demo>
xgecli pong --generate windows-cpp -o out/pong
xgecli --check               # load every game, a line each: ok or what is wrong
xgecli pong frogger --check -x tinyxml2
```

| Option | Means |
|---|---|
| *game*, `-g`, `--game` | the game, bare or with `-g` (not both). Default `pong` |
| `-w`, `--window` | `sfml3` (default), `raylib`, `sdl2`, `opengl`, `none` (no window: the game runs at its own size, nothing drawn, no keys; text measured in the built-in font) |
| `-x`, `--xml` | `xerces` (default), `tinyxml2`, `pugixml`, `rapidxml` |
| `-a`, `--audio` | `sfml3` (default), `raylib`, `sdl2`, `none` |
| `--record` | play from a new seed; when the window closes, write the play into the game file as its [`<demo>`](states.md#attract-mode) |
| `--check` | load the games given (several allowed; none: every game in `descriptions/`) as for play, with a null window and no sound, and print `pong.xml: ok` or what stopped it, a line each. Exits 1 if any failed. Takes `-x`; not `-w`, `-a`, `--record` or `--generate` |
| `--generate <target>` | write the game out as a program instead of playing it ([below](#generating-a-program)) |
| `-o`, `--output <dir>` | where `--generate` writes (default `<game>-<target>`); an error without `--generate` |
| `-h`, `--help` | usage, listing the libraries this build has |

**Options.** A short option takes its value attached or spaced (`-gpong`, `-g pong`); a long one needs the space (`--game pong`, never `--game=pong`). Names are not case sensitive, and each option may be given once. Only libraries this build has can be named ([backends](engine.md#backends)). `--record`, `--check` and `--generate` are long only and do not go together.

**Finding the game.** In order: as given; its file name in the working directory; `descriptions/` of the **games folder**.

- The games folder is the first folder holding both `descriptions/` and `assets/`, looked for in the working directory, the program's folder and the folder above, each itself and then its `games/`.
- The program then makes the games folder its working directory, so it runs from anywhere and asset paths (`assets/paddle.jpg`) are relative to it (`engine/folder.h`).

**Output.** It prints the file, window, XML and sound library it chose, one a line. A game that will not load (schema, an expression, a name it does not have) prints a message saying where, and exits.

## xgegui

The Qt 6 application, built only when Qt is found.

- `xgegui pong` opens a game; with none, a file dialog in `games/descriptions/`.
- The game is drawn beside a tree of its data, which can be edited while it runs. Play, Pause, Step and Reset drive it; Options picks the video, XML and sound libraries.
- A game that will not load is shown as a message, and the program carries on.
- The game is drawn as large as fits, keeping its proportions. A bar under it shows the defined size, its scale and the percent shown, with smaller, 100% and bigger buttons.
- One window or two: the game can have its own window, opened by the chosen library.

Design: [gui](../designs/53-gui.md).

In C++ the libraries are chosen with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`.

## Generating a program

```
xgecli pong --generate windows-cpp                 # writes pong-windows-cpp/
xgecli pong --generate windows-cpp -o out/pong     # or into the folder named
```

This writes a C++ program on SFML 3 that is **the engine with the game built in**, so it plays exactly as `xgecli` does. The engine first loads the game as for play (a game that will not load is not generated), then the XSLT stylesheets in `games/generators/windows-cpp/` (run by libxslt) write:

| Written | By | What |
|---|---|---|
| `source/<game>.cpp` | `merge.xsl`, then `data.xsl` | the game as the engine's own `GameData` (`game/data.h`), a line of C++ per tag, with the [pieces and every `from=`](objects.md#pieces-and-from) worked out first. One function returns it (`pongMinGame()`) |
| `source/game_parts.h` | copied from `modules/` | the small functions those lines call |
| `source/main.cpp`, `CMakeLists.txt`, `README.md`, `.gitignore` | `program.xsl` | makes the `Game`, an `SFMLWindow` and an `SFMLAudio` and runs `Engine::loop`; the build finds or fetches SFML 3 and lunasvg |
| `engine/` | listed in `engine.xml` | the engine's files, copied unchanged from `xge/lib/` of the repository `xgecli` was built from, less XML reading and the other libraries |
| `assets/`, `LICENSE` | `xgecli` | the pictures and font it uses (with a font's licence), the repository's licence |

`xgetest-generated` (`cmake --build build --target xgetest-generated`, never in an ordinary build) generates every game, compares its compiled data with what `readGame` reads from the file field by field, and plays the two side by side frame by frame. Without libxslt, `xgecli` is built without `--generate` and says so. Design: [generator](../designs/40-generator.md).
