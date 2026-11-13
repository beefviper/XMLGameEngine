# XMLGameEngine

XMLGameEngine is a video game description language written in XML, plus a C++ engine that runs it. A whole game (window, sounds, objects, screens, rules) is one `.xml` file, checked against a schema, [games/xgedef.xsd](games/xgedef.xsd).

The file holds no code: no loops, no `if`, no function calls. What things do comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<die />`, `<hop direction="up">`), and the engine knows how to carry out each one.

The aim is to describe the non-scrolling 2D games of the late 1970s and early 1980s, and the board and card games beside them, adding a word only when a real game cannot be said without it.

## A whole game

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="Bounce"><width>640</width><height>480</height></window>
  <variables>
    <variable name="size">8</variable>
  </variables>
  <states>
    <state name="playing">
      <objects>
        <object name="ball">
          <sprite><circle><radius>size</radius></circle></sprite>
          <position><x><centered /></x><y><centered /></y></position>
          <velocity><x>3</x><y>2</y></velocity>
          <collisions>
            <collision edge="vertical"><bounce /></collision>
            <collision edge="horizontal"><bounce /></collision>
          </collisions>
        </object>
      </objects>
      <inputs>
        <input button="space"><reset /></input>
      </inputs>
    </state>
  </states>
</game>
```

A ball bounces round the window, and Space puts it back. A collision rule reads like a sentence: this ball, when it touches an edge, bounces.

Pong is about 80 lines ([pong_min.xml](games/descriptions/pong_min.xml)). The [documentation](docs/readme.md) has a short tour of the language and a page for every part of it.

## The games

58 games come with it, in [games/descriptions/](games/descriptions/), each with a test that plays it frame by frame:

- **Arcade:** Pong, Breakout, Space Invaders, Asteroids, Pac-Man, Frogger, Donkey Kong, Q\*bert, Centipede and more.
- **Puzzles:** Tetris, Columns, Snake, and pushing, merging and mine-finding games.
- **Tables and tracks:** mini golf, artillery, pinball, a racer.
- **Boards and cards:** Checkers, Reversi, Klondike, Hearts, Texas Hold'em and others, with computer players.

[The full list](docs/reference/games.md) has every game and its keys. Space starts a game, and player one is W A S D or the arrows.

## Build and run

```
cmake -B build
cmake --build build --parallel
output/Debug/xgecli frogger
```

| Program | What |
|---|---|
| `xgecli` | plays a game from the command line; `--generate windows-cpp` writes one out as a C++ program of its own |
| `xgegui` | a Qt 6 app that shows a game beside a tree of its data (built only when Qt 6 is found) |
| `xgetest` | the tests: `cmake -B build -DBUILD_TESTING=ON`, then `output/Debug/xgetest` |

Everything needed to run lands in `output/<Config>/`, so the folder can be copied anywhere. The options, the other libraries the engine can sit on, and the build in full are in [programs](docs/reference/programs.md) and [the engine](docs/reference/engine.md#building).

## Status

Version 1 is non-scrolling games; a camera for scrolling ones is planned for version 2. What it cannot do yet is in [known limitations](docs/reference/limits.md).

## Credits and licence

XMLGameEngine is by beefviper, under the MIT licence ([LICENSE](LICENSE)); a generated program carries a copy.

It is built on these, each found through vcpkg or the system, or fetched and built:

| Library | For |
|---|---|
| [Xerces-C](https://github.com/apache/xerces-c) | parsing and full XSD validation |
| [SFML 3](https://github.com/SFML/SFML) | window, keys and sound |
| [lunasvg](https://github.com/sammycage/lunasvg) | drawing SVG sprites, and reading picture sheets |
| Qt 6, libxslt, [Catch2](https://github.com/catchorg/Catch2) | `xgegui`, `--generate`, the tests |

raylib, SDL2, OpenGL, TinyXML2, PugiXML and RapidXML can be built in as well.

The fonts in `games/assets/` keep their own licences: Tuffy is public domain, and Press Start 2P is under the SIL Open Font License (`pressstart2p-ofl.txt` beside it).

## Documentation

- [docs/readme.md](docs/readme.md): where everything is, a tour, and an index of every word.
- [AGENTS.md](AGENTS.md): for contributors and AI coding agents.
