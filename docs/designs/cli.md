# The command line

**Status:** Built.

## xgecli

```
xgecli [game] [options]
  -g, --game <game>    the game, same as giving it bare
  -w, --window <name>  sfml3 (default), raylib, sdl2, opengl
  -x, --xml <name>     xerces (default), tinyxml2, pugixml, rapidxml
  -a, --audio <name>   sfml3 (default), raylib, sdl2, none
  --generate <target>  write the game out as a program (windows-cpp) instead of playing it
  -o, --output <dir>   where --generate writes (default <game>-<target>)
  -h, --help
```

- The simplest use stays the simplest: `xgecli pong`, `xgecli pong.xml`, or nothing (runs `pong`). Every option is optional; defaults match the C++ defaults.
- A short option takes its value attached or spaced (`-gpong`, `-g pong`); a long one needs the space (`--game pong`; `--game=pong` is an error that says so, the author prefers the spaced form). Names are not case sensitive. A repeated option or game is an error (not "last wins": a quiet way to run the wrong backend); unknown option or backend errors list the valid names. A value starting with `-` is never taken as the previous option's value. Bare and `-g` are the same game, so giving both is an error.
- Start message before anything else: `file:`, `window:`, `xml:`, `audio:`, one per line.
- **Finding the game:** add `.xml` if no extension; then as given; then its file name alone in the working directory; then in `games/` of the data folder. The **data folder** is the first of the working directory, the program's folder, and the folder above it that has both `games/` and `assets/`; then the working directory is changed to it (the engine reads `assets/` relative to it), so the program starts from anywhere. Shared with xgegui through `lib/include/data_folder.h` so they cannot drift.
- `parseCommandLine` and `findGameFile` throw `CliError`; `main()` prints the message and usage to stderr. `cli.cpp` is compiled into `xgetest` too (the library does not contain it). Library code throws and never calls `exit()` because xgegui loads game after game.
- **Generating** ([generator](generator.md#generating-a-program-with-xslt)): `--generate` takes a platform-language target (`windows-cpp`), long form only because `-g` is the game; `-o` is an error without it. It is resolved before the working directory changes, so the default folder and a relative `-o` are where the user started; the stylesheets are found in `generators/` of the data folder. It prints the files written and the assets copied, and does not start a window. `generate.cpp` is compiled into `xgetest` too.
- Rejected: `-g` for generate (it is the game); `--game=pong`; last-one-wins; short backend aliases (`sfml`, `xerxes`). Open: every backend is built in, so every name is accepted; a build that leaves one out should reject its name with that reason.
