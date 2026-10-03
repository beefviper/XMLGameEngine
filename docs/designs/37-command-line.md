# 37. Command line

**Status:** built (`cli/source/cli.cpp`, `cli/include/cli.h`, `cli/source/main.cpp`, `tests/test_cli.cpp`)

## Decision

`XGECLI` takes a game and the three backends:

```
XGECLI [game] [options]
  -g, --game <game>    the game, same as giving it bare
  -w, --window <name>  sfml3 (default), raylib, sdl2, opengl
  -x, --xml <name>     xerces (default), tinyxml2, pugixml, rapidxml
  -a, --audio <name>   sfml3 (default), raylib, sdl2, none
  -h, --help           the usage
```

The simplest use stays the simplest: `XGECLI pong` and `XGECLI pong.xml` work as before, and no argument at all runs `pong`. `-g` and `--game` find the file exactly as the bare argument does (below), and fail with no value; so do `-w`, `-x` and `-a`. Every option is optional, and the defaults are the ones `Game` and `Engine` already had (Xerces and SFML3, and SFML3 for sound). `-a` was added with sound ([43](43-sound.md)); `none` plays nothing.

A short option takes its value attached or after a space, `-gpong -wsfml3 -xtinyxml2` or `-g pong -w sfml3 -x tinyxml2`. A long option needs the space, `--game pong`; `--game=pong` is an error that says so, because it is not accepted. Backend names are not case sensitive. An option, or the game, given twice is an error rather than the last one winning, and so is an unknown option or backend (the message lists the valid names). Bare and `-g` are the same game, so `pong -g breakout` is two games and an error. A value that starts with `-` is never taken as the value of the option before it, so `-g -w sdl2` says `-g` has no game.

## Start message

Before anything else is printed, `main()` says what was chosen, one to a line, with the backends by the names the options take:

```
file: games/pong.xml
window: sfml3
xml: xerces
audio: sfml3
```

## Finding the game file

A name with no extension gets `.xml`. Then, in order:

1. exactly as given (a path, or a name in the working directory);
2. its file name alone in the working directory (only different from 1 when a directory was given);
3. its file name alone in the `games/` of the data folder.

If none has it the error names the file that was not found.

The data folder is where `games/` and `assets/` are: the first of the working directory, the folder the program is in, and the folder above that (a Visual Studio build puts the program in `build/Debug` and the copies of `games/` and `assets/` in `build/`) that has both. With none, `games/` beneath the working directory is used. A game given by a relative path is found first, relative to where the program was started; then the working directory is changed to the data folder, because the engine reads `assets/` relative to it. So `XGECLI` can be started from anywhere. The lookup is in `XGELIB` (`lib/include/data_folder.h`) and is shared with `XGEGUI` ([38](38-qt-front-end.md)), so the two stay in step; `findGameFile` takes the games directory it is given and has no idea where it came from.

## Layout

`parseCommandLine` (arguments to `CliOptions`) and `findGameFile` (name to path) are separate functions that throw `CliError`, so each can be tested; neither exits the program, `main()` prints the message and the usage to stderr and returns failure. The window and XML enums were already in the library (`WindowBackend`, `XmlBackend`) and `Game` and `Engine` already took them, so the library did not change: `main()` passes `options.xml` to `Game` and `options.window` and `options.audio` to `Engine`. `cli.cpp` is compiled into `XGETEST` as well, for `test_cli.cpp`, since the library does not contain it.

## Options considered

- **`--game=pong` as well as `--game pong`.** Common elsewhere; left out on purpose, the author prefers the spaced form for long options.
- **Last one wins for a repeated option.** Common in shells and aliases, but a quiet way to run the wrong backend; an error was chosen.
- **Short names for backends (`sfml`, `xerxes`).** Not added; the four XML libraries and four window libraries are named as their libraries are.
- **Compile-time availability.** Every backend is built in today, so every name is accepted; if a build ever leaves one out, its name should be rejected here with that reason.

## Approximations

Only the parsing, the file lookup and the data folder search (`tests/test_data_folder.cpp`) are tested, with the arguments as strings and temporary directories. Running `XGECLI` with each backend was not done from here (the sandbox has no SFML, raylib or SDL2 libraries and the full project was not built), and `-h` was added beyond what was asked, since an error needs a usage to show.
