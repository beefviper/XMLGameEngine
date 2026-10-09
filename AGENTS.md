# AGENTS.md

XMLGameEngine is a video game description language in XML plus a C++ engine that runs it. A game is one `.xml` file (`games/`), checked against `xgedef.xsd`; behavior comes from a closed vocabulary of verb tags, never from code in the file.

## Read in this order

1. [docs/readme.md](docs/readme.md): how the engine works **today** (format, every verb, backends, source map, limitations). The authority on behavior.
2. [docs/designs/00-designs.md](docs/designs/00-designs.md): index of one small file per topic: why, options rejected, ideas, backlog. Open only the one for your area.
3. [docs/agents/notes.md](docs/agents/notes.md): gotchas.

The root [readme.md](readme.md) is the short public overview.

## Layout

| Path | What |
|---|---|
| `lib/` | `xgelib`, the engine (`source/`, `include/`; flat). Throws, never `exit()`s |
| `cli/`, `gui/` | `xgecli`; `xgegui` (Qt 6, built only when Qt is found) |
| `tests/` | Catch2 (`xgetest`), one `test_<topic>.cpp` per area; games are played frame by frame |
| `games/`, `assets/` | the games; their media |
| `xgedef.xsd` | the schema, the definition of the language |
| `scripts/cmake/`, `scripts/shell/` | CMake modules; prerequisite installers |

Flow: `game_xml` (parse, validate) → `game_expr` (evaluate with exprtk) → `Object`/`State` → `Game` (frame update) + `Engine` (loop, windows, sound). Backends sit behind `XmlDocument`, `Window` and `Audio` factories.

## Build and test

```
cmake -B build -DBUILD_TESTING=ON
cmake --build build
output/Debug/xgetest            # Catch2 filters work, e.g. "[kaboom]"
output/Debug/xgecli frogger     # -w window, -x xml, -a audio libraries
```

A plain build has one backend of each kind (SFML 3, Xerces); `-DXGE_WITH_<NAME>=ON` adds one, `-DXGE_ALL_BACKENDS=ON` all, and `BUILD_TESTING` implies all. Backend code is compiled behind `XGE_WITH_<NAME>`, so build with no options as well as with tests when you touch the factories, `cli.cpp` or `session_options.cpp`. `FORCE_LOCAL_<NAME>` forces a fetched dependency. Outputs land in `output/<Config>`.

## Conventions

- Every C++ source or header starts with this block (real filename and date):

  ```
  // main.cpp
  // XML Game Engine
  // author: beefviper
  // date: Sept 18, 2020
  ```

- Files end with exactly one trailing newline; stored LF (`.gitattributes` checks out CRLF on Windows).
- **Docs move with code:** update `docs/readme.md` (and root `readme.md` when a game, verb or dependency changes) and the matching `docs/designs/` file in the same change. The code is the truth. Design files hold: what we looked for, looked at, chose and why, what is open; no diaries or test logs.
- A new game goes in `data_xml` in `scripts/cmake/assets.cmake`; a test file in `scripts/cmake/tests.cmake`; an engine source in `ENGINE_SOURCES` in the top `CMakeLists.txt`.
- **Warnings stay at zero** (`xge_warnings()` in `scripts/cmake/platform.cmake`; clean on GCC and Clang). Fix, do not silence. A constructor parameter named like its member counts (`-Wshadow`, C4458).
- Games share one key convention: Space starts, pauses and restarts (never Enter); player one is W A S D plus the arrows; where Space fires, pause is P or Escape. Colors are `color.` names only.
- The format's one rule: an attribute names or picks something; everything else is element content. A new verb is added only when a game cannot be said without it, and names a behavior.
- Be direct; the author prefers clear pushback to hedging. This repository is public: nothing personal from chat exports goes in it (privacy rule in the notes).
