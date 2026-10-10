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
  --trace <dir>        play with no window and write the frames kept to <dir>
  --compare <dir>      play with no window and compare the frames kept with those in <dir>
  --frames <list>      the frames to keep (--frames 10,60), counted from 1
  --keys <list>        keys to press (+) and release (-) before a frame (--keys 5+space,40-d)
  --seed <n>           start every <random> from n (default 1 when tracing)
  --tolerance <n>      how far apart a colour may be and still be the same (0 to 255)
  --allow <n>          how many pixels may be further apart than that
  -h, --help
```

- The simplest use stays the simplest: `xgecli pong`, `xgecli pong.xml`, or nothing (runs `pong`). Every option is optional; defaults match the C++ defaults.
- A short option takes its value attached or spaced (`-gpong`, `-g pong`); a long one needs the space (`--game pong`; `--game=pong` is an error that says so, the author prefers the spaced form). Names are not case sensitive. A repeated option or game is an error (not "last wins": a quiet way to run the wrong backend); unknown option or backend errors list the valid names. A value starting with `-` is never taken as the previous option's value. Bare and `-g` are the same game, so giving both is an error.
- Start message before anything else: `file:`, `window:`, `xml:`, `audio:`, one per line.
- **Finding the game:** add `.xml` if no extension; then as given; then its file name alone in the working directory; then in `games/` of the data folder. The **data folder** is the first of the working directory, the program's folder, and the folder above it that has both `games/` and `assets/`; then the working directory is changed to it (the engine reads `assets/` relative to it), so the program starts from anywhere. Shared with xgegui through `lib/include/data_folder.h` so they cannot drift.
- `parseCommandLine` and `findGameFile` throw `CliError`; `main()` prints the message and usage to stderr. `cli.cpp` is compiled into `xgetest` too (the library does not contain it). Library code throws and never calls `exit()` because xgegui loads game after game.
- **Generating** ([generator](40-generator.md#generating-a-program-with-xslt)): `--generate` takes a platform-language target (`windows-cpp`), long form only because `-g` is the game; `-o` is an error without it. It is resolved before the working directory changes, so the default folder and a relative `-o` are where the user started; the stylesheets are found in `generators/` of the data folder. It prints the files written and the assets copied, and does not start a window. `generate.cpp` is compiled into `xgetest` too.
- **Tracing** ([generator](40-generator.md)): `--trace` and `--compare` play the game and take over the program: no start message, no `printGame`, a window of no size on the screen (`WindowFactory::createOffscreen`), no sound, a line for each frame, exit 1 if one differs. They need `--frames`; the other four are an error without one of them, and neither goes with `--generate`. The values are checked as the command line is read (`CliError`), by the same `trace::parseFrames` and `parseKeys` the run uses. Chosen: long options only, and a list as one argument (`5+space,40-d` has no `-` first, so it is a value and not an option). The generated program has to read the same words, so they are the contract between the two (`modules/trace.h` repeats them by hand). Not chosen: a key script in a file (one argument is enough for a script every game shares), PNG frames (a library to read them back; BMP is 50 lines both ways and any viewer opens it).
- Rejected: `-g` for generate (it is the game); `--game=pong`; last-one-wins; short backend aliases (`sfml`, `xerxes`). Open: every backend is built in, so every name is accepted; a build that leaves one out should reject its name with that reason.
