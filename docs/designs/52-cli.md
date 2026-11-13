# The command line

**Status:** Built.

## xgecli

```
xgecli [game] [options]
  -g, --game <game>    the game, same as giving it bare
  -w, --window <name>  sfml3 (default), raylib, sdl2, opengl, none
  -x, --xml <name>     xerces (default), tinyxml2, pugixml, rapidxml
  -a, --audio <name>   sfml3 (default), raylib, sdl2, none
  --generate <target>  write the game out as a program (windows-cpp) instead of playing it
  -o, --output <dir>   where --generate writes (default <game>-<target>)
  --record             play, and write what was played into the game file as its <demo>
  --check              load the games (none: all) and say ok or what is wrong
  --frames <n>         play n frames with no window and no sound, then stop
  --seed <n>           the random numbers start from n
  --keys <file>        play back the <demo> in the file
  --dump <file>        with --frames, write the state as XML (--dump-every <n> as well)
  -h, --help
```

- The simplest use stays the simplest: `xgecli pong`, `xgecli pong.xml`, or nothing (runs `pong`). Every option is optional; defaults match the C++ defaults.
- A short option takes its value attached or spaced (`-gpong`, `-g pong`); a long one needs the space (`--game pong`; `--game=pong` is an error that says so, the author prefers the spaced form).
  - Names are not case sensitive.
  - A repeated option or game is an error (not "last wins": a quiet way to run the wrong backend); unknown option or backend errors list the valid names.
  - A value starting with `-` is never taken as the previous option's value.
  - Bare and `-g` are the same game, so giving both is an error.
- Start message before anything else: `file:`, `window:`, `xml:`, `audio:`, one per line.
- **Finding the game:** add `.xml` if no extension; then as given; then its file name alone in the working directory; then in `descriptions/` of the data folder.
  - The **data folder** is the games folder: the first folder with both `descriptions/` and `assets/` in it, looking at the working directory, the program's folder and the folder above it, each itself and then its `games/`; then the working directory is changed to it (the engine reads `assets/` relative to it), so the program starts from anywhere.
  - Shared with xgegui through `xge/lib/engine/folder.h` so they cannot drift.
- `parseCommandLine` and `findGameFile` throw `CliError`; `main()` prints the message and usage to stderr. `cli.cpp` is compiled into `xgetest` too (the library does not contain it). Library code throws and never calls `exit()` because xgegui loads game after game.
- **Generating** ([generator](40-generator.md)): `--generate` takes a platform-language target (`windows-cpp`), long form only because `-g` is the game; `-o` is an error without it.
  - It is resolved before the working directory changes, so the default folder and a relative `-o` are where the user started; the stylesheets are found in `generators/` of the data folder, and the engine's files it copies from `lib/` of the repository's `xge/` it was built from (`GenerateRequest::engineFolder`, set from `XGE_ENGINE_FOLDER`).
  - It prints the files written and the assets copied, and does not start a window.
  - `generate.cpp` is compiled into `xgetest` too.
- **Recording** ([states-and-conditions](07-states-and-conditions.md), attract mode): `--record` plays as usual from a new seed and, when the window closes, writes the `<demo>` into the game file (`withDemo`), replacing the one there and keeping the file's line endings. Long form only, like `--generate`, with which it cannot go. It writes the repository's file on purpose: the demo is part of the game.
- - A backend that this build leaves out is refused by name, with the list of those built in.
- **Checking** (`--check`): "does this game load?" with no window, for authors, scripts and the long tests. It loads as play does (the reader, then the engine on the null window, so positions made from text sizes are worked out too), hides the libraries' loading chatter, and prints one line a game. Several games only here; none means all. `-w`/`-a` are refused with it, since it opens and plays nothing.
- **Stepping** (`--frames`, `--seed`): so an agent, which cannot watch a window, can play a game and read what happened. `--frames` plays as fast as the frames go on the null window (`playFrames` in `cli.cpp`, a loop of `Engine::step`), so it is the play the window would have shown, frame for frame. A real window or sound named with it is refused rather than ignored. `--seed` fixes the numbers before the game loads, because loading already draws them (a `<random>` position).
- **Playing keys back** (`--keys`): the file is a `<demo>`, the format `--record` already writes, so a person's recording is an agent's input and the other way round; no second key format. A game file is taken too (its `<demo>` is a child of the root), so `--record` then `--keys` on the same file needs nothing in between. `Engine::replay` starts a new game from the demo's seed, as `record()` started the one recorded, and presses each key at the start of its frame, where a window's key would have been handled. The demo's seed is the only one that replays it, so `--seed` with it is refused.
- **Dumping** (`--dump`, `Game::describe`): XML because the project's tools are XML, so a dump is read with the same libraries and checked with a schema of its own (`games/xgedump.xsd`). Numbers are element content and names are attributes, as in a game file. It holds what is in play only, the things a player could see, not every field of `Object`: a test that compares whole dumps would freeze every game, so none is kept.
- Rejected: `-g` for generate (it is the game); `--game=pong`; last-one-wins; short backend aliases (`sfml`, `xerxes`).
