# XMLGameEngine

XMLGameEngine is a video game description language written in XML, plus a C++ engine that runs it. A whole game (window, sounds, objects, screens, rules) is one `.xml` file, checked against a schema, [games/xgedef.xsd](games/xgedef.xsd).

The file holds no code: no loops, no `if`, no function calls. What things do comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<die />`, `<hop direction="up">`), and the engine knows how to carry each one out. The aim is to describe the non-scrolling 2D games of the late 1970s and early 1980s with a small vocabulary, and to add a word only when a real game cannot be written without it.

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
            <x>sgn(ball.side) * ball.speed * cos(ball.angle * pi / 180)</x>
            <y>ball.speed * sin(ball.angle * pi / 180)</y>
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

A collision rule reads like a sentence: this ball, when it touches the top or bottom edge, bounces; when it leaves by a side, it starts again; when it touches anything else, it deflects by up to 45 degrees.

## The language

### Grammar

- **Attributes name or pick; everything else is content.** `name`, `class`, `object`, `edge`, `button`, `state`, `sound` are attributes. A size, a color, a position or a flag is element content: `<radius>10</radius>`.
- **Behavior is a list of verbs, run in the order written.** Lists sit in a collision rule, an object's action, a key, a condition, a timer and a path step; the schema sorts the verbs into groups and says which groups each place takes, so a verb can only be written where it does something.
- **A value is a number written one of three ways:** arithmetic as text (`window.width.center - ball.radius`, worked out by exprtk), a `<random>` with a `<min>` and a `<max>`, or the same arithmetic as tags (a nested `<formula>` of `add`, `subtract`, `multiply`, `divide`), which leaves nothing in a string for a tool to parse.
- **Names are dotted.** `ball.radius` is the ball's variable, `paddle1.x` where it is, `aliens.3.2` a cell of a group, `playing.room2.robots` an object in a scene. `window.right`, `window.width.center` and the `color.` names are built in, and `self.` and `touch.` mean the object running a rule and the touch it answers.
- **Keys are bound in states, never in objects.** An object has named actions (`up`, `fire`), and a state binds keys to them, so changing a key is one edit.
- **Mistakes stop the load** with a message that says where: an unknown verb, a misspelled key (`did you mean 'space'?`), a state, sound or look that does not exist, or a verb in a place where it does nothing.

### Structure

```
<game>
  <window>      size, background, frame rate
  <variables>   numbers the whole game uses
  <sounds>      notes, rests and parts of chip music           (optional)
  <paths>       routes that objects fly                        (optional)
  <styles>      named sizes, colors and fonts for text and bitmaps (optional)
  <kit>         pieces written once, taken or spawned later    (optional)
  <objects>     what every state shares                        (optional)
  <keeps>       objects kept from one game to the next         (optional)
  <states>      title, playing, paused, game over ...
  <demo>        a recorded game for the attract mode           (optional)
</game>
```

A **state** is a mode of the game. It owns its objects, can hold **scenes** (the rooms or screens of one state), binds its keys in `<inputs>`, checks `<conditions>` every frame, and can have `<timers>`, `<music>` and a `<computer>` player. States form a stack: `<push>` a pause over play, `<pop />` back.

### Saying a thing once

Something many objects or states share is written once under a name, higher up, and each place below takes it and says only what differs.

- **Leave out what is nothing.** An object that never moves has no `<velocity>`; one that never collides has no `<collisions>`.
- **Groups** write their members' shared parts once. Members are listed, laid out as columns and rows (where a row, a column or a cell changes what it needs), or placed by a **map**: rows of text where each character stands for a group or object (Pac-Man's walls and dots).
- **Named sets** at the top of `<states>` hold the `<keys>`, `<shows>` and `<conditions>` several states use; a state takes one with `set=` or `keys=`.
- **The kit** at the top of the game holds objects, groups and maps written once. Nothing in it is in play until an `<objects>` takes it (`<object name="paddle2" from="paddle">`, or just by the piece's name) or a `spawn` copies it; where it is taken says how long it lasts: the game, a state or a scene.
- **Copies.** `<state from="youwin">` and `<scene from="room1">` start as a copy of an earlier one and change what differs, by the same rule a take of a kit piece follows.
- **Scenes** take their pieces from the kit, and an exit names a way (`<enter way="east" />`) that each scene points somewhere different.

### Vocabulary

**Things**

| Word | What it is |
|---|---|
| `object` | anything drawn, moved or touched; `class=` makes it one of a kind |
| `group`, `member`, `columns`, `rows`, `row`, `column`, `cell` | many objects written once |
| `map`, `tile` | level data as rows of characters, each character a group or object |
| `state`, `scene`, `way` | modes of the game; places within a mode; exits between places |
| `variable` | a number, owned by the game or by an object |
| `sound`, `note`, `rest`, `part`, `loop` | chip sounds and music, from five waves (`square`, `triangle`, `sawtooth`, `sine`, `noise`) |
| `path`, `step`, `start`, `home`, `speed` | a route of straight legs, with verbs at its steps |
| `timer`, `every`, `after` | something done again and again, or once after a while; a named one's time left is a number of its object |
| `keep`, `style`, `demo` | an object kept between games (a best score); a text style; a recorded game for the attract mode |

**Looks.** A `<sprite>` is a `circle`, `rectangle`, `text`, `image`, `bitmap` (ASCII rows, one color per character), `svg`, a set of `line`s, or a `tether` (a line between two objects, redrawn each frame). Text is built from `content`, `number`, `letter` and `choose` pieces. Several named sprites are an object's **looks**, changed by `<become>`, which can also swap a bitmap's palette for another style's colors (`<become style="angry" />`), and an `<animation>` of `<frame>`s cycles them; a named animation is a look that moves. `<poses>` picks the look for standing, walking, skidding, rising and falling by itself, and `<mirror>` flips the pictures for the other way. The `<window>` can have a `<scale>`, so a game is written at the size of the original screen and drawn bigger.

**Properties** of an object: `position`, `velocity`, `acceleration` (gravity), `heading` (turns the picture), `facing` (one of four ways), `grip` (how fast the speed follows the keys, so it slides), `drag`, `mass`, `pull` and `seek` (toward the nearest of something), `altitude` and `shadow` (height off the ground), `light` and `dark` (seen only when lit), `maze` (walks a map's tiles), `active` (false: starts out of play), `drawn` (in play but never drawn), and `most` (how many copies a `spawn` may have made of it at once).

**Collisions.** `<collisions>` turns touching on (`enabled`) and says how: `type` (`box` or `pixel`), `solid` characters, a `hitbox`, and `lockstep` for a group that marches as one. Each `<collision>` rule is about a screen `edge` or another `class`/`object`, and can be narrowed by `side=`, `unless=` (not while also touching that class), `sprite=` (only in that look), `<slower>`, `<faster>`, `through=` (only a knock felt through that class, as Mario Bros.' floors pass a bump on) and `<condition>`s.

**Verbs**

| Family | Words |
|---|---|
| Moving | `move`, `hop` and `jump` (with an `arc` to go up and over), `leap`, `land`, `climb`, `accelerate`, `thrust`, `turn`, `reverse`, `stop`, `track` (follow the mouse or a stick) |
| Steering | `chase`, `aim`, `follow` a path, and in a maze `go`, `hunt`, `wander` |
| Touching | `bounce`, `deflect`, `knock`, `stick`, `wrap`, `ride`, `carry`, `drop`, `hole` |
| In and out of play | `die`, `reset`, `fire`, `release`, `reveal`, `spawn` and `despawn` (objects made and taken away while the game runs) |
| Numbers | `inc`, `dec`, `set`, `copy`, `rank` |
| Look and sound | `become`, `play`, `silence` |
| Flow | `push`, `pop`, `enter`, `trigger` (run an object's action), `turns` with `pass` and `out`, `demo`, `hold` (a computer player's key) |

**Conditions** test an object's variable, a count, or a distance, and run verbs when the test holds:

```xml
<condition class="aliens"><remaining>0</remaining><push state="youwin" /></condition>
```

The tests are `atleast`, `atmost`, `above`, `below`, `equal` (against a number, which can name another variable and is read as the game runs) and `remaining`. `total=` counts a group or takes its sum, highest or lowest, `distance=` measures to an object, and `<all>`/`<any>` of `<condition>`s join tests.

**Input.** `<input button="space">` binds a key, a mouse button (`mouse.left`) or a gamepad button (`pad1.a`); `<input axis="mouse.x">` binds a control that points.

### A frame

Keys run the current state's bindings, timers count, objects move, collisions are swept (so a fast ball cannot pass through a thin paddle) and their rules run, conditions are checked, the sounds asked for play, and the frame is drawn. The frame lives in `xge/lib/play/frame.cpp` and the verbs in `xge/lib/play/verbs.cpp`.

## Games

There are 33 games in [games/descriptions/](games/descriptions/), from Pong and Breakout to Pac-Man, Donkey Kong, Q\*bert and Adventure. Many of them brought a word with them:

| Game | Words it needed |
|---|---|
| Pong | `deflect`: the angle off a paddle depends on where the ball hits it |
| Space Invaders | groups of columns and rows in `lockstep`, `hole` for the shields |
| Frogger | `ride` a log, `unless=` (the water drowns you unless you are on a log) |
| Donkey Kong | `leap`, `land` and `climb` for girders and ladders, and a group's `slope` |
| Pac-Man | `map`, `maze`, and the ghosts' `go`, `hunt` and `wander` |
| Mario Bros. | `poses`, `mirror`, `grip` and the window's `scale` |
| Q\*bert | hops with an `arc`, `hunt` by hops, `solid` characters and a `hitbox` |
| Galaxian | `path`s and `follow`, `turns` for two players, a `demo`, a top-five with `rank` |
| Trick Shot | `knock` by `mass`, slowed by `drag` |
| Tennis | `altitude` and `shadow`, a `computer` player |
| Haunted House | `light` and `dark` |
| Adventure | scenes and ways, `carry` and `drop` |

The games share one key convention: Space starts, pauses and restarts, player one is W A S D (plus the arrows), and where Space fires, P or Escape pauses.

## Build and run

```
cmake -B build
cmake --build build --parallel
output/Debug/xgecli frogger
```

The build makes `xgelib` (the engine), `xgecli` (command line) and `xgegui` (a Qt 6 app that shows the game beside a tree of its data, built only when Qt is found). Everything needed to run lands in `output/<Config>/`: the programs, the games, the schema and the libraries, so the folder can be copied anywhere.

`xgecli frogger` finds `frogger.xml` in `games/descriptions/`; with no name it runs Pong. `-w`, `-x` and `-a` pick the window, XML and sound libraries (`xgecli pong -w raylib -a sdl2`).

**Dependencies.** Each is found through vcpkg or the system, or fetched and built when missing (`FORCE_LOCAL_<NAME>` forces a fetch).

| Library | For |
|---|---|
| [Xerces-C](https://github.com/apache/xerces-c) | parsing and full XSD validation |
| [exprtk](https://github.com/ArashPartow/exprtk) | arithmetic in values |
| [SFML 3](https://github.com/SFML/SFML) | window, keys and sound |
| [lunasvg](https://github.com/sammycage/lunasvg) | drawing SVG sprites |
| Qt 6 Widgets | `xgegui` (found, never fetched) |
| libxslt | `xgecli --generate` (found, never fetched) |
| Catch2 | the tests |

**Backends.** A plain build has one of each kind. Others are built on request, to choose between and compare:

| Kind | Default | Others |
|---|---|---|
| Window | SFML 3 | Raylib, SDL2, OpenGL |
| Sound | SFML 3 | Raylib, SDL2, none |
| XML | Xerces | TinyXML2, PugiXML, RapidXML (with a built-in validator for the schema's subset of XSD) |

`-DXGE_WITH_<NAME>=ON` adds one and `-DXGE_ALL_BACKENDS=ON` adds all of them.

**Tests.** `cmake -B build -DBUILD_TESTING=ON` builds every backend and `xgetest`, which plays the games frame by frame and compares the backends. Run `output/Debug/xgetest`.

**Generating a program.** `xgecli pong --generate windows-cpp` writes the game out as a standalone SFML program in `pong-windows-cpp/`. XSLT stylesheets in `games/generators/` turn the game file into C++ data, and the engine's own code plays it, so it plays exactly as `xgecli` does. The `xgetest-generated` target checks this for every game, and on Windows `scripts\shell\windows\games-run-all.bat` generates, builds and tests them all.

## Status

Version 1 is non-scrolling games; a camera for scrolling ones is planned for version 2. Movement is in pixels per frame and timers count frames. Paths are straight legs, maze walkers steer as the arcade's ghosts do (no search of the maze), and sound is chip music made from notes, never sound files. The full list of limits is in [docs/readme.md](docs/readme.md#known-limitations).

## Licence

MIT, in [LICENSE](LICENSE); a program `--generate` writes carries a copy. The fonts in `games/assets/` keep their own: Tuffy is public domain, and Press Start 2P is under the SIL Open Font License (`pressstart2p-ofl.txt` beside it).

## Documentation

- [docs/readme.md](docs/readme.md): how the engine works today, verb by verb.
- [docs/designs/00-designs.md](docs/designs/00-designs.md): design decisions, options rejected, and ideas not built yet.
- [AGENTS.md](AGENTS.md) and [docs/agents/notes.md](docs/agents/notes.md): notes for contributors and AI coding agents.
