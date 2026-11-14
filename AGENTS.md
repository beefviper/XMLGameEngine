# AGENTS.md

XMLGameEngine is a video game description language in XML plus a C++ engine that runs it. A game is one `.xml` file in `games/descriptions/`, checked against `games/xgedef.xsd`. Behavior comes from a closed vocabulary of verb tags, never from code in the file.

## Where to read

| For | Read |
|---|---|
| tips that are hard to work out from the code | [docs/agents/](docs/agents/readme.md): its first page always, then the page for the part you touch |
| what a tag or program does today | [docs/reference/](docs/reference/), a page per topic, listed in [docs/readme.md](docs/readme.md) |
| where each part of the code lives, and how a game is loaded and played | [docs/reference/engine.md](docs/reference/engine.md) |
| why it is this way | [docs/designs/00-designs.md](docs/designs/00-designs.md); open only the file for your area |
| what is next | [docs/plan.md](docs/plan.md) |

The code is the truth, then the reference. The root [readme.md](readme.md) is the short public overview.

## Build and test

```
cmake -B build -DBUILD_TESTING=ON
cmake --build build --parallel
output/Debug/xgetest            # Catch2 filters work, e.g. "[kaboom]"
cd build && ctest -j16          # each case its own run, side by side
cmake --build build --target xgetest-generated && output/Debug/xgetest-generated   # generated games
cmake --build build --target xgetest-long && output/Debug/xgetest-long             # minutes
xvfb-run -a output/Debug/xgetest "[window]"   # opens every window backend
output/Debug/xgecli frogger     # -w window, -x xml, -a audio libraries
```

`BUILD_TESTING` turns every backend on. Backend code is compiled behind `XGE_WITH_<NAME>`, so build with no options as well when you touch the factories, `cli.cpp` or `session_options.cpp`. The rest is in [Building](docs/reference/engine.md#building).

The programs make `games/` their working directory, so paths in a game file (`assets/paddle.jpg`) and in a test (`descriptions/pong.xml`) are relative to it.

## Rules

- **Docs move with code.** A change to a tag or to behaviour updates its `docs/reference/` page and the matching `docs/designs/` file in the same change, and the word index in `docs/readme.md` when a word is added or removed.
- **Warnings stay at zero** on GCC, Clang and MSVC. Fix, do not silence.
- **The format's one rule:** an attribute names or picks something; everything else is element content. No data is hidden in a string.
- **A new verb** is added only when a game cannot be said without it, and names a behavior, never a game.
- **Games share one key convention** ([the shipped games](docs/reference/games.md#the-key-convention)), and colors are `color.` names only.
- **This repository is public:** nothing personal goes in it ([privacy](docs/agents/readme.md#privacy)).
- Be direct; the author prefers clear pushback to hedging.

## Adding a file

| New | Also goes in |
|---|---|
| a game | `games/descriptions/`, `data_xml` in `scripts/cmake/assets.cmake`, a test in `xge/tests/games/`, a row in `docs/reference/games.md` |
| a test file | `scripts/cmake/tests.cmake` |
| an engine source | `ENGINE_SOURCES` in the top `CMakeLists.txt` |
| a verb | a file in `xge/lib/verbs/`, a line in its `all.cpp` and in `games/generators/engine-cpp/engine.xml` |

Every C++ source or header starts with this block (real filename and date), ends with exactly one newline, and is stored LF:

```
// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020
```

## Writing docs

- Each fact has one home; other pages link to it.
- Reference pages say what is: short sections, a small example each, no history. Design files say why: what was wanted, looked at, chosen, and what is open, near 6 KB each, with no dates, diaries or test logs.
- No walls of text. A paragraph makes one point; a list or a table is written as one.
