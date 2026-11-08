# XMLGameEngine

XMLGameEngine is a video game description language written in XML, plus a C++ engine that runs it. A whole game (window, sounds, objects, screens, rules) is one `.xml` file, checked against a schema, [games/xgedef.xsd](games/xgedef.xsd).

The file holds no code: no loops, no `if`, no function calls. What things do comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<die />`, `<hop direction="up">`), and the engine knows how to carry out each one. The aim is to describe the non-scrolling 2D games of the late 1970s and early 1980s, and the board and card games beside them, adding a word only when a real game cannot be written without it.

## A whole game

This is all of `pong_min.xml`, two paddles and a ball:

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="Pong">
    <width>1280</width> <height>720</height>
  </window>
  <variables>
    <variable name="margin">30</variable>
    <variable name="size">16</variable>
    <variable name="width">20</variable>
    <variable name="height">120</variable>
    <variable name="step">5</variable>
  </variables>
  <states>
    <state name="playing">
      <objects>
        <object name="ball">
          <sprite><rectangle><width>size</width><height>size</height></rectangle></sprite>
          <position><x><centered /></x><y><centered /></y></position>
          <velocity>
            <x>
              <formula>
                <multiply>
                  <multiplicand><sign><value>ball.side</value></sign></multiplicand>
                  <multiplier>ball.speed</multiplier>
                  <multiplier><cosine><value>ball.angle</value></cosine></multiplier>
                </multiply>
              </formula>
            </x>
            <y>
              <formula>
                <multiply>
                  <multiplicand>ball.speed</multiplicand>
                  <multiplier><sine><value>ball.angle</value></sine></multiplier>
                </multiply>
              </formula>
            </y>
          </velocity>
          <collisions>
            <collision edge="vertical"><bounce /></collision>
            <collision edge="horizontal"><reset /></collision>
            <collision><deflect>45</deflect></collision>
          </collisions>
          <variables>
            <variable name="speed">6</variable>
            <variable name="angle"><random><min>-30</min><max>30</max></random></variable>
            <variable name="side"><random><min>-1</min><max>1</max></random></variable>
          </variables>
        </object>
        <group name="paddles">
          <sprite><rectangle><width>width</width><height>height</height></rectangle></sprite>
          <collisions>
            <collision edge="vertical"><stick /></collision>
          </collisions>
          <actions>
            <action name="up"><move direction="up">step</move></action>
            <action name="down"><move direction="down">step</move></action>
          </actions>
          <member name="paddle1"><position><x>margin</x><y><centered /></y></position></member>
          <member name="paddle2"><position><x>window.right - margin - width</x><y><centered /></y></position></member>
        </group>
      </objects>
      <inputs>
        <input button="w"><trigger object="paddle1" action="up" /></input>
        <input button="s"><trigger object="paddle1" action="down" /></input>
        <input button="up"><trigger object="paddle2" action="up" /></input>
        <input button="down"><trigger object="paddle2" action="down" /></input>
      </inputs>
    </state>
  </states>
</game>
```

A collision rule reads like a sentence: this ball, when it touches the top or bottom edge, bounces; when it leaves by a side, it starts again; when it touches anything else, it deflects by up to 45 degrees. The serve is worked out with `<formula>` tags, so no arithmetic beyond `+ - * /` hides in a string.

## The language in brief

- **Attributes name or pick; everything else is content.** `name`, `class`, `edge`, `button` are attributes; a size, a colour or a position is element content.
- **Behaviour is a list of verbs, run in order,** in a collision rule, an action, a key, a condition, a timer or a path step. The schema says which verbs each place takes.
- **A value is a number:** plain arithmetic as text (`window.right - margin`), a `<random>`, or a `<formula>` of tags.
- **Names are dotted:** `ball.radius`, `aliens.3.2`, `playing.room2.robots`; `self.` and `touch.` inside a rule.
- **Keys are bound in states, never in objects,** so changing a key is one edit.
- **Say a thing once:** groups write what members share; a kit holds pieces taken by name; states and scenes copy others with `from=` and say only what differs.
- **Mistakes stop the load** with a message that says where, and "did you mean" for a misspelt name.

```
<game>
  <window>      size, background, frame rate, scale
  <variables>   numbers the whole game uses
  <sounds>      chip notes and music                  (optional)
  <paths>       routes that objects fly               (optional)
  <styles>      named text and bitmap looks           (optional)
  <kit>         pieces written once, taken later      (optional)
  <objects>     what every state shares               (optional)
  <keeps>       objects kept between games            (optional)
  <states>      title, playing, paused, game over ...
  <demo>        a recorded game for attract mode      (optional)
</game>
```

The [documentation](docs/readme.md) has a tour, every word, and an index linking each to its page.

## Games

55 games in [games/descriptions/](games/descriptions/), each with a test that plays it frame by frame:

| Kind | Games |
|---|---|
| Paddle and ball | Pong, Breakout, Space Race, Tennis |
| Shooters | Space Invaders (two versions), Galaxian, Asteroids, Missile Command, Megamania, Yars' Revenge, Berserk, Demon Attack, Atlantis, Astrosmash, Air-Sea Battle, Combat, Depth Charge |
| Platforms and rooms | Donkey Kong, Mario Bros., Q\*bert, Pitfall!, Frostbite, Adventure, Haunted House, Lunar Lander |
| Mazes, lanes, catching | Pac-Man, Frogger, Freeway, Kaboom!, Fishing Derby |
| Grid puzzles | Tetris, Columns, Snake, Centipede, Capsules |
| Tables and tracks | Putt Park, Cannon Hill, Pinball, Speedway, Trick Shot |
| Board games | Checkers, Reversi, Four in a Row, Tic-Tac-Toe, Dots and Boxes |
| Card games | Klondike, Crazy Eights, Blackjack, Texas Hold'em, Hearts, Spades, Gin Rummy, Pyramid |

The [full list](docs/reference/games.md) says what each game needed. They share one key convention: Space starts, pauses and restarts; player one is W A S D plus the arrows; where Space fires, P or Escape pauses.

## Build and run

```
cmake -B build
cmake --build build --parallel
output/Debug/xgecli frogger
```

The build makes `xgelib` (the engine), `xgecli` (command line) and `xgegui` (a Qt 6 app showing the game beside a tree of its data, built only when Qt is found). Everything needed to run lands in `output/<Config>/`, so the folder can be copied anywhere. `-w`, `-x` and `-a` pick the window, XML and sound libraries (`xgecli pong -w raylib -a sdl2`).

| Library | For |
|---|---|
| [Xerces-C](https://github.com/apache/xerces-c) | parsing and full XSD validation |
| [SFML 3](https://github.com/SFML/SFML) | window, keys and sound |
| [lunasvg](https://github.com/sammycage/lunasvg) | drawing SVG sprites; its plutovg reads picture sheets (PNG, JPEG, BMP) |
| Qt 6 Widgets | `xgegui` (found, never fetched) |
| libxslt | `xgecli --generate` (found, never fetched) |
| Catch2 | the tests |

Each is found through vcpkg or the system, or fetched and built. A plain build has one library of each kind; `-DXGE_WITH_<NAME>=ON` adds raylib, SDL2, OpenGL, TinyXML2, PugiXML or RapidXML, and `-DXGE_ALL_BACKENDS=ON` all of them.

- **Tests:** `cmake -B build -DBUILD_TESTING=ON`, then `output/Debug/xgetest`.
- **A program of its own:** `xgecli pong --generate windows-cpp` writes the game as a standalone SFML program. XSLT turns the game file into C++ data and the engine's own code plays it, so it plays exactly as `xgecli` does.

## Status

Version 1 is non-scrolling games; a camera for scrolling ones is planned for version 2. Movement is pixels per frame and timers count frames; sound is chip music made from notes. The limits are listed in [known limitations](docs/reference/limits.md).

## Licence

MIT, in [LICENSE](LICENSE); a generated program carries a copy. The fonts in `games/assets/` keep their own: Tuffy is public domain, and Press Start 2P is under the SIL Open Font License (`pressstart2p-ofl.txt` beside it).

## Documentation

- [docs/readme.md](docs/readme.md): the guide, a tour, and an index of every word.
- [docs/reference/](docs/reference/): how the engine works today, a page per topic.
- [docs/designs/00-designs.md](docs/designs/00-designs.md): why, options rejected, ideas not built.
- [AGENTS.md](AGENTS.md) and [docs/agents/notes.md](docs/agents/notes.md): notes for contributors and AI coding agents.
