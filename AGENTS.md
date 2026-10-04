# AGENTS.md

XMLGameEngine is a video game description language in XML plus a C++ engine that runs it. A game is one `.xml` file (`games/`), checked against `assets/xmlgameengine.xsd`; behavior comes from a closed vocabulary of verb tags, never from code in the file.

## Read in this order

1. [docs/readme.md](docs/readme.md): how the engine works **today**: file format, every verb, collisions, input, backends, source map, known limitations. The authority on behavior.
2. [docs/designs/00-designs.md](docs/designs/00-designs.md): 13 design files: why things are the way they are, options rejected, ideas not built. Read the one for the area you touch.
3. [docs/agents/notes.md](docs/agents/notes.md): gotchas, what is untested, next steps.

The root [readme.md](readme.md) is the short public overview (games, build, status).

## Layout

| Path | What |
|---|---|
| `lib/` | `XGELIB`, the engine (`source/`, `include/`; flat). Throws, never `exit()`s |
| `cli/` | `XGECLI`, the command line program |
| `gui/` | `XGEGUI`, the Qt 6 application (built only when Qt is found) |
| `tests/` | Catch2 (`XGETEST`), one `test_<topic>.cpp` per area; games are played frame by frame |
| `games/`, `assets/` | the games; the schema, font, sprite sheet |
| `scripts/cmake/`, `scripts/shell/` | CMake modules; prerequisite installers |

Flow: `game_xml` (parse, validate) → `game_expr` (evaluate with exprtk) → `Object`/`State` → `Game` (frame update) + `Engine` (loop, windows, sound). Backends sit behind `XmlDocument`, `Window` and `Audio` with factories. The source map is in `docs/readme.md`.

## Build and test

```
cmake -B build -DBUILD_TESTING=ON
cmake --build build
output/Debug/XGETEST            # all tests; Catch2 filters work (e.g. "[kaboom]" or a test name)
output/Debug/XGECLI frogger     # -w window, -x xml, -a audio libraries
```

Dependencies are found (vcpkg or system) or fetched; `FORCE_LOCAL_<NAME>` forces a fetched copy. Outputs land in `output/<Config>` (programs, `games/`, `assets/`, `libraries/`). Tests link `XGELIB`.

## Conventions

- Every C++ source or header starts with this block, real filename and date:

  ```
  // main.cpp
  // XML Game Engine
  // author: beefviper
  // date: Sept 18, 2020
  ```

- Files end with exactly one trailing newline. Files are stored LF; `.gitattributes` checks out CRLF on Windows (LF-to-CRLF warnings are harmless).
- **Docs move with code:** update `docs/readme.md` (and the root `readme.md` when a game, verb or dependency changes) and the matching `docs/designs/` file in the same change. Treat the code as the truth. Keep design files to: what we looked for, what we looked at, what we chose and why, what is open. No progress diaries or test-run logs.
- A new game file goes in `data_xml` in `scripts/cmake/assets.cmake`; a new test file in `scripts/cmake/tests.cmake`; a new engine source file in `ENGINE_SOURCES` in the top-level `CMakeLists.txt`.
- Games share one key convention: Space starts, pauses and restarts (never Enter); player one is W A S D plus the arrows; where Space fires, pause is P or Escape. Colors are `color.` names only.
- The format's one rule: an attribute names or picks something; everything else is element content. A new verb is added only when a game cannot be said without it, and names a behavior.
- Be direct; the author prefers clear pushback to hedging. This repository is public: nothing personal from chat exports goes in it (see the privacy rule in the notes).
