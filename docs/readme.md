# XMLGameEngine: how it works

This is the reference for what the engine does **today**: the game file, every element and verb, the programs, the backends and the source. When it disagrees with the code, the code is right and this file is fixed. Why things are the way they are, what was rejected and what is still an idea is in [designs/00-designs.md](designs/00-designs.md); the short public overview is the [root readme](../readme.md).

## Contents

- [The idea](#the-idea): a game file in one page, and the six words to know
- [Running games](#running-games): `xgecli`, `xgegui`, the shipped games, generating a program
- [The game file](#the-game-file): layout, the format's one rule, values, names
- [Window, variables and styles](#window-variables-and-styles)
- [Objects](#objects): parts, groups, maps, the kit, spawning
- [Pictures](#pictures): sprites, bitmaps, text, animation, looks, poses
- [Sounds](#sounds)
- [Commands](#commands): where they go and what each does
- [Collisions](#collisions): rules, filters, how a touch is found, knocks
- [Input](#input): keys and their three moments, mouse, pads, held keys
- [Timers and paths](#timers-and-paths)
- [States and scenes](#states-and-scenes): the stack, scenes, conditions, keeps, turns, demos, computer players
- [What happens when a game runs](#what-happens-when-a-game-runs)
- [Backends](#backends), [Building](#building), [Source map](#source-map)
- [Known limitations](#known-limitations)

## The idea

A game is one `.xml` file, checked against the schema [games/xgedef.xsd](../games/xgedef.xsd). The file is declarative: it has no loops, no `if` and no function calls. Behavior comes from a fixed vocabulary of verbs written as tags (`<bounce />`, `<die />`, `<hop direction="up">`), and the engine knows how to carry out each one. In this file and the design notes a verb is sometimes written `bounce()` for short; in a game file it is always the tag.

Six words carry most of the language:

| Word | What it is | Example |
|---|---|---|
| **object** | anything drawn, moved or touched: a ball, a paddle, a score | `<object name="ball" class="ball">` |
| **rule** | what an object does when it touches a screen edge or another object | `<collision edge="vertical"><bounce /></collision>` |
| **action** | a named thing an object can do, which a key or a rule runs | `<action name="up"><move direction="up">step</move></action>` |
| **state** | a mode of the game (title, playing, paused, game over): its keys, its objects, its conditions | `<input button="w"><trigger object="paddle1" action="up" /></input>` |
| **condition** | a test checked every frame, with what to do when it holds | `<condition object="ball" variable="misses"><atleast>3</atleast> ... </condition>` |
| **value** | a number: plain arithmetic text, a `<random>`, or a `<formula>` | `window.width.center - ball.radius` |

Put together, a ball that bounces off the top and bottom, starts again when it leaves by a side, and turns off a paddle by where it hits:

```xml
<object name="ball">
  <sprite><rectangle><width>size</width><height>size</height></rectangle></sprite>
  <position><x><centered /></x><y><centered /></y></position>
  <velocity><x>ball.speed</x><y>2</y></velocity>
  <collisions>
    <collision edge="vertical"><bounce /></collision>      <!-- top or bottom: bounce -->
    <collision edge="horizontal"><reset /></collision>     <!-- out by a side: serve again -->
    <collision><deflect>45</deflect></collision>          <!-- anything else: off at up to 45 degrees -->
  </collisions>
  <variables>
    <variable name="speed">6</variable>
  </variables>
</object>
```

A rule reads as a sentence: *this ball, when it touches the top or bottom edge, bounces*. The whole of `pong_min.xml`, two paddles and a ball, is in the [root readme](../readme.md#a-whole-game).

Each frame the engine reads the keys and runs the current state's bindings, counts the timers, moves everything (finding every touch on the way and running its rules), checks the conditions, plays the sounds asked for and draws the picture ([the frame in full](#each-frame)).

## Running games

### xgecli

```
xgecli                       # loads "pong"
xgecli breakout              # a bare name gets ".xml" added
xgecli path/to/pong.xml      # a name with an extension is used as given
xgecli pong -w sdl2 -x tinyxml2 -a none
xgecli --game pong --window raylib --xml pugixml
xgecli galaxian --record     # play, and write what was played into the file as its <demo>
xgecli pong --generate windows-cpp -o out/pong
```

| Option | Means |
|---|---|
| *game*, `-g`, `--game` | the game, bare or with `-g` (not both). Default `pong` |
| `-w`, `--window` | window library: `sfml3` (default), `raylib`, `sdl2`, `opengl` |
| `-x`, `--xml` | XML library: `xerces` (default), `tinyxml2`, `pugixml`, `rapidxml` |
| `-a`, `--audio` | sound library: `sfml3` (default), `raylib`, `sdl2`, `none` |
| `--record` | play as usual from a new seed, and when the window closes write what was played into the game file as its [`<demo>`](#attract-mode) |
| `--generate <target>` | write the game out as a program of its own instead of playing it ([below](#generating-a-program)) |
| `-o`, `--output <dir>` | where `--generate` writes (default `<game>-<target>`); an error without `--generate` |
| `-h`, `--help` | the usage, listing the libraries this build has |

- **Finding the game.** As given (a path, or a name in the working directory), then its file name in the working directory, then in `descriptions/` of the **games folder**: the first folder holding both `descriptions/` and `assets/`, looked for in the working directory, the program's folder and the folder above it, each itself and then its `games/`. The program then makes the games folder its working directory, so it starts from anywhere and a game's asset paths (`assets/paddle.jpg`) are relative to it (`engine/folder.h`).
- **Options.** A short option takes its value attached or after a space (`-gpong`, `-g pong`); a long one needs the space (`--game pong`, never `--game=pong`). Names are not case sensitive. Each option may be given once. Only the libraries the program was built with can be named ([Backends](#backends)); a default build has `sfml3`, `xerces` and `none`. `--record` and `--generate` are long only and do not go together.
- The program prints the file, window, XML and sound library it chose, one to a line, then the game.
- A game file that is wrong (it does not match the schema, an expression will not evaluate, a command names something the game does not have) stops the load with a message saying where; `xgecli` prints it and exits. See [cli](designs/52-cli.md).

### xgegui

The Qt 6 application, built only when Qt is found. `xgegui pong` opens a game the same way, or a file dialog in `games/descriptions/` when none is named. The game is drawn beside a tree of its data, which can be edited while it runs; Play, Pause, Step and Reset drive it, and the Options dialog picks the video, XML and sound libraries. A game that will not load is shown as a message, and the program carries on. See [gui](designs/53-gui.md).

In C++ the libraries are chosen with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`.

### The shipped games

40 games in `games/descriptions/`. They share one key convention: **Space starts, pauses and plays again**; player one plays on **W A S D**, with the arrows as a second way in one-player games and player two's keys in two-player ones; where Space fires, **P or Escape** pauses.

| Game | Keys besides the convention | What it is, and what it shows |
|---|---|---|
| `pong` | W/S, Up/Down | Atari's arcade Pong: a square ball, a dashed net, first to eleven, [`<deflect>`](#touching) off the paddles |
| `pong_min` | W/S, Up/Down | two paddles and a ball, nothing else: the game [`--generate`](#generating-a-program) was grown on |
| `breakout` | A/D, the mouse | eight rows of bricks, the ball speeding up, a half paddle at the top; the paddle follows the mouse ([`<track />`](#pointing-controls)) |
| `spaceinvaders` | A/D, Space fires | the march speeding up, bombs, a mystery ship, shields worn away by [`<hole>`](#touching) |
| `spaceinvaders2` | as Space Invaders | the same game drawn from an SVG sprite sheet ([SVG pictures](#svg-pictures)) |
| `frogger` | W A S D hop | logs and diving turtles across the river, five bays against the clock, [`<ride />`](#touching) and [`unless=`](#rule-filters) |
| `spacerace` | W/S, Up/Down | two rockets through drifting debris, the most crossings in a minute |
| `kaboom` | A/D, the mouse | the Mad Bomber's eight waves as [scenes](#scenes); catch the bombs |
| `freeway` | W/S, Up/Down | two chickens across ten lanes; a hit knocks one back a lane; 2:16 |
| `depthcharge` | A/D, Space drops | charges off both sides, submarines in four lanes laying mines, 90 seconds |
| `astrosmash` | A/D, Space fires | falling rocks that split, spinners, a spare base each 1,000 |
| `lunarlander` | Left/Right turn, Up thrusts | land upright and slowly on pads worth 2X, 3X, 5X until the fuel is gone; [`<line>`](#lines) pictures, `pixel` collisions |
| `asteroids` | Left/Right turn, Up thrusts, Space fires, Down hyperspace | rocks that break, waves, shots that wrap and run out, a ship that comes back when the middle is clear, a big saucer that shoots anywhere and a small one that aims (likelier as the score climbs), rocks that break the saucers, the ship's flame, the heartbeat as music, the best score kept |
| `berserk` | W A S D walk and face, Space fires | four rooms as [maps](#maps), robots that patrol, line up and fire, Evil Otto, the exit shut behind you |
| `demonattack` | A/D, Space fires | three waves of demons that fire back and split |
| `frostbite` | W/S jump between rows, A/D walk | build the igloo by landing on white ice; three nights |
| `galaxian` | A/D, Space fires; 2 on the title for two players | the fleet flies in on [paths](#paths), sways, and dives at the ship (a flagship with its two nearest escorts), worth more diving; a top five with initials, [turns](#taking-turns), an [attract mode](#attract-mode) |
| `donkeykong` | A/D walk, W/S climb, Space jumps | sloping girders, ladders, barrels that sometimes take a ladder, fireballs from the oil drum, three times to Pauline |
| `pitfall` | A/D run, Space jumps, W/S climb | four jungle screens built from a [kit](#the-kit-and-from), logs, tar, crocodiles, treasure |
| `missilecommand` | W A S D move the sight, Space launches | counter-missiles trailing a [`<tether>`](#sprites), fireballs, three waves |
| `combat` | W/S drive, A/D turn, F or Left Shift fires; Up/Down, Left/Right, Right Shift or Right Control | two tanks among walls, a hit knocks the tank back |
| `airseabattle` | A S D point, W fires; Left Down Right, Up | two guns, four lanes of planes, 2:16 |
| `megamania` | A/D, Space fires | eight waves that wrap round, the energy as a [timer's time left](#time-left) |
| `atlantis` | A, W, D fire the three posts | Gorgon ships dropping a lane each pass; named timers and conditions copied by [`from=`](#scenes) |
| `pacman` | W A S D steer | the arcade maze as a [map](#maps), four ghosts each hunting their own way ([walking a map](#walking-a-map)) |
| `adventure` | W A S D walk, Space drops | five rooms, keys, castles, dragons; [`<carry />`](#touching) |
| `trickshot` | W A S D move the cue tip | pool: balls that [`<knock>`](#knocks) one another, slowed by `<drag>` |
| `mariobros` | A/D walk, W or Space jumps; 2 on the title for Luigi on the arrows | bump the floor under a pest to flip it, six phases (shellcreepers, sidesteppers, fighter flies, a bonus phase of coins); [`<poses>`](#poses-mirror-grip-and-blinking), a window [`<scale>`](#window), [palette swaps](#palette-swap), [scenes](#scenes) |
| `fishingderby` | W A S D move the hook | you against a [computer](#computer-players) fisherman: tethers for the lines, values [read when used](#values-read-when-used) |
| `qbert` | W D S A hop up-right, down-right, down-left, up-left | a pyramid of cubes as a map, timed hops with an `<arc>`, Coily hunting by hops |
| `yarsrevenge` | W A S D fly, Space fires | the shield a moving map, a destroyer missile that [`<seek>`](#parts-of-an-object)s, the neutral zone |
| `tennis` | W A S D walk | a ball with an [altitude](#height-and-shadows) and a shadow, a computer at the far end, the score in words |
| `hauntedhouse` | W A S D walk, Space strikes a match | a dark house seen only by [light](#light-and-dark), four floors, three pieces of an urn |
| `tetris` | A/D move, W/X turn, E/C turn back, S drops faster, Space drops at once; P pauses | seven shapes in a well of [cells](#cells), standard turns and kicks, full rows cleared, faster every ten rows, the next shape in a box (`<send>`) |
| `columns` | A/D move, W/X/Space cycle the jewels, S drops faster; P pauses | columns of three random jewels; lines of three alike across, down or slanting go, and the rest [fall](#cells) into chains |
| `capsules` | A/D move, W/X turn, E/C turn back, S drops faster; P pauses | germs in a bottle and two-coloured capsules, four of a colour across or down to clear them; halves with [`<joins>`](#cells) looks that stay joined, a robot doctor who throws the next capsule |
| `snake` | W A S D or the arrows turn; Space or P pauses | a snake that goes on by itself with a [chain](#cells) for its body, food in a free cell, faster as it grows |
| `puttpark` | A/D or Left/Right aim, Space held then let go putts; P or Escape pauses | an original mini golf of six holes as [scenes](#scenes): walls drawn as [`<line>`](#lines)s that the ball [bounces](#touching) off at their slope, posts that kick it away faster, soft cushions; the putt built up by a key [held and let go](#a-keys-three-moments) |
| `pinball` | A or Left, D or Right flip; Space held then let go shoots; P or Escape pauses | an original table: flippers that [swing about a pivot](#turning-about-a-pivot) and lend the ball their speed where they strike it, a plunger [held and let go](#a-keys-three-moments), bumpers and slingshots that [bounce](#touching) it off faster, three rollover lanes for a bonus |
| `centipede` | W A S D move the wand about its zone, Space fires; P or Escape pauses | a centipede winding down through a mushroom field, [split](#cells) in two where a shot hits it; each piece left grows a head; a line nobody sees over the bottom six rows turns it back down and holds the wand in |

What each game was written to test, and what it left out: [games-classic](designs/54-games-classic.md), [games-written-by-ai](designs/55-games-written-by-ai.md), [games-atari](designs/56-games-atari.md).

### Generating a program

```
xgecli pong --generate windows-cpp                     # writes pong-windows-cpp/
xgecli pong --generate windows-cpp -o out/pong         # or into the folder named
```

`--generate windows-cpp` does not play the game: it writes a C++ program on SFML 3 that is **the engine with the game built in**, so it plays exactly as `xgecli` does. The engine first loads the game as it would to play it (a game it will not load is not generated), then XSLT stylesheets in `games/generators/windows-cpp/` (run by libxslt) write the program:

| Written | By | What |
|---|---|---|
| `source/<game>.cpp` | `merge.xsl`, then `data.xsl` | the game as the engine's own data, the `GameData` (`game/data.h`) `readGame` makes of the file, a line of C++ for each tag. The [kit and every `from=`](#the-kit-and-from) are worked out first, as the engine works them out. One function returns the game (`pongMinGame()`) |
| `source/game_parts.h` | copied from `modules/` | the small functions those lines call |
| `source/main.cpp`, `CMakeLists.txt`, `README.md`, `.gitignore` | `program.xsl` | makes the `Game`, an `SFMLWindow` and an `SFMLAudio` (silent with no sound device) and runs `Engine::loop`; the build finds or fetches SFML 3 and lunasvg |
| `engine/` | listed in `engine.xml` | the engine's own files, copied unchanged from `xge/lib/` of the repository `xgecli` was built from. Left out: reading XML, the other libraries and the factories that choose among them |
| `assets/`, `LICENSE` | `xgecli` | the font if it shows text (and a font's licence beside it), the pictures it names, the repository's licence |

`xgetest-generated` (left out of an ordinary build; `cmake --build build --target xgetest-generated`) generates every game, compiles its data in, compares it with what `readGame` reads from the file, field by field, and plays the two side by side, frame by frame. Without libxslt `xgecli` is built without `--generate` and says so. See [generator](designs/40-generator.md).

## The game file

### Layout

One `<game>` root, its children in this order (the schema enforces it):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="Pong"> ... </window>            <!-- size, scale, background, frame rate -->
  <variables> ... </variables>                  <!-- numbers the whole game uses -->
  <sounds> ... </sounds>                        <!-- optional: notes and music -->
  <paths> ... </paths>                          <!-- optional: routes objects fly -->
  <styles> ... </styles>                        <!-- optional: named text and bitmap looks -->
  <kit> ... </kit>                              <!-- optional: pieces written once, taken below -->
  <objects> ... </objects>                      <!-- optional: what every state shares -->
  <keeps> ... </keeps>                          <!-- optional: what a new game passes over -->
  <states> ... </states>                        <!-- title, playing, paused, game over ... -->
  <demo> ... </demo>                            <!-- optional: a recorded game -->
</game>
```

**Where an object is written says how long it lasts:** the game's `<objects>` for the whole game, a [state](#states)'s while it is the current state, a [scene](#scenes)'s while it is the current scene. What is written once and used in several places, or only ever spawned, goes in the [`<kit>`](#the-kit-and-from). A game whose states own all their objects has no game-level `<objects>`.

A game with no `xsi:noNamespaceSchemaLocation` is not validated. The schema checks shape and where each command may go; the loader checks names and meaning once everything is read.

### The format's one rule

**An attribute names or picks something; everything else is the content of an element.**

- Attributes are labels that cannot be computed: `name`, `class`, `object`, `variable`, `state`, `action`, `button`, `edge`, `direction`, `unless`, `sound`, `wave`, `pitch`, `to`. `<inc variable="paddle1.score" />`, `<collision edge="left">`, `<input button="space">`.
- Everything else, a size, a color, a position, a flag, a verb's number, is written between tags: `<radius>10</radius>`, `<fullscreen>true</fullscreen>`, `<chase object="hero">2</chase>`.
- A verb with one number has it as its content; anything more follows as named tags: `<jump direction="up">rowgap<seconds>leap</seconds></jump>`.
- What something *does* is a list of command tags, run in the order written. There are no `;`-separated strings.

An attribute holds one name: several are an element each (`<button>a</button><button>left</button>`, `<keys>`, a row's `<number>`). A note's `pitch="F#4"` is one name, a standard one, like a colour's.

### Values

Wherever an element holds a number (a position, a speed, a size, a variable, a threshold) it is a **value**, written one of three ways. A value holds text or one tag, never both and never two tags.

| Form | Example | Notes |
|---|---|---|
| arithmetic as text | `window.width.center - title.width / 2` | numbers, names, `+ - * /`, a sign in front and brackets, worked out by the engine's own `load/calc`; nothing more (no functions, no comparisons) |
| `<random>` | `<random><min>-7</min><max>7</max></random>` | a number from min to max (either order; each end an expression or a `<formula>`) |
| `<formula>` | see [below](#arithmetic-as-tags) | the same arithmetic as tags, nothing left in a string for a tool to parse |

**When a value is worked out.** Once, when the game loads, with three exceptions:

- a timer's `<every>` and `<after>`, worked out again each time the timer starts over;
- a position that names a text's or an image's size, finished once the window has measured it and again whenever that size changes;
- the values of a **command** and of a **condition's test**, read as they run ([below](#values-read-when-used)).

A `<random>` is drawn at load and again whenever its object comes back into play: a reset (of it, of its scene or of the game), a `<reveal>`, `<release>` or `<fire>`, or a new copy made. So each Pong serve is new. A `<random>` in a game-level variable is drawn once; entering a scene draws nothing.

#### Names a value can use

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | the window's height, its width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| a game `<variable>` | its value (worked out in the order written, so one can use those above it) |
| `object.variable` | that object's variable, wherever the object is in the file |
| `object.width`, `object.height` | the object's size (its sprite's footprint); meant for `<position>`. An object names itself by its own name |
| `object.timer` | a named object timer's [time left](#time-left), whole seconds rounded up |
| `object.x`, `.y`, `.velocity.x`, `.velocity.y`, `.heading`, `.altitude`, `.rise`, `.light`, `.blink` | **live numbers**: where it is (its top left), how it goes, its [height](#height-and-shadows), its [light](#light-and-dark), its [blinking](#poses-mirror-grip-and-blinking). Only a command's or a test's value can read them |
| `self.` before any of these | in an object's own rules, actions and timers, the object running it (`self.worth`) |
| `touch.x`, `touch.y`, `touch.along`, `touch.depth`, `touch.normal.x`, `touch.normal.y` | in a rule about another object: the point of the touch, where along the object's own side (0 to 1), how far they went into each other, the way out of the other toward it |

An object's own `<variable>` named like a size or a live number (`width`, `x`) wins over it. Anything past `+ - * /` (the smallest of two, a sign, a sine) is a [formula tag](#arithmetic-as-tags); a text that calls a function (`max(6, 14 - player.depth)`) stops the load and says so.

#### Values read when used

A command whose numbers name something that changes (an object's variable, a live number, the touch) is worked out again every time it runs, from what is so then: `<inc variable="player.score">self.worth</inc>` adds the worth of the fish just landed, and `<set variable="shark.velocity.x">shark.velocity.x * 1.1</set>` speeds the shark up. A condition's test works the same way: `<above>gun2.score</above>` follows gun 2's score. Every other number is fixed at load, and a value that is not a command's (a position, a size) naming a live number stops the load, since nothing is anywhere yet.

#### Arithmetic as tags

A `<formula>` is one operation whose operands are elements, each a name, a number, a `<random>` or another operation. Precedence is the nesting, so there is nothing to parse; text and formula give the same number and sit side by side in one game. Plain sums stay text; whatever text cannot say is a formula (Berserk's `<largest>` keeps a wait above a floor, Pong serves with `<sign>`, `<sine>` and `<cosine>`).

| Operation | First operand | Second operand(s) | Means |
|---|---|---|---|
| `<add>` | `<augend>` | `<addend>` | first + second |
| `<subtract>` | `<minuend>` | `<subtrahend>` | first - second |
| `<multiply>` | `<multiplicand>` | `<multiplier>` | first * second |
| `<divide>` | `<dividend>` | `<divisor>` | first / second |
| `<remainder>` | `<dividend>` | `<divisor>` (one) | what is left over, from 0 up to the divisor, so a count wraps (-1 of 4 leaves 3) |
| `<smallest>` | `<value>` | `<value>` | the smallest of two or more |
| `<largest>` | `<value>` | `<value>` | the largest of two or more |
| `<absolute>` | `<value>` | | the one `<value>` without its sign |
| `<negate>` | `<value>` | | the `<value>` with its sign turned over |
| `<sign>` | `<value>` | | -1, 0 or 1, as the `<value>` is below, at or above 0 |
| `<floor>`, `<ceiling>`, `<round>` | `<value>` | | the `<value>` to a whole number: down, up, or the nearest (a half goes up, away from 0) |
| `<sine>`, `<cosine>` | `<value>` | | of the `<value>` as an angle in degrees, like every angle in a game |
| `<clamp>` | `<value>` | `<atleast>`, then `<atmost>` | the `<value>`, kept from `<atleast>` up to `<atmost>` |
| `<compare>` | `<value>` | its test: `<above>`, `<below>`, `<atleast>`, `<atmost>` or `<equal>` (one) | 1 when the `<value>` passes the test, 0 when not; the same words as a [condition's tests](#conditions). Pac-Man chases for ever from the fourth wave: 20 + 100000 × `<compare><value>mode.waves</value><atleast>4</atleast></compare>` |

```xml
<x>
  <formula>
    <subtract>
      <minuend>window.width.center</minuend>
      <subtrahend><divide><dividend>title.width</dividend><divisor>2</divisor></divide></subtrahend>
    </subtract>
  </formula>
</x>
```

- An operand is a **name or a number**, never an expression; anything more is an operation of its own.
- Several second operands chain left to right: `a - b - c` is one `<subtract>` with two `<subtrahend>`s.
- A `<random>`'s `<min>` or `<max>` can be a `<formula>` in place of its text: `<random><min><formula><smallest><value>w</value><value>7</value></smallest></formula></min><max>20</max></random>`.
- **Dividing by 0.** A divisor certainly 0 at load (a number, a game variable, or an operation of only those) stops the load, naming the `<divide>`. One that is 0 only because it is not known yet (another object's variable, a text not yet measured) gives 0 quietly, and a position or interval made from it is worked out again later. While the game runs a divisor of 0 gives 0 and one warning per place. Expression text divides by 0 as it always did.

### Names

Names are dotted, and every dot means "inside":

| Name | Is |
|---|---|
| `ball.radius` | the ball's variable `radius` |
| `logrow3.2`, `aliens.3.2`, `maze.dots.17` | a [group](#groups)'s member, a cell (column 3, row 2), a [map](#maps)'s piece |
| `playing.hero` | the state `playing`'s own object |
| `playing.room2.robots.3` | a member of a scene's group |
| `playing.room2.pace` | a scene's variable |
| `shot.1` | a [copy](#spawn-and-despawn) made while the game runs |

- A name written in the file is looked for **from the inside out**, from where it is written: the scene's own, then its state's, then the game's, and last as the end of exactly one path. So the rules inside a scene say `robots` and mean their own, and `hero` means `playing.hero` anywhere.
- A scene's object also answers to `state.name` (`playing.robots`): the robots of every scene of the state, so a state-level rule saying `robots` means whichever room's it meets. Where a short name could mean two things, the shortest path wins; two as short stop the load (`'rock' could be playing.rock, practice.rock; write the path of the one meant`).
- A name given twice where both can be seen (a scene's object named like its state's, a state's like the game's), and two scenes of one name, stop the load.
- A reference is split at its **last** dot into owner and variable, and the arithmetic sees every name as one plain symbol.
- `class="..."` puts an object in a kind that rules and conditions can match, like a CSS class; one object can be named alone, a class names all of its kind.

## Window, variables and styles

### Window

`<window name="Pong">` (the title), then in this order:

| Element | Default | Meaning |
|---|---|---|
| `<width>`, `<height>` | required | the game's size in pixels. Plain whole numbers, since everything else is worked out against them |
| `<scale>` | 1 | each of the game's pixels is drawn as a square this many window pixels across. The game is written and played at its own size; only the window is bigger. Mario Bros. is 256 by 224 at `<scale>3</scale>`, the arcade's screen. The mouse is read back in game pixels |
| `<background>` | `color.black` | a [color](#colors) name |
| `<fullscreen>` | `false` | `true` or `false` |
| `<framerate>` | 60 | frames a second, up to 255. It is how often the whole engine runs, keys and timers included, so a slow game keeps 60 and steps on a timer rather than lowering it |

### Variables

```xml
<variables>
  <variable name="margin">30</variable>
  <variable name="paddle">margin * 4</variable>
  <variable name="start"><random><min>1</min><max>3</max></random></variable>
</variables>
```

Game-level variables are **named constants** any value can use. They are worked out in the order written. A name may hold dots, and is then always that variable. Declaring a name twice keeps the later value. Numbers that change belong to objects (`paddle1.score`); an object with no picture can hold them ([numbers only](#parts-of-an-object)).

### Styles

A style is a named look, said once and taken by any `<text style="...">` or `<bitmap style="...">`:

```xml
<styles>
  <style name="banner"><size>64</size><color>color.red</color><font>assets/pressstart2p.ttf</font></style>
  <style name="mario">
    <scale>3</scale>
    <color>color.red</color>              <!-- the * pixels -->
    <color char="b">color.blue</color>    <!-- each b pixel -->
  </style>
</styles>
```

| Part | For | Meaning |
|---|---|---|
| `<size>` | text | a value, worked out with the game's names |
| `<scale>` | bitmap | how many pixels each character is |
| `<color>`, `<color char="x">` | text takes the plain one; a bitmap takes all | colors |
| `<font>` | text | a font file, relative to the games folder. `assets/tuffy.ttf` is the font when none is named; `assets/pressstart2p.ttf` (Press Start 2P, letters 8 by 8 at size 8, so use multiples of 8) is used by Mario Bros., Q*bert, Pac-Man and Donkey Kong. Its licence, the SIL Open Font License, is `pressstart2p-ofl.txt` beside it |

What a text or bitmap gives itself wins over its style. A `style=` naming no style, or a text left with no size at all, stops the load. The shipped games name styles by role: `banner` (titles and endings), `prompt` (press Space), `help`, `keys`, `hud` (score and lives).

#### Palette swap

`<become style="angry" />` puts another style's colors on an object's bitmaps:

- each bitmap that takes a style takes that one instead, for the colors it took from its own style;
- colors a bitmap gives itself stay, and so does any color the new style leaves out, so a swap style names only what changes;
- the look stays (with `sprite=` as well, both change), a moving look goes on where it was, and a reset puts the first colors back.

The pictures for each style a `<become>` can give the object are drawn as the game loads, so a swap costs nothing while playing. Mario Bros.' pests are drawn once, each kind in its own style; one that gets up by itself does `<become style="angry" />`, and the second time a rule puts on `furious`. One `angry` style recolours every kind, since a style's characters that a bitmap does not use are passed over. A style naming nothing, an object with no bitmap that takes a style, or one with a `<heading>`, stops the load.

## Objects

An object is anything that can be drawn, moved or collided with. `name` is required (how everything else refers to it) and `class` optional (a kind that rules and conditions can match). `from=` takes a piece of the [kit](#the-kit-and-from).

### Parts of an object

Every part but the position is optional, and **what is left out is nothing**: no velocity stands still, no `<collisions>` touches nothing. In this order:

| Part | What it does |
|---|---|
| `<sprite>` | what it looks like: one [shape](#sprites). Several named sprites are [looks](#looks) or the frames of an [animation](#animation) |
| `<animation>` | which sprites are shown, in what order, for how long |
| `<position>` | where it starts, `<x>` and `<y>` from the top left, each a value. `<centered>value</centered>` puts its middle there instead (empty: the window's middle). Required, unless it has no sprite |
| `<velocity>` | pixels a frame, `<x>` and `<y>`; a half left out is 0 |
| `<acceleration>` | added to the velocity every frame before anything moves: gravity is a `<y>`. `<stop />` takes it away, a reset gives it back |
| `<heading>` | degrees clockwise from straight up. Lets `<turn>`, `<thrust>` and `<swing>` work, and turns the picture |
| `<pivot>` | after `<heading>`: the point the picture turns about, `<x>` and `<y>` in the drawing's own pixels (a flipper's hinge at one end). The `<position>` is then where that point sits, and it stays there however the object turns ([turning about a pivot](#turning-about-a-pivot)). Left out, it turns about its middle |
| `<drag>` | 0 up to 1: the share of its speed lost each frame (a pool ball on the cloth) |
| `<friction>` | 0 or more, after `<drag>`: how much a still surface it touches holds it back along that surface, as a share of how hard it met it; and what brings it to [rest](#knocks) |
| `<mass>` | above 0: how hard it is to [knock](#knocks). Left out, a knock does not move it |
| `<pull object="sun">strength</pull>` | that much more speed toward the nearest of that name each frame; several add up |
| `<seek object="ship">degrees</seek>` | a homing missile: the way it goes turns toward the nearest of that name, at most that many degrees a frame, at the same speed |
| `<light>`, `<dark>` | how far round it lights; `unlit` or `unseen`: drawn only when lit ([light and dark](#light-and-dark)) |
| `<altitude>`, `<shadow object>` | how high off the ground; kept under another object ([height and shadows](#height-and-shadows)) |
| `<facing>` | `up`, `down`, `left` or `right`, without turning the picture: follows its last move, and a `<fire>` leaves that way |
| `<poses>`, `<mirror>`, `<grip>` | looks picked by how it moves, pictures flipped going one way, how quickly its speed follows the keys ([poses](#poses-mirror-grip-and-blinking)) |
| `<maze map="maze">` | walks that map's tiles ([walking a map](#walking-a-map)) |
| `<active>` | `false`: starts out of play (not drawn, touching nothing, not counted) until a `<reveal>`, `<release>` or `<fire>` brings it in |
| `<drawn>` | `false`: in play (moves, touches, counts its timers) but never drawn: an exit, a trigger zone |
| `<most>` | how many [copies](#spawn-and-despawn) of it may be out at once |
| `<collisions>` | how and what it touches ([collisions](#collisions)). Written, it is on (`<collisions />` alone is something others touch) unless it says `<enabled>false</enabled>` |
| `<actions>` | named `<action>`s: what it does when a key or a rule runs that name |
| `<variables>` | its own numbers, read anywhere as `name.variable` |
| `<timers>` | what it does on its own, every so often or once after a while ([timers](#timers)) |

- **Numbers only.** An object with no `<sprite>` only holds numbers: a `status` with the score and lives, a `tally`. It is never drawn, needs no position, and must have `<variables>` or `<timers>`. Keep score, lives and clock on such an object that no state draws (Pitfall's `status`), so a reset of the player does not touch them.
- **Projectiles.** An object with `class="projectile"` starts out of play, waiting to be fired.
- A member of a group, an object on a map and a piece of the kit have the same parts (the schema's `objectParts`), so they cannot drift apart.

### Groups

A `<group>` is several objects written once: a lane of logs, a wall of bricks, the pads of Frogger. It sits beside `<object>` in any `<objects>`, gives the parts its members share, then lists its members. **Whatever a member leaves out it takes from its group**; after that it must be as complete as an object.

```xml
<group name="logrow3" class="logs">
  <sprite><rectangle><width>3 * cell</width><height>body</height><color>color.brown</color></rectangle></sprite>
  <position><y>3 * cell</y></position>                 <!-- the row, once -->
  <velocity><x>-1</x></velocity>
  <collisions>
    <collision edge="horizontal"><wrap /></collision>
  </collisions>
  <member><position><x>20</x></position></member>      <!-- where along it each starts -->
  <member><position><x>272</x></position></member>
  <member name="last"><position><x>524</x></position></member>
</group>
```

- **Members are ordinary objects**, named after the group with a number from 1 in the order written (`logrow3.1`, `logrow3.2`), or by their own `name`. Each is drawn, moved and touched on its own; each `<wrap />` or `<die />` happens to one member.
- **The group's name means every member** wherever a name is taken: `<reset object="logrow3" />`, `object="logrow3"` in a rule or a condition. The group's `class` is every member's.
- **A member gives only** its `<sprite>`s, `<animation>`s, `<position>` and `<velocity>`; a position or velocity can give just its `<x>` or `<y>`. Rules, actions, variables and timers are the group's, so every member acts the same way; one that acts differently is an object beside the group.
- **A member's sprite changes the group's of the same name.** Of the same shape it gives only what changes: `<sprite><rectangle><width>10</width></rectangle></sprite>` is the group's rectangle, 10 wide; a text's pieces are new words in the group's size and color; a tether names its two ends again. Of another shape it must be complete. A new name adds a sprite.
- Members are drawn in the order written, where the group stands in the file.

#### Groups inside a group

A group can hold groups instead of members, and **an inner group takes from the one around it** the same way. Frogger's ten lanes all wrap, so the `<wrap />` is written once, in a group of all ten; the river's log lanes share a log, each lane giving its own row, gap and speed:

```xml
<group name="lanes">
  <rows>1</rows>
  <collisions><collision edge="horizontal"><wrap /></collision></collisions>
  <group name="river" class="logs">
    <sprite><bitmap> ... </bitmap></sprite>
    <group name="logrow2">
      <columns>3</columns>
      <padding><x>(window.right + 180) / 3 - 180</x></padding>
      <position><x>20</x><y>2 * cell + 6</y></position>
      <velocity><x>1.4</x></velocity>
    </group>
    <group name="logrow4"> ... </group>
  </group>
  <group name="traffic" class="hazard"> ... </group>
</group>
```

- **A single thing is replaced** by the inner group's (a half of the position, velocity or padding, `<columns>`, `<rows>`, the animation, acceleration, heading, facing, `<active>`, the class). A sprite changes the outer's of its name, as a member's does.
- **A list adds:** the inner group's collision rules run after the outer's; its actions, variables and timers join the outer's (one of the same name replaces it).
- **Names.** Members are named after their innermost group (`logrow4.2.1`), and every group around them names them too: `object="river"` is every log.
- A group holds members, cells or groups, never two of these. Groups nest as deep as wanted.

#### Columns and rows

Instead of listing members, a group can lay out a block: `<columns>` and `<rows>` (whole numbers) make a cell for each place, and `<padding>` is the gap between them. They come first in the group; its `<position>` is the top left of the first cell.

```xml
<group name="bricks" class="bricks">
  <columns>14</columns>
  <rows>8</rows>
  <padding><x>4</x><y>4</y></padding>
  <sprite><rectangle><width>brick</width><height>12</height><color>color.red</color></rectangle></sprite>
  <position><x>margin</x><y>top</y></position>
  <collisions> ... </collisions>
  <row><number>3</number><number>4</number><sprite><rectangle><color>color.orange</color></rectangle></sprite></row>
  <row number="5"><sprite><rectangle><color>color.green</color></rectangle></sprite></row>
  <cell row="1" column="7" name="boss"> ... </cell>
</group>
```

- Each cell is an ordinary object named after the group, its column and its row (`bricks.5.3`), made the top row first, left to right.
- `<slope>N</slope>` after the padding puts each column N pixels lower than the one before it (higher when N is below 0). Donkey Kong's girders are rows of truss pieces each half a pixel lower than the last, sloping down to their open ends as the arcade's do.
- Three tags change what the cells they pick have, from the general to the particular:

| Tag | Picks | Can change |
|---|---|---|
| `<row number="2">` | one row by `number=`; several by a `<number>` each, first (`<row><number>2</number><number>4</number>`); every other by `<odd />` or `<even />` | sprites, animation, velocity, padding (`<y>` the gap above it, `<x>` between its cells), variables |
| `<column number="...">` | columns, the same way | the same; its padding's `<x>` is the gap to its left |
| `<cell row="..." column="..." name="...">` | one cell, and may name it | sprites, animation, velocity, variables |

- A cell is a slot the size of its row's sprite; a cell of another size sits in the middle of its slot and its neighbours stay put.
- A `<cell>` wins over the rows and columns that pick it, but a row and a column (or two rows) that change the same thing in one cell stop the load: which should win is not something to guess.
- Space Invaders is one group of 11 columns and 5 rows: the squids' two sprites and animation are the group's, and two `<row>`s give the crabs' and octopuses' pictures.

### Maps

A `<map>` is level data drawn as text: each `<row>` is a row of tiles, one character to a tile, so the file shows the level as it looks. It stands in any `<objects>`, or in the kit.

```xml
<map name="maze">
  <tile><width>tile</width><height>tile</height></tile>
  <position><x>0</x><y>top</y></position>
  <row>r--------T--------7</row>
  <row>|o.......|.......o|</row>
  <row>|.[].[-].u.[-].[].|</row>
  ...
  <group name="walls" class="wall">
    <group name="across" char="-"> <sprite>...</sprite> </group>
    <group name="corner" char="r"> <sprite>...</sprite> </group>
  </group>
  <group name="dots" char="." class="food"> ... </group>
  <object name="pacman" char="@"> ... </object>
</map>
```

- `<tile>` is a tile's size and `<position>` the map's top left; the rows must all be as wide.
- **A group** picking a character with `char="x"` has a member in the middle of every tile of it, named under the map and numbered in reading order (`maze.dots.1`, `maze.dots.2`), so two maps in one list each number their own: `left.walls.1` and `right.walls.1` are both `walls`, and the map's name picks one map's. It has no position, layout or members of its own, but everything else a group has, inner groups included: Pac-Man's walls are one group of class `wall` whose bitmap each piece changes.
- **An object** picks a character found on exactly one tile, where it starts.
- A character nothing picks is an empty tile; two pieces may not pick one character. Rows are trimmed like any text, so an empty tile at either end is a character nobody picks (Pac-Man's `,`), not a space.
- The map's name means everything on it (`<reset object="maze" />`).

**A map in the kit.** Where several maps share their tile, place and pieces (Berserk's four rooms, Haunted House's four floors), the kit holds a `<map>` that says them once, and each map in play takes it by name or by `from=` and gives only what differs, most often its rows: `<map name="maze"><row>+-+-+x+-+-+</row> ... </map>`. A map takes only a map piece; what it still lacks once taken (a tile, a row) stops the load.

#### Walking a map

An object with `<maze map="maze">` walks that map's tiles: it keeps to the middle of its lane, chooses its way only in the middle of a tile, and never goes onto a tile that is a wall to it.

- **Walls** are the `<wall>`s inside: `<wall class="wall" />` is every object of that class on the map that is in play; `<wall class="door" sprite="normal" />` is one only while the walker shows its `normal` [look](#looks), which is how a ghost's eyes go in through a door that stops the ghost.
- Off the map (a tunnel) it goes straight on, and the window's edge can `<wrap />` it round.
- **Slow ground** is a `<slow class="tunnel">0.5</slow>` inside: on a tile with an object of that class (in play, drawn or not) it goes at that share of its speed. Pac-Man's ghosts go at half speed on the tunnel's tiles.
- **How it chooses** comes from the last of three [steering verbs](#steering) it was given: `<go>` (a player's stick), `<hunt>` (the arcade's targeting, no search of the maze) or `<wander>` (at random).
- It faces the way it goes and turns its `<heading>` if it has one. `<reverse />` turns it round; `<stop />` or a reset stops it until it is told again; on a path or in a jump, that moves it, and it carries on from the tile it is left on.
- A tile is found from the walker's middle, so a walker of any size works; it should go less than half a tile a frame.

#### Cells

A map with `<cells />` is a playfield of cells, such as Tetris's well: its pieces move a whole cell at a time, it knows which cell holds what, and it can be asked questions about its rows.

```xml
<map name="board">
  <tile><width>cell</width><height>cell</height></tile>
  <position><x>left</x><y>top</y></position>
  <columns>10</columns>                  <!-- in place of <row>s: an empty map -->
  <rows>20</rows>
  <cells />
  <group name="blocks" class="block">
    <group name="cyan" char="i"> ... </group>        <!-- on no tile: waits to be spawned -->
  </group>
  <group name="pieces" class="falling">
    <timers><timer><every>status.gravity</every><hop direction="down" /></timer></timers>
    <collisions><collision edge="bottom"><settle /></collision></collisions>
    <group name="t">
      <shape>
        <row>.t.</row>
        <row>ttt</row>
        <row>...</row>
      </shape>
    </group>
  </group>
  <checks>
    <full class="block"><clear /><collapse /><inc variable="status.lines" /></full>
  </checks>
</map>
```

- **Its size** is its `<row>`s, as any map's, or `<columns>` and `<rows>` for one that starts empty (not both).
- **A piece waiting.** A group or an object on a cell map that picks no character, or one on no tile, is one piece out of play (`board.cyan`), waiting to be [spawned](#spawn-and-despawn) into a cell: `<spawn object="pieces"><random /><column>4</column><row>1</row></spawn>`. Column and row count from 1; left out, they are the spawner's own cell. `<free />` in their place is a free cell of the map at random, never one a refused step is going into (Snake's next food); `<free><below>6</below></free>` is one under row 6 (Capsules' germs). `<random />` picks one member of a group at random. A spawn into a cell that is taken, or off the map, is refused, so the rest of the action is not done (Tetris's game over).
- **A `<shape>`** is a piece of several cells, drawn as rows of characters: each character is the piece on the map that picks it (`t` the magenta block), and a `.` (or any character nothing picks) is no cell. **A group of groups** may pick one character for all of them (`<group name="jewels" char="j">` holding a group per colour): each waits, and a shape's cell of `j` is any one of them, at random (Columns' jewels). A shape has no sprite; its cells, spawned with it, are drawn. Live numbers `column` and `row` are a piece's cell.
- **Moving.** `<hop direction="down" />` steps a cell (or its content, a whole number of cells); a held `<move direction="left">12<wait>0.2</wait></move>` steps once, waits `<wait>` seconds, then goes `12` cells a second while held; `<slide direction="down" />` goes cell by cell, at once, until a step is refused, and that refused step is a touch as any is (Tetris's hard drop lands that way); `<turn direction="clockwise" />` on a shape is a quarter turn, once a press; `<cycle />` moves each of a shape's cells on to the next cell of its outline, in reading order, the last to the first (Columns' jewels each go down one, the bottom one to the top).
- **A refused move is a touch.** A step into a taken cell, or off the map, is not made, and runs the rules of both for that touch: `<collision class="block" side="bottom">` for a block under it, `<collision edge="bottom">` for the map's floor. If the rules take away what was in the way (`<despawn />`), the step goes on: that is how Snake eats. A [`<through />`](#touching) in them makes it into the taken cell anyway, the two sharing it (Centipede's turn down). Cell pieces touch one another only this way, never by pixels, and the window's edges do not apply to them.
- **Going on by itself.** `<go direction="up">8</go>` on a cell map keeps the piece going that way, 8 cells a second, until another `<go>` (it is a key's verb, in an action); a turn keeps the step's timing. A go straight back into its chain's first link is refused.
- **A chain.** `<chain object="body" />` on a piece: the pieces of that name or group behind it, beside one another in the map's rows (`ooo@`), are its links, and each step each link takes the cell of the one ahead. Its tail is no blocker, since it leaves as the head comes. `<grow />` makes it one longer: after its next step (the step being made, when food is eaten) a new link of the chain's piece fills the cell the tail left. `<split />` on a link (a rule of the link that is shot) cuts the chain there: the links ahead stay with the head, the next link behind becomes a head of its own with the rest, going the old head's way and taking its look, so its rules are the head's (Centipede). `<reverse />` turns a `<go>` on cells round. A step whose touch rules move the piece (Centipede's turn: reverse and hop down) is not made as well.
- **Turning** is the modern rule (the Super Rotation System): a shape turns about the middle of the square it is drawn in (3x3 for T S Z J L, 4x4 for I; a 2x2 never changes), and a turn that does not fit tries the standard kicks off walls and the floor in order. A shape of two cells (a capsule, `<row>cc</row>`) turns about its bottom left cell: lying, it stands up from its left cell (down from it in the top row); standing, it lies along the row of its bottom cell (one to the left against a wall). A square of another size turns in place or not at all.
- **`<settle />`** ends a shape: it is despawned and its cells stay, loose. Then the map asks its `<checks>`.
- **Joined pieces.** `<joins>` on a piece (after `<grip>`) names a look for each way the piece it is joined to can lie, and for none: `<alone sprite="pill" /><left sprite="openleft" /><right .../><up .../><down .../>`. While it falls it is joined to the other cells of its shape; settled, cells with `<joins>` stay joined to one another, and a `<fall />` moves them together, as far as all of them can go. At the end of every frame each shows the look for where its other half is, or `alone` once that one is gone; only while it shows one of those looks (Capsules' halves).
- **Another map: a next-piece box.** A second map of cells may have nothing on it (`<map name="next">` with its tile, position, `<columns>4</columns><rows>2</rows><cells />`). `<spawn object="pieces" into="next"><random /></spawn>` makes a piece on it (in its first cell unless told), and `<send object="pieces" from="next" into="board"><column>4</column><row>1</row></send>` moves the piece of that name on `next` (a shape with its cells) to that cell of `board` (left out: the cell it is in). A send with no piece to send, or no room for it, is refused. **A piece away from the map it was written on is only shown:** its timers do not count and its actions do not run, so the next shape neither falls nor answers the keys until it is sent home. One press reaches only the pieces that were there when it came, so a shape sent in by another's landing does not drop too.
- **`<checks>`.** `<full class="block">` is a row of the map whose every cell holds a piece of that class. Its commands run once for each full row, from the bottom up; in them `<clear />` takes that row's pieces away and `<collapse />` brings every piece above it down a row. `<line class="jewel"><atleast>3</atleast>` is every line of that many or more alike (of one group; a copy is of the group of what it copies), across, down or on a slant (`<straight />` after the `<atleast>`: across and down only), all at once; `<alike variable="hue" />` counts pieces as alike by that variable's value instead of their group (a germ and a capsule half of one colour); its commands run once for them all, and it is asked again on what they leave until none is found or nothing was taken. `<fall />` drops every loose piece of the map down its column onto what is under it, so lines the fall makes are found too: a chain. `<fall class="pill" />` drops only the pieces of that class; the others stay where they are and hold up what lands on them (Capsules' germs).

### The kit and from=

The `<kit>` holds objects, groups and maps written once, called **pieces**. A piece is never in play itself: it comes into play where an `<objects>` takes it, and as copies a [`<spawn>`](#spawn-and-despawn) makes.

```xml
<kit>
  <object name="paddle" class="paddle"> ... </object>
  <object name="bomb" class="bomb"> ... <most>6</most> </object>   <!-- only ever spawned -->
  <group name="robots" class="robot"> ... <member name="boss"> ... </member> ... </group>
</kit>
<objects>
  <object name="paddle1" from="paddle"><position><x>10</x></position></object>
  <object name="paddle2" from="paddle"><position><x>780</x></position></object>
</objects>
```

- **Taking a piece.** `from="piece"` in any `<objects>` takes it, under the name written, changed by what is written. An object or group with no `from=` that has a piece's name takes it as it is (`<object name="topwall" />`).
- A named member of a group piece can be taken alone, by its name: the group with that member only, so it keeps the group's class, rules and picture.
- A piece can be `from=` an earlier piece: Asteroids' medium and small rocks are each `from="rock"`.
- **Where it is taken says how long it lasts:** the game's, a state's or a scene's `<objects>`.
- **Only ever spawned.** A piece that a `<spawn>`, `<fire>` or `<release>` names, and nothing takes, is put beside what spawns it (the object whose rule, action or timer it is, or the state or scene whose input, condition or timer it is), as the description its copies are made from. So a copy belongs where its spawner does and reads that scene's variables (Kaboom's bombs fall at their wave's speed).

**The one change rule.** A take changes its piece, a state `from=` another changes the objects it copies, and a scene `from=` another changes that one's objects, all the same way. Over what it starts from, what is written:

| What is written | Does |
|---|---|
| attributes (its `class`) | replace those it has |
| a `<sprite>` or `<animation>` | replaces the one of the same name, whole, or is added |
| `<x>` or `<y>` of a position, velocity, acceleration or padding | replaces that half |
| a `<variable>`, `<action>`, `<timer>` | replaces the one of the same name, or is added (an unnamed timer is added) |
| `<collision>` rules | are added after those it has |
| `<solid>`s | replace all of them |
| `<member>`s, `<row>`s, `<column>`s, `<cell>`s | replace all of them |
| an inner group, or a map's piece, of the same name | is changed the same way, or added |
| anything else | replaces what it has of that tag |

An object or group written with a name the list already has changes it this way (Missile Command's later waves change the group `more` inside `missiles`); a changed member says only its sprites, animations, position and velocity.

A `from=` naming no piece, a take of a name the list already has, a map `from=` an object (or the other way round), two pieces of one name, a piece `from=` a later one, and a dotted name (`rocks.2`; give the member a `name` instead) stop the load.

The engine works all of this out on the XML before reading any object (`load/merge.cpp`), and a [generated program](#generating-a-program) does the same in XSLT (`merge.xsl`), step for step.

### Spawn and despawn

Objects can be made and taken away while the game runs, and **a copy is no different to any part of the engine from an object the file wrote.**

```xml
<action name="fire">
  <fire object="shot" />                 <!-- a new copy of the kit's shot, from the ship -->
  <play sound="zap" />                   <!-- only if a shot left: see refused presses -->
</action>
...
<object name="shot" class="shot">        <!-- in the kit -->
  <sprite> ... </sprite>
  <velocity><y>-8</y></velocity>
  <most>3</most>                         <!-- three in flight at most -->
  <collisions>
    <collision edge="all"><despawn /></collision>
    <collision class="rock"><despawn /></collision>
  </collisions>
</object>
```

- `<spawn object="shot">`, `<fire object="shot" />` and `<release object="shot">` each make a new copy, named for what it copies with the first number no copy has now (`shot.1`, `shot.2`, and `shot.1` again once that one is gone).
- A copy moves, touches, counts its own timers, holds its own variables and draws its own `<random>`s. Inside its own rules, `self` and its own name are the copy.
- **The description's name means every copy:** `<collision object="shot">`, a condition counting `object="shot"`, `<despawn object="shot" />`. A copy's own name can be written too (`<tether from="base" to="abm.1">` draws while there is an `abm.1`).
- **A group piece** spawns its members in turn (Asteroids' medium rocks in their three shapes), or with `<random />` one of them at random, and a member with a name is spawned by it.
- **`<most>`** caps how many copies of it are out at once; at the cap a spawn makes none, and, as for any [refused press](#a-press-that-cannot-be-done), the rest of the action or timer is not done.
- **`<despawn />`** takes an object out: a copy is gone when the frame ends (its name free again); one the file wrote stays out until a reset brings it back.
- A copy made during a frame joins the game when the frame is over, so it is first moved the frame after it is made.
- A full reset takes every copy away; `<reset object>` and `<reset scene>` take away the copies of what they reset. A scene's copies are away with their scene.
- Any object can be spawned, not only kit pieces: `<spawn object="ball">` makes a second ball.

Before spawning, a group of members with `<active>false</active>` was a **pool** that `<fire>`, `<release>` and `<reveal>` took from. Pools still work, and suit things that come back to where they were (Frostbite's igloo blocks, a door).

## Pictures

### Sprites

A `<sprite>` holds one shape. A `name` is needed only when an object has several (for an [animation](#animation) or as [looks](#looks)). A `<color>` left out is `color.white`.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | a filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | a filled rectangle |
| `<text>` | pieces (below), then `<size>`, `<color>`, `<font>`; `style=` | text |
| `<image>` | `<path>`, `<flip>` | an image file, loaded by the window backend |
| `<line>`s | each `<from>`, `<to>`, `<color>`, `<thickness>` | a drawing of straight lines ([lines](#lines)) |
| `<bitmap>` | `<row>`s of characters, `<scale>`, `<color>`s, `<flip>`; `style=` | a picture written as text ([bitmaps](#bitmaps)) |
| `<svg>` | `<path>`, a part (`<x>`, `<y>`, `<width>`, `<height>`), `<scale>`, `<hide>`s, `<flip>` | a drawing from an SVG file ([SVG pictures](#svg-pictures)) |
| `<tether from="a" to="b">` | `<thickness>`, `<color>` | a line from the middle of `a` to the middle of `b`, drawn again every frame wherever they are; in play only while both ends are (a fishing line, a missile's trail) |

**Pictures the engine draws.** Lines, bitmaps and SVGs all go one way, once, when the game loads (`buildPicture`, `load/expr.cpp`): the engine reads what the file describes, draws it into its own `Bitmap`, flips it if asked, and keeps it to turn for an object with a `<heading>`. No window backend draws them itself, so every library shows the same pixels; the object's size is known with no window (`name.width` works in tests); and a `pixel` [collision](#how-a-touch-is-found) tests exactly what is drawn. Cells of a group drawing the same picture share it. Text and `<image>` files are the exception: only a backend can measure them.

**Turning.** An object with a `<heading>` shows its picture turned to the heading, to the nearest whole degree, made again only when the heading moves a degree. Lines are drawn again turned, so they stay sharp; a bitmap or SVG has its finished pixels turned, each taking the one pixel under it, so chunky pixels stay chunky. Every heading comes out as the same square, big enough for any angle, which is the object's size.

#### Turning about a pivot

A `<pivot>` names the point of the drawing the object turns about. The picture is still turned about its middle, and the object is moved so that the pivot stays where it was: Pinball's flippers are a `<line>` with the pivot at the hinge end, their `<position>` the hinge on the table.

```xml
<object name="leftflipper" from="flipper">
  <position><x>141</x><y>539</y></position>
  <heading>30</heading>
  <pivot><x>0</x><y>0</y></pivot>
  <actions>
    <action name="flip"><swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing></action>
  </actions>
</object>
```

- **`<swing>`** (in an action, its key held) turns the heading `<degrees>` its `direction=` (as `<turn>`: `clockwise` adds to the heading, `counterclockwise` takes from it) in one stroke `<seconds>` long; let go, it comes back the same way to the heading it started with. Pressed again on the way back, it turns round where it is.
- **The stroke is eased, as half a wave:** it starts from still, speeds up to its fastest half way, and slows to a stop at the end, as a flipper has to get its weight moving and its coil pulls less at the end. Where it is, how fast it turns and how that changes all come from that one curve. Pinball's 60 degrees in 0.1 seconds (six frames) turns 4, 11, 15, 15, 11 and 4 degrees, so a ball struck half way goes off hardest.
- **The turn is swept.** What a swing turns in a frame is that frame's spin: the move goes from the old heading to the new one through the frame, alongside every other object's move, so a touch is found where the flipper's wedge first meets the ball's path, however fast either goes ([how a touch is found](#how-a-touch-is-found)). A heading changed any other way (a `<turn>`, a `<set>`) is not swept.
- **A turning thing lends the speed of the point that touches.** A `<knock>` takes each side's speed at the touch point: its velocity, and for one turning, its spin times how far that point is from the pivot, across the arm. A ball struck by a flipper's tip goes off faster than one struck by the hinge, and off the way the flipper faces there.

### Lines

```xml
<sprite>
  <line><from><x>10</x><y>2</y></from><to><x>20</x><y>2</y></to></line>
  <line><from><x>20</x><y>2</y></from><to><x>25</x><y>7</y></to><color>color.red</color><thickness>2</thickness></line>
</sprite>
```

- Points are in pixels from the sprite's top left, so never negative. `<thickness>` is at least 1 (1 when left out).
- The lines are drawn in the order written, with no gaps, into a picture as big as the furthest endpoint plus the thickness; everything not drawn on is clear.
- A line thicker than 1 is every pixel within half its thickness of it: a bar of that width at any angle, with round ends, sitting in the square of its thickness down and to the right of each end. So a thick line turned is the same shape turned, which is what a flipper touches with.
- Lines cannot be mixed with another shape and have no fill. Lunar Lander's moon is one object of fifteen lines, its pads single thick lines.

### Bitmaps

```xml
<sprite>
  <bitmap>
    <row>..*...*..</row>
    <row>..*****..</row>
    <row>.**w*w**.</row>
    <row>*********</row>
    <scale>5</scale>
    <color>color.green</color>             <!-- the * pixels -->
    <color char="w">color.white</color>    <!-- the w pixels -->
  </bitmap>
</sprite>
```

- One character to a pixel: `.` is clear, `*` is drawn in the plain `<color>`, and each `<color char="x">` names one more character and its color (a ghost's body, white eyes and blue pupils).
- Every character becomes a block of `<scale>` by `<scale>` pixels (a whole number, 1 when left out), so the picture above is 45 by 20.
- The rows must be as long as one another and hold only `.`, `*` and the named characters; anything else stops the load naming the row and character (`row 3 of a bitmap has 'o' as character 2`).
- The scale and colors can come from a [style](#styles) (`<bitmap style="mario">`); a sprite changing a group's gives only the colors it changes.
- `<flip>horizontal</flip>` or `vertical` mirrors the pixels; the size stays.

### SVG pictures

```xml
<sprite>
  <svg>
    <path>assets/Space Invaders Color Sprites.svg</path>
    <x>96</x> <y>64</y> <width>32</width> <height>32</height>
    <scale>2</scale>
    <hide>backdrop</hide>
  </svg>
</sprite>
```

- `<path>` is the file, found as an image's is. `<x>`, `<y>`, `<width>` and `<height>` (all four or none) pick a part of the drawing, so one sheet serves many objects. `<scale>` is pixels per unit of the drawing (it need not be whole). Each `<hide>` is the id of an element to leave out.
- The drawing keeps its own colors, antialiased, so there is no `<color>`; a pixel counts as solid for a `pixel` collision if anything at all is drawn on it.
- It is drawn by the engine with lunasvg (only `picture/svg.cpp` includes it), so no window backend loads an SVG. A missing file, a part outside the drawing, or a scale of 0 or less stops the load.

### Text

A `<text>` is made of pieces, in any order, then its `<size>`, `<color>` and `<font>` (or a `style=` for those it leaves out):

| Piece | Shows |
|---|---|
| `<content>PONG</content>` | the words as written |
| `<number>paddle1.score</number>` | a number. When the value is one `owner.variable` it is live and redraws when the variable changes; anything else is worked out once |
| `<letter>entry.first</letter>` | a number as a letter, 0 is A and 25 is Z, round again either way: initials are three variables an `<inc>` and `<dec>` change |
| `<choose><by>value</by><word>LOVE</word><word>15</word>...</choose>` | one of its words, the first for 0 (a fraction goes down, past either end the end word), live like a number: Tennis says its points in words |

```xml
<text style="hud"><content>SCORE</content><number>ship.score</number></text>   <!-- SCORE 120 -->
```

- Pieces are joined with a space, except that letters side by side make one word (`ABC 120`).
- In a group, a piece naming the group's own variable (`table.score` in the group `table`) shows each member's own.
- A text whose look changes, or whose width changes (a score gaining a digit), is measured again, so a position worked out from its width keeps it centred.
- Text is drawn with the font its style or `<font>` names, else `assets/tuffy.ttf`, unsmoothed, so a pixel font stays pixels in a scaled window. If a font file is missing, each backend says so once and draws with the 8 by 8 font built into the program (chunky, monospaced, about `<size>` / 16 blocks tall).

### Animation

```xml
<object name="crab">
  <sprite name="open"> <bitmap> ... </bitmap> </sprite>
  <sprite name="closed"> <bitmap> ... </bitmap> </sprite>
  <animation>
    <interval>1</interval>
    <frame sprite="open" />
    <frame sprite="closed" />
  </animation>
  ...
</object>
```

- `<interval>` is how many **seconds** each picture shows (a value above 0); each `<frame>` picks a sprite by name. There are at least two frames, and a sprite may come up more than once.
- Frames are pictures (bitmaps, SVGs or lines), all the same size. An unnamed animation must show every sprite the object has.
- Seconds become frames of the game with the `<framerate>`, so an animation, like everything else, counts frames: a slow machine animates slowly.
- It moves on only while its object is in play, so a pause holds it; a reset or coming back into play starts it from the first picture. The cells of a group start together, so a block of aliens flaps as one.
- In a group, a member, row, column or cell giving sprites changes the group's of those names, and the animation is looked up among the sprites it ends up with (Space Invaders' rows of different aliens in one block).

### Looks

An object with several named sprites and no unnamed animation has **looks**: it starts as the first, and `<become sprite="name" />` shows another from then on; a reset brings back the first.

- `<become sprite="blue" object="ghosts" />` changes every object of that name or group; with `from="normal"` only those showing `normal` (a power pellet turns the ghosts blue but leaves an eaten one's eyes as they are).
- A collision rule with `sprite="white"` runs only while its object shows that look, so **a look is a simple state the player can see.** Frostbite's floe:

  ```xml
  <collision object="bailey" sprite="white">
    <become object="row1" sprite="blue" />
    <inc variable="bailey.score">10</inc>
  </collision>
  ```

- **A look can count hits.** A brick of two looks, `whole` and `cracked`, has `<collision object="ball" sprite="cracked"><die /></collision>` and then `<collision object="ball" sprite="whole"><become sprite="cracked" /></collision>`. Each rule asks for the look its object had **as the touch happened**, not one an earlier rule for the same touch gave it, so the order does not matter, and two rules can turn a look back and forth.
- **A look tells the next state something.** One `wins` state whose `winner` has a look per player: the condition that ends the game does `<become object="winner" sprite="player2" />` and then pushes it.
- A look is any shape kind, and cannot be a text showing a `<number>` or `<letter>`. With a `<heading>` every look (each frame of a moving one too) is kept to turn, so the look showing is drawn at the heading: Asteroids' ship has `coasting` and a flickering `burning`.

**Looks that move.** A named `<animation name="walking">` is a look of its own, picked by name like a sprite (`<become sprite="walking" />`, a pose, a take). The looks are the named animations in the order written, then the sprites they leave out, and the object starts as the first (Coily's `egg` animation is listed before his `snake`). Animations are all named or none.

### Poses, mirror, grip and blinking

Three parts, after `<facing>`, give a walking, jumping thing its look and feel without a rule for each:

```xml
<poses>
  <standing sprite="stand" />
  <walking sprite="walking" />
  <skidding sprite="skid" />
  <rising sprite="rising" />
  <falling sprite="falling" />
  <thrusting sprite="burning" />
</poses>
<mirror>left</mirror>
<grip>0.15</grip>
```

- **`<poses>`** names the look for each way of moving, and the engine picks it every frame. `standing` is needed; walking falls back to standing, skidding to walking, falling to standing, rising to falling, thrusting to standing. While a `<thrust>` is held it is thrusting; in the air it is rising, then falling; on the ground standing when still, skidding when its grip has it sliding against its keys, walking otherwise. Poses are picked only while a pose look is showing, so a `<become>` to another look (Mario's `dead`) holds until a `<become>` puts a pose back.
- **`<mirror>left</mirror>`** (or `right`) is the way its pictures do not face: going that way they are drawn flipped. The way is the move key held, else a jump's way across, else its velocity; stopping keeps the last. Not with a `<heading>` or a `<hole>`.
- **`<grip>`** (above 0 up to 1, 1 when left out) is how quickly its speed across follows its keys: each frame it goes that share of the way to what the keys ask, so Mario's `0.15` takes a moment to get going and slides. Up and down too when nothing pulls it. It also slows an object with no keys held, so give it only to something a player steers.
- **Blinking.** `<set variable="mario.blink">2</set>` makes an object blink for two seconds, counting down by itself while in play. It is a live number, so a rule can leave a blinking player alone (`<condition object="mario" variable="blink"><atmost>0</atmost></condition>`). A reset ends it.

### Light and dark

What is `<dark>` is drawn only where there is light. Something in play with a `<light>` lights everything within that many pixels of its middle (any part of a thing's box counts).

- `<dark>unlit</dark>` is drawn only while some light reaches it; `<dark>unseen</dark>` from the first time one does until it is reset, so a map fills in as it is explored.
- Darkness changes only what is drawn: a dark thing moves, touches and runs its rules as ever.
- The light is a live number, so a `<set>` strikes a match and a timer burns it down. Haunted House:

```xml
<action name="strike"><set variable="player.light">match</set></action>
...
<timer><every>0.1</every><set variable="player.light">max(near, player.light - 5)</set></timer>
```

### Height and shadows

An object with an `<altitude>` is in the air over where it is: its position is the point on the ground under it, which is what it touches, and it is drawn raised by its altitude.

```xml
<object name="shadow"> ... <shadow object="ball" /></object>      <!-- written first, drawn under -->
<object name="ball">
  ...
  <altitude><start>80</start><fall>0.25</fall><bounce>0.7</bounce></altitude>
  <collisions>
    <collision object="player">
      <condition variable="altitude"><below>reach</below></condition>
      <set variable="ball.rise">loft</set>
    </collision>
  </collisions>
</object>
```

- `<start>`, `<rise>` (how fast it goes up), `<fall>` (how much less the rise is each frame) and `<bounce>` (0 to 1, how much of its speed it comes back up with) are each optional, 0 when left out. It never goes below the ground, and comes to rest once a bounce would be less than one frame's fall.
- `name.altitude` and `name.rise` are live numbers: `<set>` hits a ball up, a rule's `<condition>` lets a high one go past.
- `<shadow object="ball" />` keeps an object under another, middle to middle, in play while it is.

## Sounds

Sounds are written as notes, the bleeps and tunes of an 8-bit sound chip; nothing is loaded from a sound file. Each `<sound name>` is something a [`<play>`](#look-and-sound) or a state's [`<music>`](#states) starts.

```xml
<sounds>
  <sound name="wall" wave="square">
    <note pitch="A3">0.04</note>
  </sound>
  <sound name="gameover" wave="square">
    <volume>0.4</volume>
    <note pitch="G4">0.2</note>
    <note pitch="E4" to="D#4">0.5</note>       <!-- slides down -->
    <rest>0.1</rest>
    <note pitch="2000" wave="noise">0.15</note>
  </sound>
  <sound name="thrust" wave="noise">
    <loop />                                   <!-- over and over until silenced -->
    <note pitch="150">1</note>
  </sound>
  <sound name="start">
    <part wave="square"> <note pitch="C5">0.1</note> ... </part>   <!-- a tune -->
    <part wave="triangle"> <note pitch="C3">0.2</note> ... </part> <!-- over its bass -->
  </sound>
</sounds>
```

| Part | Meaning |
|---|---|
| `wave=` | `square` (default), `triangle`, `sawtooth`, `sine` or `noise` (a new random level every half cycle: a high pitch hisses, a low one rumbles). On a sound, a part or a note; the nearest wins |
| `<loop />` | first: the sound plays over and over (below) |
| `<volume>` | a value, 0 (silent) to 1; 0.3 when left out |
| `<note pitch to wave>seconds</note>` | a tone. `pitch` is a note and octave (`C4` middle C, `A4` 440 Hz, `F#3`, `Bb5`) or hertz (`440`); `to` slides to another pitch evenly in semitones. The length is a value, more than 0 and at most 10 |
| `<rest>seconds</rest>` | silence |
| `<part wave>` | a voice of its own: a sound in parts plays them all at once, as long as its longest. Parts add up, so keep the volume down. A sound has parts or loose notes, not both |

- **Loops.** A `<play>` of a looping sound switches it on until a `<silence sound>`, the reset of the game, or, when the `<play>` is on a key (or in the action a key runs), until that key is let go: Asteroids' engine hums while thrust is held. A state change lets go of the held keys, so a pause is quiet.
- **One voice per sound**, like a chip's channel: playing a sound that is still playing starts it again; different sounds play over each other; a sound asked for twice in one frame plays once.
- Every sound is made into samples (16 bit, one channel, 44100 a second) by the engine when the window opens, the same for every sound library, with a fade of a couple of milliseconds so notes do not click.
- An unusable wave, pitch or length, a volume outside 0 to 1, a loop after anything, a sound with no notes, two sounds of one name, and a `<play>`, `<silence>` or `<music>` naming no sound stop the load (`sound 'wall' > <note> 1: pitch="H2" is not a pitch; ...`).

## Commands

A command is a tag in a list: a collision rule, an action, a key's `<input>`, a condition, a timer, or a path's step. A list runs in the order written. Most verbs work in all of them.

### Where a command can go

Commands are sorted by what they need, and each place takes only the groups that mean something there. A command anywhere else is a schema error, and, for a game read without the schema, a load error listing what can go there (`ruleFor` in `load/check.cpp`, built from the same groups as the schema).

| Group | Commands | Needs |
|---|---|---|
| changes | `inc`, `dec`, `set`, `copy`, `rank`, `play`, `silence`, `become`, `reveal`, `follow`, `shift`, `enter`, `reset`, `spawn`, `despawn object=`, `trigger` | nothing: they name what they change |
| flow | `push`, `pop`, `turns`, `pass`, `out`, `demo` | the game's running order |
| self | `die`, `drop`, `move`, `release`, `reverse`, `stop`, `fire`, `despawn` | an object running it |
| steering | `aim`, `chase`, `hunt`, `wander` | an object to steer |
| touch | `carry`, `deflect`, `hole`, `knock`, `land`, `ride` | another object touched |
| edge | `stick`, `wrap` | a screen edge |
| bounce | `bounce` | a screen edge or another object |
| controls | `accelerate`, `climb`, `go`, `hop`, `jump`, `leap`, `thrust`, `turn`, `track` | a key held or pressed |
| press | `hold` | a [computer player](#computer-players) |

| Place | Takes |
|---|---|
| a `<collision>` with another object | changes, self, steering, touch, bounce |
| a `<collision edge>` | changes, self, edge, bounce |
| an object's `<action>` | changes, self, steering, controls |
| an object's `<timer>`, a path's `<step>` | changes, self, steering |
| a state's or scene's `<input>`, `<condition>` and `<timer>` | changes, flow |
| a `<computer>`'s `<condition>` | press |

The schema cannot tell the two kinds of `<collision>` apart (that is in the attributes), so it lets a rule hold either's commands and the loader holds each to its own.

### A press that cannot be done

In an object's `<action>`, its `<timer>`s and path steps, a collision rule (about an edge or another object), a state's key and a condition, **a verb that is refused ends the list there**: what is written after it is not done either. A collision rule's later rules still run; only the rest of the one rule is dropped. Refused are a `<fire>` or `<spawn>` with every copy already out, a `<hop>`, `<jump>` or hunt by hops in the air or landing outside the window, a `<leap>` off the ground, and an `<aim>` with `<lined>` that is not lined up. What is written before it has run.

So the verb goes first and its answers after: `<fire object="bullet" /><play sound="shoot" />` sounds only when a shot leaves. Collision rules and a state's commands run every command as before.

An object out of play (hidden or dead) does nothing when its action is pressed or triggered; letting go still lets go.

### Moving

| Command | Does |
|---|---|
| `<move direction="up">step</move>` (`down`, `left`, `right`) | in an action, held: while the key is down the object goes `step` pixels a frame that way (see [held keys](#held-keys-and-state-changes)). In a rule: shifts it once (and everything in lockstep with it) |
| `<hop direction="up">distance</hop>` | in an action, once a press: a step of `distance` pixels, made before collisions, so it is judged where it lands; refused if it would leave the window. Also the diagonals `up-left`, `up-right`, `down-left`, `down-right`, which go `distance` both ways or `<x>` across and `<y>` up or down (Q*bert's `<hop direction="down-right"><x>32</x><y>48</y></hop>`) |
| `<seconds>` and `<arc>` in a `<hop>` or `<jump>` | `<seconds>`: it takes that long, through the air, touching nothing until it lands; another asked for in the air does nothing. `<arc>` after it (pixels): its [altitude](#height-and-shadows) climbs to that height halfway and back, so it is drawn going up and over |
| `<jump direction="down">90<seconds>0.35</seconds></jump>` | a timed jump of its content's distance, in a straight line (0.3 seconds when left out); not made if it would land off the screen. Frostbite's jump between rows |
| `<leap>height</leap>` | a jump up under its own pull: it leaves the ground fast enough to rise about `height` pixels against its `<acceleration>`, and falls until a `<land />`. Only from the ground and not on a ladder; the way across is fixed until it lands. Needs an acceleration down and a `<land />` rule. Donkey Kong |
| `<land />` | in a rule about another object: coming down onto its top, the object stands on it, its fall stopped, until the next frame's landing. Any other side, or going up, does nothing, so a platform can be jumped up through and walked off at its ends |
| `<climb direction="up" class="ladder">step</climb>` (or `down`) | held: while the object stands at an object of that class (its middle over it, its feet between that one's top and bottom) it gets on, lined up, and climbs. Nothing pulls it on a ladder, and the step is its velocity, so it touches whatever it passes (a barrel on the same ladder, the girders it climbs through) as any moving object does; `<land />` does nothing while it climbs. At the top or bottom it steps off, onto what it lands on there; at an end with nothing it lands on (a class or object one of its `<land />` rules names, its top there, under any of it), a held climb stays on, stopped (the top of Donkey Kong's broken ladders). Pieces of the class stacked one on another, as wide and lined up, are one ladder (Donkey Kong's are columns of rungs). Run by a rule, a timer or a condition rather than a key, it takes the object along the next ladder it is at and then lets go: a barrel goes down one ladder, not every ladder after |
| `<accelerate direction="up" burn="fuel">amount</accelerate>` | held: adds `amount` to the velocity every frame (opposite ones cancel). `burn` names one of the object's own variables, taken 1 off each frame; at 0 the thrust does nothing |
| `<thrust burn="fuel">amount</thrust>` | held: the same along the object's `<heading>` |
| `<turn direction="counterclockwise">degrees</turn>` (or `clockwise`) | held: turns the heading that many degrees a frame |
| `<swing direction="counterclockwise"><degrees>60</degrees><seconds>0.1</seconds></swing>` | held: turns the heading that many degrees that way in one eased stroke that long (from still, fastest half way, to still); let go, back to where it started the same way. The turn is swept, and lends speed to what it knocks ([turning about a pivot](#turning-about-a-pivot)). Pinball's flippers |
| `<reverse />` | the velocity turns round, both ways at once (Kaboom's bomber on a timer) |
| `<stop />` | comes to rest and stays: velocity 0, and no longer pulled by its acceleration or pushed by a held key, until a reset. Since the pull goes too, it is no landing for something that can be knocked up again |
| `<track />`, `<track path="rail" />` | in an action run by a [pointing control](#pointing-controls): the object goes where it points; with `path=`, its middle goes that far along the path's legs from its `<start>` |

### Steering

| Command | Does |
|---|---|
| `<chase object="hero">1.5<near>100</near></chase>` | heads straight for the middle of the nearest one in play of that name or group, at its content's speed, facing that way (a `<heading>` turns too); within `<near>` pixels it stops. It acts once, so a timer repeats it, and how often is how quickly it turns. Evil Otto, Missile Command's missiles |
| `<aim object="man" />` | turns to the nearest one in play: its next `<fire>`s go straight at where that one's middle was, from the shooter's middle, at the projectile's own speed. Kept until it aims again, is reset, or a key moves it. With `<lined>8</lined>` it aims only along one of eight ways, when the target lies within that many pixels of one, and is refused otherwise (Berserk's robots) |
| `<follow path="dive" />`, `<follow path="in" object="aliens"><stagger>0.15</stagger></follow>` | flies a [path](#paths): the object itself, or every one of that name or group in play, each `stagger` seconds after the one before. `<nearest>2</nearest>` sends only the two nearest the one doing it (a flagship's escorts). Without `object=` only in an object's own places |
| `<shift object="aliens"><x>sway.way</x></shift>` | moves those objects, and the places they go [home](#paths) to, by `<x>` across and `<y>` down (either may be left out); one on a path only has its place moved. A reset puts them back. Galaxian's formation sways with it |
| `<go direction="left">speed</go>` | a [maze walker](#walking-a-map), on a key: turns that way at the first tile it can, carrying on until then; stops in the middle of a tile at a wall; turns straight round at once |
| `<hunt object="pacman">2<ahead>4</ahead><with object="blinky" /><retreat object="corner">8</retreat></hunt>` | a maze walker, from now on: at every tile, of the open ways other than back, the one whose next tile is nearest a target, as the arcade's ghosts chase. The target is the nearest one in play of that name; `<ahead>` tiles in front of the way it faces; `<with>`, as far past that point again as the partner is short of it; `<retreat>`, while within that many tiles, the corner object instead. A tie goes up, left, down, right |
| `<hunt object="qbert"><hop><x>32</x><y>48</y><seconds>0.3</seconds></hop></hunt>` | a hunt by hops, for an object that walks no maze: of the four hops it could make, the one landing nearest the target and staying in the window. Q*bert's Coily, on a timer |
| `<wander>1.25</wander>` | a maze walker, from now on: an open way at random at every tile, never back (a frightened ghost) |

### Touching

| Command | Does |
|---|---|
| `<bounce />`, `<bounce>0.8</bounce>` | off what it touched, as off a surface standing still: the velocity is mirrored about the way the surface faces there, keeping that share of the speed it came in at (1 when left out; above 1 it comes off faster, a bumper). A screen edge and a box's side face straight out, so off them it is the plain flip; a circle faces out from its middle, and a `pixel` picture the way it is drawn there, so a ball comes off a slope drawn as a `<line>` at its angle. One already going away keeps on. A surface that moves and lends its speed is a [`<knock>`](#knocks) |
| `<deflect>45</deflect>` | off another object at an angle set by where it hit: straight out from the middle of the side touched, up to that many degrees toward either end, in proportion. The speed is kept. Pong's ball |
| `<knock />`, `<knock>0.5</knock>` | the two meet as things with weight do ([knocks](#knocks)); the content, 0 to 1, is how bouncy |
| `<stick />` | against a screen edge: kept inside it, and only the velocity into that edge stops, so it still slides along it. In a rule about another object, only the velocity into the side touched stops, so it slides along what it met (Centipede's wand under the zone's line and against mushrooms) |
| `<wrap />` | against a screen edge: once wholly off the screen and still going that way, put back in from the opposite edge, a window's width (or height) plus its own size along, so a lane keeps its spacing |
| `<through />` | in a rule for a refused step on a map with [cells](#cells): the step is made after all, into the cell with what is there, which stays; the two share it until one moves on (Centipede turning down onto a mushroom or another centipede) |
| `<ride />` | lent the touched object's velocity for that frame, on top of its own: a frog on a log. Worked out again every frame, so stepping off needs nothing |
| `<carry />` | picks up the object touched and takes it along, held where it was picked up, until a `<drop />`. The carried thing is swept on the way and meets what it reaches (a key opening a gate). One at a time: picking up another puts the first down; either out of play or reset, and it is put down. Adventure's hero |
| `<drop />` | puts down what the object carries; it is not picked up again until the two stop touching |
| `<hole>4<rough>0.5</rough></hole>` | cuts a round hole of that radius in the object's own picture where the other struck it; what is cut is not drawn and, with `pixel` collisions, not touched. `<rough>` (0 to 1) leaves a ragged edge. Only for lines or a bitmap with no heading, animation, mirror or second look. A reset mends it. Space Invaders' shields |

### In and out of play

| Command | Does |
|---|---|
| `<die />` | takes the object out of play: not drawn, not touching, not moving, until a `<reveal>` or a reset brings it back. It stays in the game (to take a copy away for good, `<despawn />`) |
| `<reset />` | in an object's place: back as it started, its position, velocity, heading and variables (with their `<random>`s drawn again). In a state's place: **a new game**: every object back, the copies gone, every scene started again, the state stack down to the first state, except what is [kept](#keeps) |
| `<reset object="name" />` | resets that object, or every member of a group, the same way, **every variable included**, and takes away copies made of it. So never reset the player from someone else's rule: keep score and lives on an object of their own |
| `<reset scene="room1" />`, `<reset way="out" />` | puts a [scene](#scenes) back as it started, objects and timers; `way=` names the scene the current scene's way leads to. Whether it is current does not change |
| `<enter scene="room2" />`, `<enter way="east" />` | makes that scene the current one of its state; the scene left stays as it was. `way=` enters where the current scene's way leads |
| `<reveal object="blocks">count</reveal>` | brings back the first `count` (1 when left out) of that name that are out of play, each where it started |
| `<release object="debris">count</release>` | in a rule: puts `count` of that name into play centred on the object running it: new copies of a kit piece, or the first out of play of a pool. How a rock breaks |
| `<fire object="shot" />` | launches the projectile: a new copy of a kit piece (as many in flight as its `<most>`), or the first of a pool out of play. From a shooter that has aimed, along the aim; with a `<facing>`, from the middle of that side, that way (a diagonal when two keys are held); with a `<heading>`, from its nose along the heading; else from its top centre with the shot's own velocity |
| `<spawn object="ball">` with `<position>`, `<velocity>` | makes a new copy, placed (its top left, or the middle of a `<centered>` half) and going as they say, read as it runs: `<x><centered>self.x + 5</centered></x>`. A half left out is as the file says |
| `<despawn />`, `<despawn object="bomb" />` | takes the object, or every copy of that name and the object itself, out of the game ([spawn and despawn](#spawn-and-despawn)). Kaboom's bomb hitting the ground takes the whole wave's bombs |

### Numbers

| Command | Does |
|---|---|
| `<inc variable="ship.score" />`, `<inc variable="ship.score">self.worth</inc>` | adds 1, or the amount, and redraws any text showing it |
| `<dec variable="frog.lives" />` | takes 1, or the amount, off; it may go below 0, which `<atmost>0</atmost>` notices |
| `<set variable="target">value</set>` | puts the value in: an object's variable, or one of its live numbers (`owner.x`, `owner.y`, `owner.velocity.x`, `owner.velocity.y`, `owner.heading` for one with a heading, `owner.altitude`, `owner.rise`, `owner.light`, `owner.blink`). A group's name sets every member |
| `<copy variable="ship.score" into="best.score" />` | the second takes the number the first holds now |
| `<rank variable="entry.score" into="table" />` | the entry takes its place in a [table of the best](#a-table-of-the-best) |

The variable named must belong to an object; one no object has stops the load, listing the object's variables.

### Look and sound

| Command | Does |
|---|---|
| `<become sprite="blue" />` | the object shows that [look](#looks); timers that follow the look start over. `object=` changes every object of that name or group (out of play too); `from="normal"` only those showing `normal`; `style="angry"` puts on that style's colors, a [palette swap](#palette-swap) |
| `<play sound="zap" />` | asks for a [sound](#sounds), played once the frame's work is done. In an action or input it plays on the press; a loop plays while the key is held |
| `<silence sound="thrust" />` | stops that sound if it plays |

### Flow

| Command | Does |
|---|---|
| `<push state="paused" />` | puts that state over the current one |
| `<pop />` | back to the state below; with only the first state left it does nothing |
| `<pop state="result" />` | this state goes and that one takes its place, so moving on does not grow the stack |
| `<trigger object="ship" action="fire" />` | runs one of an object's actions, as one press of its key (a `<move>` in it stays on). `object=` naming a group runs it on each member, each one's refused verbs stopping only its own run. `from="normal"` runs it only on those showing that [look](#looks): Pac-Man's mode object sends `scatter` and `chase` to the normal ghosts, leaving the blue ones and the eyes alone, and a ghost normal again asks the mode, `<trigger object="mode" action="chase" from="chase" />`. In an object's own rules, actions and timers `object=` may be left out (itself): one death written once as an action and triggered by each rule that kills. Actions that trigger one another in a circle stop the load |
| `<turns>`, `<pass />`, `<out />` | players [taking turns](#taking-turns) |
| `<demo />` | plays the game's recorded `<demo>`, the [attract mode](#attract-mode) |
| `<hold button="l" />` | a computer player holds that key while its condition holds |

### Mistakes the loader catches

A game that would play wrong stops loading, with a message that says where and, for a likely typo, what was meant (`(did you mean 'paused'?)`). Names are checked once everything is built, so a name used before the thing it names is fine:

- an unknown command, or one missing an attribute it needs (`object 'ball' > <collisions> > <collision>: unknown command <explode>`);
- a state, object, action, projectile, sound, look, scene or way that does not exist;
- a key the engine does not have (`there is no key named 'spcae' (did you mean 'space'?)`);
- a color that is not a `color.` name;
- a command in a place where it does nothing (a `<move>` in an `<input>`, which needs a `<trigger>` of an action), listing what can go there;
- an `<inc>` or `<dec>` of a variable no object has.

### Colors

`color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`, and the muted `color.grey`, `color.darkgrey`, `color.lightgrey`, `color.brown`, `color.orange`, `color.purple`, `color.darkblue`, `color.darkgreen`, `color.forestgreen` (`picture/color.cpp`).

## Collisions

`<collisions>` says how an object touches, then its rules:

```xml
<collisions>
  <type>pixel</type>                      <!-- optional: box (default) or pixel -->
  <solid char="*" />                      <!-- optional, pixel only: which bitmap characters touch -->
  <collision edge="vertical"><bounce /></collision>
  <collision class="bricks"><deflect>30</deflect></collision>
  <collision object="paddle" side="bottom"> ... </collision>
</collisions>
```

In order: `<enabled>` (`false` turns it off), `<lockstep>`, `<type>`, `<solid>`s, a `<hitbox>`, then the `<collision>` rules. Only objects in play with collisions on take part.

### Rules

- **Against a screen edge:** `edge="left"`, `right`, `top`, `bottom`, or `vertical` (top and bottom), `horizontal` (left and right), `all`. Several rules for one edge all run, in order. Edges are checked by position, before the frame's move.
- **Against another object:** `class="..."` (a kind) and/or `object="..."` (one object, a group, a copy's description), both meaning AND; a `<collision>` with neither is about anything. An edge rule with `class=` or `object=` stops the load.
- **Every matching rule runs**, in the order written, and each sees the object as the rule before left it (a changed look, a stopped speed).
- **What a rule can do** is what its place takes ([where commands go](#where-a-command-can-go)). A `<trigger>` with `object=` left out runs an action of the object itself.

### Rule filters

A rule about another object can be narrowed; most filters work on edge rules too.

| Filter | Runs only |
|---|---|
| `side="top"` (`bottom`, `left`, `right`, `horizontal`, `vertical`) | when touched on that side of its own object: a ball falling onto a block is touched on its bottom; Mario's head bumping a floor is his top |
| `sprite="cracked"` (edge rules too) | while its object shows that [look](#looks), as the touch happens: taken once before any rule runs, so a look one rule gives does not start another rule for the same touch |
| `unless="logs"` (edge rules too) | when the object is not, at that moment, also touching something in play of that class. Frogger's river: `<collision class="water" unless="logs">`. On an edge: `<collision edge="bottom" unless="paddle">` |
| `through="floor"` | for a touch felt through something of that class ([knocks](#knocks)); a rule without it, only for a touch itself |
| `<slower>N</slower>`, `<faster>N</faster>` (first inside the rule) | while the object's speed is under N, or N or more, as the touch happens, taken once before any rule runs: a landing and a crash on one pad. Not on edge rules |
| `<condition>`s, `<all>`, `<any>` (after those) | while every one holds, written as in a [condition](#conditions). One with no `class=` or `object=` is about the object itself, and `self` in it is that object (each member, on a group). Not on edge rules yet |

### How a touch is found

- **Swept.** Each object moves along its path for the frame, and the detector finds the moment two of them first touch (boxes as boxes, circles as circles, a circle against a box with rounded corners), so a small fast object cannot pass through a thin one. The earliest touch in the whole frame is handled first: everything moves to that moment, the pair's rules run, and the rest of the frame plays with the velocities they left. Each pair reacts at most once a frame.
- **Who is looked at.** The pairs are those a rule answers for, worked out once at load. A pair is skipped unless one of the two has moved: it has a velocity or rides something, or it is not where the last frame left it, whatever put it there (a hop or a landing, a `<set>` of its `x`, a `<shift>`, a teleport, a spawn). Being loaded or reset puts an object in its place, which is not a move. An object in the middle of a jump is in no pair. Something a rule brings into play touches nothing until the next frame.
- **What a touch is.** Worked out once, before either side's rules (`contactOf`, `play/geometry.h`): the edge of each that was touched, the point where they meet, the way straight apart, where along each one's side, how deep, and how far through the frame. Each side's rules see it their way, as `side=`, `touch.*`, `<bounce>` and `<knock>` read it.
- **`<type>pixel</type>`.** The pair is first swept as boxes, then walked half a pixel at a time from the moment the boxes touch until a drawn pixel of one lies on a drawn pixel of the other. A pixel object is a picture the engine drew (lines, a bitmap, an SVG); a circle or rectangle is solid as itself; text and images cannot be `pixel`. The edge reported is the one the motion came in through. How deep they are is how far the first must go along the way apart for no drawn pixels to meet (not how far their boxes overlap), which is how far a `<knock>` parts them. The point is the middle of the pixels where they meet, and the way apart is how what is drawn lies there (`pixelContactAt`): of the solid pixels of each within four pixels of the point, the one lying flattest across the point is the surface (a drawn line, a wall's side), facing away from the other; where neither does (a corner, the end of a line) it is from the middle of the one's pixels there towards the other's.
- **Turning.** For a pair of which one turns this frame (a [`<swing>`](#turning-about-a-pivot)), both are walked through the frame together, the turning one turned about its pivot and drawn as it would be shown at each moment (the nearest whole degree), half a pixel of the furthest any point of either goes at a time, until they touch; the moment is then narrowed down. The engine never runs a frame in smaller steps: only the moment of the touch is looked for.
- **`<solid char="*" />`**, after `<type>pixel</type>`: only those characters of the object's bitmap touch; the others are drawn but touch nothing. Q*bert's cubes touch by their tops.
- **`<hitbox>`**: the shape that touches other objects instead of the picture, a `<rectangle>` or `<circle>`, then a `<position>` of its top left from the picture's. Q*bert touches with his feet alone. The window's edges, `<land />`, `<stick />` and a carry still use the picture. Not with `pixel`.

```xml
<hitbox>
  <rectangle><width>12</width><height>6</height></rectangle>
  <position><x>6</x><y>18</y></position>
</hitbox>
```

### Lockstep

`<lockstep>true</lockstep>` in a group's `<collisions>` makes the members of each innermost group one block: they never touch each other, a `<move>` in a rule moves them all, and one hitting the left or right edge with `<bounce />` turns the whole block. Each cell is still swept on its own, so a bullet meets only the cells still alive. This is the invaders' march.

### Knocks

`<knock />`, in a rule about another object, makes the two meet as things with weight do:

- Along the way straight apart they share out what they carry by their `<mass>`es; across it each keeps its own, so a ball hit off its middle goes off at an angle.
- Each brings the speed of its point where they touch: its velocity, and for one [turning about a pivot](#turning-about-a-pivot) its spin times how far out the point is. A flipper's rule `<collision class="ball"><knock>0.3</knock></collision>` (the flipper with no `<mass>`, the ball with one) strikes harder the nearer the tip.
- The content, 0 to 1 (1 when left out), is how bouncy: 1 is pool balls, 0 sends them on together. Nothing happens unless they are closing, so a second `<knock />` for the same touch does nothing.
- **Friction and rest.** With `<friction>`, a thing that knocks or bounces off something with no `<mass>` loses speed along the surface: up to its friction times the speed the touch took off straight into it. And when it only settles there (below), it **comes to rest** if what it lies on is still and either its pull cannot slide it along that surface (the pull along it no more than friction times the pull into it: 0.05 holds a ball on a slope of under 3 degrees) or it lay against another surface a frame or two ago and its pull points between the two (wedged in a corner: a ball cradled between a raised flipper and the slope above it). At rest it does not move and is not pulled, so it is perfectly still, until something gives it a speed or moves it, or what it rests on moves, turns or swings. Without `<friction>` nothing rests.
- **Settling.** Against something with no `<mass>`, a thing closing slower than four frames of its own `<acceleration>` into that surface does not bounce at all: a ball lying on a flipper or a slope settles there instead of hopping. A `<bounce>` up to 1 settles the same way; a bumper's (above 1) never does. A pull that does not press it into the surface (a walker meeting a wall) never settles it.
- **No `<mass>` is as heavy as can be:** never moved by a knock, and its own knock sends a thing off at up to twice its speed (Trick Shot's cue tip).
- **Through a thing with no mass.** A knock on something with no mass goes through it to whatever with a mass rests against its far side, as from something as heavy as can be; that thing then runs its own rules about the knocker that say `through="class"` of what it came through. Mario's head hitting a floor tile from below is a `<knock>0</knock>` that reaches the shellcreeper standing on it, whose `<collision class="brother" through="floor">` flips it. A knock goes through one thing, not a pile.
- A row of balls passes a knock on by itself, each pair meeting in turn within the frame.

## Input

**Keys are bound in states, never in objects.** An object names what it can do; a state binds a key to that name:

```xml
<object name="paddle1">
  ...
  <actions>
    <action name="up"><move direction="up">step</move></action>
  </actions>
</object>
...
<state name="playing">
  <inputs>
    <input><button>w</button><button>up</button><trigger object="paddle1" action="up" /></input>
  </inputs>
</state>
```

So remapping a key is one attribute in one state, a key can do different things in different states, and one action name can mean different things on different objects.

- One key is `button="w"`; several keys for the same commands are a `<button>` each, before the commands (`<button>a</button><button>left</button><button>pad1.left</button>`).
- **Key names** are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more in `engine/keys.cpp`.
- **The mouse and pads** are keys too: `mouse.left`, `mouse.right`, `mouse.middle`, and for two pads `pad1.a`, `.b`, `.x`, `.y`, `.lb`, `.rb`, `.back`, `.start`, `.up`, `.down`, `.left`, `.right` (and `pad2.`), named for an Xbox pad. A pad's directions are its d-pad or its left stick pushed past halfway.
- **Key sets.** Bindings several states share can be a named `<keys>` set at the top of `<states>`, taken with `<inputs keys="walk">` (several by a `<keys>walk</keys>` each, first in `<inputs>`, each over the one before); the state's own `<input>`s go over the set's, key by key. No shipped game uses one now.

### A key's three moments

A key goes down, stays down and is let go; `on=` on an `<input>` picks which it answers:

| `on=` | Runs its commands |
|---|---|
| `press`, or left out | as the key goes down. The verbs that are held (`<move>`, `<accelerate>`, `<thrust>`, `<turn>` in the action it triggers) stay on until it is let go |
| `hold` | every frame while it is down, from the frame it went down, each time as one press |
| `release` | once, as it is let go |

```xml
<input button="space" on="hold"><trigger object="arrow" action="charge" from="ready" /></input>
<input button="space" on="release"><trigger object="arrow" action="putt" from="ready" /></input>
```

- A key may have all three, in one state or a key set, each its own `<input>`.
- **`hold` and `release` answer only a press made in the current state.** Space held through the title into the game neither charges the putt nor strikes it when let go; a pause and back ends what a key held before it was doing. A press binding keeps the old rule: its held verbs carry on into a new state that binds the key the same way.
- Putt Park builds a putt this way: held, an `<inc>` adds to the power and a `<formula>` `<smallest>` keeps it at 100; let go, a `<set>` gives the ball its speed along the arrow's heading.
- `on=` is for a button; a control that points (`axis=`) runs as it moves.

### Pointing controls

`<input axis="mouse.x">` (or `mouse.y`, `pad1.x`, `pad1.y`, `pad2.x`, `pad2.y`) runs its `<trigger>` whenever that control moves, and a `<track />` in the action it triggers puts the object where it points: its middle at the mouse, or as far across the window as the stick is pushed, kept inside the window. That is the Atari's paddle controller. It runs only when the control moves, so the keys can move the same object while the mouse rests. Kaboom's bucket and Breakout's paddle:

```xml
<action name="steer"><track /></action>
...
<input axis="mouse.x"><trigger object="bucket" action="steer" /></input>
```

An axis input runs only `<trigger>`s and goes in a state's own `<inputs>`, not a key set. Every window library reads the mouse and pads; Qt (`xgegui`'s own window) reads the mouse only.

### Held keys and state changes

- **A held `<move>`** records its direction's step while the key is down. On every key change the velocity along that key's axis is worked out from the two directions of the axis, so holding Down and tapping Up cancels out, and two keys on the two axes make a diagonal. A key on one axis leaves the other alone: walking does not stop a fall.
- **`<accelerate>`, `<thrust>` and `<turn>`** are held the same way, but add to the velocity or heading each frame instead of setting it, so what was gained stays when the key is let go. `<drag>` then gives thrust a top speed.
- **One-shot verbs** (`<hop>`, `<jump>`, `<leap>`, `<fire>`, a state change) run only on a real press. Two hops asked for in one frame keep the later.
- **Facing.** An object with a `<facing>` faces the way of the last key pressed; while keys of both axes are held it faces the diagonal between them, and a `<fire>` leaves along it (Berserk's man shoots eight ways).
- **The current state decides what a held key means.** When the state changes, keys still down stop driving what the old state bound them to and start driving what the new one binds them to. Hold Left in Breakout and pause: the paddle stops; unpause with Left still down and it moves again, with no new press. Only held bindings resume this way, so holding Space through the title does not pause the game. Letting go of a key always stops what it drives, unless another key still down drives the same action.

## Timers and paths

### Timers

```xml
<timers>
  <timer>
    <every><random><min>0.3</min><max>1.4</max></random></every>
    <reverse />
  </timer>
  <timer name="fuse">
    <after>3</after>
    <despawn />
  </timer>
</timers>
```

A `<timer>` goes off `<every>` so many seconds, again and again, or once `<after>` that long, and runs the commands after it.

- **Whose clock.** An object's timers (a group's are every member's) count while it is in play; a state's while it is the current state; a scene's while it is the current scene. So a pause stops them and they carry on afterwards.
- **Starting over.** A reset starts a timer over, and so does the object coming back into play (fired, released, revealed or spawned), so a shot that dies on a timer has its whole time each time. So does a push: a state's timers start over each time it is pushed, so a state that pops itself after a while (Space Invaders' `blasted`, Pac-Man's READY!) says `<after>`. A pop back to a state lets its timers carry on where they were (a pause does not restart them).
- **The interval is worked out again each round**, after the commands have run, so a `<random>` waits a new time each time (Kaboom's bomber turns whenever he likes) and an expression can follow a variable the commands change.
- Seconds become frames with the window's `<framerate>`; a timer never waits less than one frame, and `<after>0</after>` goes off on the first frame it counts.
- A visible clock is a timer that `<inc>`s or `<dec>`s a variable a text shows, or a named timer's time left (below).
- Conditions are the last thing in a frame, so a scene a condition puts back is drawn once before its timers count: what must happen before it is seen goes in that condition too.

**Look timers.** An object's timer with `sprite="blue"` counts only while the object shows that [look](#looks), and starts over whenever a `<become>` gives it that look, even the look it shows already. Looks and timers make modes: Pac-Man's ghosts have a `blue` timer `<after>fright</after>` that makes them `normal` again, and a second pellet starts the fright over.

#### Time left

An object's timer with a `name=` is also a number of the object's: `energy.level` is the seconds the timer `level` on `energy` has left, rounded up to whole seconds (3, 2, 1, then 0 once it has gone off). Anything that reads a variable reads it: a text's `<number>energy.level</number>`, a condition, a command's value. It keeps its value while the timer is not counting, a reset gives it its whole time again, and a `<set>` on it is undone the next frame it counts. The object cannot also have a variable of that name. A state's or a scene's timer has no object to hold the number, so a clock everyone sees is a timer on an object of that state with no picture. Megamania's energy bar reads `energy.level`.

A `name=` on a state's or scene's timer is for [`from=`](#scenes), which replaces a copied timer of the same name.

### Paths

```xml
<paths>
  <path name="dive">
    <speed>3</speed>
    <step><x>-24</x><y>-24</y><play sound="dive" /></step>
    <step><x>80</x><y>120</y><fire object="bomb" /></step>
    <step><x>-56</x><y>568</y></step>
    <home />
  </path>
</paths>
```

A `<path>` is a route written once in the game's `<paths>` and flown by any object that `<follow>`s it.

- It has a `<speed>` (pixels a frame), an optional `<start>` (an `<x>` and `<y>` it is put at when it sets off; else it sets off from where it is), then its legs, flown in turn.
- A `<step>` moves the object by its `<x>` across and `<y>` down from where the step began, in a straight line, and runs the commands after them as it sets off (the follower's own, as on its timer: `self` is whichever object is flying it, so Galaxian's dive sets `self.pays` for an escort a flagship took along as for the flagship).
- `<home />` flies it back to its `<position>` in the file, moved by any [`<shift>`](#commands) since, from wherever it has got to: a formation is just the objects' places, and a swaying one is shifted. Within a frame of its place it is put there. At the end of the path it comes to rest.
- A path sets the velocity each frame, so the follower moves, wraps and collides as anything does. A `<wrap />` on the way leaves what is left of the step alone: Galaxian's dive adds up to the window's height plus the alien's, so a diver that misses comes back in from the top to its own place.
- A step's `<x>` or `<y>` that names something live, or `self`, is worked out as the step starts, with `self` the follower: Galaxian's dive turns toward the ship with `<x>ship.x - self.x</x>`, and then flies that straight line whatever the ship does. Every other number on a path is fixed at load.
- An object already on a path finishes it first; a reset, `<die />`, `<stop />` or `<reveal>` ends it.

## States and scenes

### States

A state is a mode of the game: a title, play, a pause, a game over. The first in the file is where the game starts. `name` is required, and `from` copies an earlier state (below). Children, in this order:

| Element | What it does |
|---|---|
| `<objects>` | the state's own objects, in play whenever it is the current state, in every scene of it |
| `<scenes>` | the places it is played in, one current at a time ([scenes](#scenes)) |
| `<shows>` | `<show object="best" />`: an object of the game's (or another state's) drawn and run as if the state's own. `<show set="end" />`: a shared [show set](#shared-sets). `<show state="playing" />`: that state's objects drawn **still** underneath (not moving, not touching, their timers stopped), so a pause or a game over is drawn over the frozen game |
| `<music sound>` | a sound of the state's own: one that loops plays while it is the current state (stopping under a pause, starting again from its beginning after); one that does not plays once as the state begins |
| `<inputs>` | the keys and what each runs. Needed unless the state is `from=` another |
| `<conditions>` | tests checked every frame ([conditions](#conditions)) |
| `<computer>` | a player the game plays itself ([computer players](#computer-players)) |
| `<timers>` | timers that count while it is the current state |

```xml
<state name="paused">
  <objects>
    <object name="paused"> ... </object>     <!-- the PAUSED text -->
  </objects>
  <shows>
    <show state="playing" />                 <!-- the game, still, under it -->
  </shows>
  <inputs>
    <input button="space"><pop /></input>
  </inputs>
</state>
```

**The stack.** `<push state="paused" />` puts a state over the current one and `<pop />` goes back; the first state is never popped. `<pop state="result" />` puts another in the current one's place. A `<reset />` from a state's commands is a new game, and the stack collapses to the first state.

**A state from another.** `<state name="gameover" from="youwin">` is a copy of an earlier state with what it writes over it. Its objects are named by the new state (`gameover.message`), and one named like a copied object changes it by the [one change rule](#the-kit-and-from). Its `<input>`s replace the copied ones of the same key; its conditions and timers replace those of the same `name=`, after the rest; its music replaces the copied. A state with scenes cannot be copied yet. The end screens of 17 games are one state and a copy changing its message:

```xml
<state name="gameover" from="youwin">
  <objects>
    <object name="message">
      <sprite><text style="banner"><content>GAME OVER</content></text></sprite>
    </object>
  </objects>
</state>
```

**Drawing order.** First what is shown still (`<show state>`), then what is in play, each in the order written in the file, a later one over an earlier. A state's own objects come after its scenes', so they are drawn over them (`drawFrame`, `window/window.h`).

### Scenes

A state played in several places (Berserk's rooms, Pitfall's screens, Kaboom's waves, Adventure's castles) holds them as `<scenes>`: one is the **current scene**, and only its objects are in play, beside the state's own.

```xml
<state name="playing">
  <objects>
    <object name="hero"> ... </object>                  <!-- in every scene -->
  </objects>
  <scenes>
    <scene name="room1">
      <ways>
        <way name="out" scene="room2" />                <!-- where this room's exit leads -->
      </ways>
      <variables>
        <variable name="pace">2</variable>              <!-- read inside as pace -->
      </variables>
      <objects>
        <object name="topwall" />                       <!-- the kit's, as it is -->
        <group name="robots">                           <!-- the kit's, changed -->
          <velocity><x>pace</x></velocity>
        </group>
        <object name="exit"> ... </object>               <!-- this scene's own -->
      </objects>
      <conditions> ... </conditions>                    <!-- checked while it is current -->
      <timers> ... </timers>
    </scene>
    <scene name="room2" from="room1">                   <!-- all of room1, but its pace and way -->
      <ways><way name="out" scene="room1" /></ways>
      <variables><variable name="pace">3</variable></variables>
    </scene>
  </scenes>
  ...
</state>
```

- **Each scene has its own copies.** Two scenes that take the robots each have robots of their own, which move, die and are counted on their own.
- **Moving between scenes** is a command written where the move happens: `<enter scene="room2" />` in an exit's rule, a condition on the score, a timer. What an object of the state carries goes along.
- **A scene left stays as it was** until it is entered again; `<reset scene="room1" />` puts it back as it started; a new game starts every scene again with each state's first scene current.
- **Ways** let one kit piece lead somewhere different in each scene: a scene's `<ways>` name where each way leads, and `<enter way="east" />` or `<reset way="east" />` looks the way up in the *current* scene as it runs. Pitfall's side strips are written once in the kit; each screen says where its ways go.
- **A scene's `<variables>`** are its own numbers, read by its objects (a robot's speed in room 2 is room 2's `pace`).
- **Its `<conditions>` and `<timers>`** count only while it is current, after the state's own.
- **`from="room1"`** starts a scene as a copy of an earlier one: its objects, variables, ways, conditions and timers. What it writes is added, and a variable, way, condition or timer **of the same name** takes that one's place where it was (`<condition name="quota">`, `<timer name="launch">`). Unnamed ones are only added. A copied condition or timer is read in the new scene, so its plain names are that scene's. Kaboom's, Megamania's and Atlantis's later waves say only their numbers, pictures and where `next` leads.
- **What a condition counts.** A `class=`, or a name every scene shares (`robots`), counts what is in play: the state's own objects, the current scene's and the game's, so a wave is cleared when its own enemies are gone. A path (`room2.robots`) counts that scene's, wherever the game is.

### Shared sets

At the top of `<states>`, before the first state, named sets hold what several states share:

- `<keys name>`: inputs, taken with `<inputs keys="name">` ([input](#input)).
- `<shows name>`: shows, taken with `<show set="name" />` among a state's own shows, one set a `<show>`. Air-Sea Battle's two end screens take TIME UP and "press space" from one `end` set.
- `<conditions name>`: conditions, taken with `<condition set="name" />`, checked right there in order as if written in its place.

A set can take a set written before it.

### Conditions

```xml
<condition object="frog" variable="lives">
  <atmost>0</atmost>
  <push state="gameover" />
</condition>
```

A condition is checked every frame while its state (or scene) is current. It holds when an object matching `class=` and/or `object=` passes its test, and then runs its commands. **The first condition that holds runs, and no more are checked that frame**, so order matters. An optional `name=` lets a scene's `from=` replace it.

| Test | Holds when the variable (or a total) is |
|---|---|
| `<atleast>N</atleast>` | N or more |
| `<atmost>N</atmost>` | N or less (a count going down: lives) |
| `<above>N</above>`, `<below>N</below>`, `<equal>N</equal>` | more than, less than, the same as N |
| `<remaining>N</remaining>` | no more than N of the matched objects are still in play (`class="aliens"` with `0`: all shot) |

- **What is tested.** `variable="score"` is an object's variable, or, with none of that name, one of its live numbers (`x`, `y`, `velocity.x`, `velocity.y`, `heading`, `altitude`, `rise`, `light`, `blink`), so a condition can compare where things are.
- **The number is a value, read as the game runs** when it names something that changes, so a test can follow another object: Air-Sea Battle's winner.

  ```xml
  <condition object="gun1" variable="score">
    <above>gun2.score</above>
    <become object="result.outcome" sprite="yellow" />
    <pop state="result" />
  </condition>
  ```

- **`total=`** tests one number over everything matched and in play: `count` (no variable), or `sum`, `highest`, `lowest` of the variable. `<remaining>N</remaining>` is `total="count"` with `<atmost>N</atmost>`.
- **`distance="hero"`** tests how far each matched object is from the nearest of that name in play, middle to middle, instead of a variable. With `total="lowest"` (or `highest`, `sum`) it tests one number over them all: `class="hazard" distance="centre" total="lowest"` with `<above>150</above>` is "nothing within 150 of the middle" (Asteroids' ship waits for it; with nothing matched in play it does not hold, so add an `<any>` with `<remaining>0</remaining>`).
- **`<all>` and `<any>`** of tests: every one, or at least one. Inside them a `<condition>` is a test alone (its own `class=`, `object=`, `variable=`, `total=` or `distance=`, then one test tag), with no commands; they nest. Adventure's dragon wakes once, when the hero comes near:

  ```xml
  <condition>
    <all>
      <condition object="dragon" distance="hero"><below>160</below></condition>
      <condition object="dragon" variable="awake"><equal>0</equal></condition>
    </all>
    <inc variable="dragon.awake" />
    <become object="dragon" sprite="awake" />
  </condition>
  ```

- A condition that changes what it reads can count and start over: Frostbite's turns the four blue rows white and takes 4 off the count.
- A filter that matches nothing is a warning at load, since "none left" would hold at once. An object or variable the game does not have, a total of a variable none of its objects has, and an `<all>` with `class=` stop the load.
- A state's condition has no `self`, so it names the object it compares with; a rule's condition has `self`.

### Keeps

A new game (a full `<reset />`) puts every object back as it started. The objects in `<keeps>` are passed over: they keep their variables, place and looks from one game to the next.

```xml
<keeps>
  <keep object="best" />
</keeps>
...
<condition object="ship" variable="score">
  <above>best.score</above>
  <copy variable="ship.score" into="best.score" />
</condition>
```

**Kept on disk.** `xgecli`, `xgegui` and a generated program write what a game keeps (each kept object's variables and look) to `saves/<game>.xml` in the games folder when a new game starts and when the program closes, and read it when the game loads (`Engine::keepIn`). A game that keeps nothing writes nothing; tests write nothing unless they ask. Delete the file to start again.

#### A table of the best

A group whose members are the places, the best first, and `<rank variable="entry.score" into="table" />`: the entry takes the first place whose `score` is below its own, all its variables going into it, and the places below move down, the last falling off; a tie goes below. Whether a score makes the table is a condition, `<above>table.5.score</above>`. Galaxian keeps a top five, asking for initials one letter a state:

```xml
<group name="table">
  <sprite><text><letter>table.first</letter><letter>table.second</letter><letter>table.third</letter><number>table.score</number></text></sprite>
  ...
</group>
...
<input button="space"><rank variable="entry.score" into="table" /><reset /></input>
```

### Taking turns

Players take turns as the arcade machines did: each plays a game of their own, and the machine swaps them.

```xml
<input button="num2">
  <push state="playing" />
  <turns>
    <player state="player1" />
    <player state="player2" />
  </turns>
</input>
```

- Each player's game starts as the game is when `<turns>` runs: every object but the kept ones, every timer, the current scenes. The state current then is the one turns are played in. A `<player>`'s `state=` (optional) is pushed whenever that player's turn begins: a screen saying whose turn it is.
- **`<pass />`**: the turn goes to the next player still in. This player's game is put away as it is (what held keys were doing stops), the next one's comes back as they left it, and states above the playing one are taken off.
- **`<out />`**: this player is out, and the turn passes. When no one is left, what follows `<out />` in its list runs, so `<out /><reset />` is "the next player, or a new game". With no turns it does nothing, so the same file plays one player too.
- They go where a state's commands go, so a swap never happens mid-frame. A full `<reset />` ends the turns.

### Attract mode

A game's `<demo>`, after `<states>`, is a game recorded as it was played: the seed of its random numbers, how many frames it ran, and each key pressed and released on its frame.

```xml
<demo>
  <seed>20261016</seed>
  <frames>1200</frames>
  <press button="space">30</press>
  <release button="space">34</release>
  ...
</demo>
```

`<demo />`, where a state's commands go, plays it: a new game from the seed, its keys pressed on their frames, so it plays out exactly as recorded. A key pressed at the window, or the recording running out, ends it; a new game starts with what is kept as it was before (a demo's score is never the best). Galaxian's title runs `<demo />` on a timer after ten seconds. `xgecli <game> --record` writes one into the game file, in place of the old, keeping the file's line endings (`withDemo`, `load/xml.h`).

### Computer players

A state's `<computer>` is a player the game plays itself, pressing the state's keys as a person would: a list of conditions whose commands are `<hold button>`s. **Every** condition is tested every frame (not only the first that holds); the keys of those that hold are held down and the rest let go. Tennis's far end, on the keys a second player would use:

```xml
<computer>
  <condition object="cpu" variable="x"><below>ball.x</below><hold button="l" /></condition>
  <condition object="cpu" variable="x"><above>ball.x</above><hold button="j" /></condition>
</computer>
```

A `<hold>` anywhere else, or any other command in a `<computer>`, stops the load.

## What happens when a game runs

### Loading

1. **Parse and validate** (`load/xml`). The XML backend reads the file. If it names a schema it is validated: Xerces does full XSD validation ("strong"); the other three use a small built-in validator for the part of XSD the schema uses ("weak", `load/xsd`).
2. **Work out the kit and every `from=`** on the XML (`load/merge`), so everything after sees whole objects.
3. **Read** the tags into a `GameData` (`game/data.h`: raw objects, states, sounds, paths), a group as one raw object per member. A generated program starts here, with its `GameData` compiled in.
4. **Lay out states and scenes** (`layOutScenes`, `game/scenes.cpp`): every owned object becomes an ordinary object named by its path, and every name the file uses is written out in full.
5. **Evaluate** (`load/expr`, with `load/calc` for the arithmetic in text). Every value is worked out once (`<random>` drawn here); objects, states and sounds are built and every name a command uses is checked. Nothing here needs a window or a sound device.
6. **Open the window and the sound** (`Engine`). Every sound is made into samples, each object's real size is measured, positions waiting on a text's size are finished, and the first state is pushed. If the sound will not start the game plays silently, with a warning.

### Each frame

`Engine::step` reads the keys (or the demo's, or the computer's) and runs the current state's bindings for each press and let-go, then its `on="hold"` bindings for the keys down, then `Game::updateObjects` plays one frame (`playOneFrame`, `play/frame.cpp`), in this order:

1. animations and blinking count a frame;
2. timers count, and those that go off run;
3. paths set their followers' velocities; maze walkers choose their way; held moves on [cells](#cells) step;
4. climbing, grip, acceleration and held thrust, turns and swings, pulls and seeking, altitudes;
5. screen-edge rules for what is moving;
6. **the move**: queued hops are made, carried things are lent their carrier's motion, and everything moves (a swing turns through it), the touches found on the way running their rules ([how a touch is found](#how-a-touch-is-found));
7. sticks hold, carried things are put where they are held, tethers and shadows follow, light and dark are worked out, poses are picked;
8. conditions: the state's, then its current scene's;
9. copies made join the game and despawned ones go.

Then the sounds asked for play, loops start or stop, and the frame is drawn: what a `<show state>` shows still, then what is in play, each but the objects not `<drawn>`.

An object is **in play** when it is not out of play (`<active>false</active>`, dead, despawned) and belongs to the current state, its current scene or the game, or is one the state `<show>`s. Movement is in pixels per frame and timers count frames, with no time step.

## Backends

| Job | Interface | Implementations |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml/document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keys, mouse and pads | `Window` (`window/window.h`) | SFML 3 (default), raylib, SDL2, OpenGL (GLFW); and `xgegui`'s Qt renderer |
| Sound | `Audio` (`audio/audio.h`) | SFML 3 (default), raylib, SDL2, none (silent) |

- `Game` and `Engine` see only the interfaces; one factory per kind (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`) is the only place that knows every implementation.
- **A build has some of these, not all:** SFML 3 and Xerces by default, all with `-DXGE_ALL_BACKENDS=ON` or `-DBUILD_TESTING=ON`, each other with its own `-DXGE_WITH_<NAME>=ON`. A backend's sources compile only behind its `XGE_WITH_<NAME>`. The enums always name every one; each factory says what it has (`available()`, `availableBackends()`, `defaultBackend()`, `name()`), and `create()` for one not built throws, saying which are.
- **Pictures** the engine draws itself (lines, bitmaps, SVGs with lunasvg), so every window backend only shows a bitmap. Only text and image files are each backend's own.
- **Sound** is made by the engine (`synthesize()`, `audio/sound.h`), so a sound is the same whichever library plays it. `Game` never makes a noise: `<play>` asks (`Game::requestSound`), and `Engine` hands the frame's sounds to its `Audio`, so a game runs and is tested with no sound device. The sound library is chosen apart from the window, and any goes with any. `Engine::replaceAudio()` swaps it in a running game; `Engine::silence()` stops what plays.
- **Windows.** `Engine::replaceWindow()` swaps the window of a running game (the old destroyed first, since raylib and GLFW allow one). A front end that pauses but keeps its window calls `Engine::pump()` so it can be moved and closed. SDL is shared by the SDL2 window and sound, each starting only its own part.

See [backends](designs/50-backends.md) and, for `xgegui`'s one or two windows, [gui](designs/53-gui.md).

## Building

```
cmake -B build                      # SFML 3 and Xerces
cmake --build build --parallel
output/Debug/xgecli frogger
cmake -B build -DBUILD_TESTING=ON   # every backend, and the tests
output/Debug/xgetest                # Catch2 filters work: "[kaboom]"
cmake --build build --target xgetest-generated && output/Debug/xgetest-generated
```

- **Targets.** `xgelib` is the engine (everything in `xge/lib/`), a static library by default (`-DXGE_BUILD_SHARED=ON` for a shared one). `xgecli` (`xge/cli/`) and `xgegui` (`xge/gui/`, only when Qt 6 is found) are the programs; `xgetest` and `xgetest-generated` the tests; `xgedata` copies `games/` beside the programs.
- **Output.** Everything to run lands in `output/<Config>/` (ignored by git): the programs, a copy of `games/` with the schema, and the DLLs (or `.so` files) and Qt plug-ins in `libraries/`, found through a manifest on Windows and a relative run path elsewhere. The folder can be copied anywhere.
- **Dependencies** are found through vcpkg or the system, or fetched and built when missing (`FORCE_LOCAL_<NAME>` forces a fetch). Qt 6 and libxslt are found only, never fetched. CMake modules are in `scripts/cmake/` ([build-and-layout](designs/51-build-and-layout.md)).
- **Warnings** stay at zero under a strict set on GCC, Clang and MSVC (`xge_warnings()`, `platform.cmake`).
- **Folders.** The engine is a folder per part, a header beside its source, included by that path (`"game/game.h"`). The games and all they are made of are in `games/` (`descriptions/`, `assets/`, `generators/`, and the schema `xgedef.xsd`, which every game names as `../xgedef.xsd`). The programs make `games/` the working directory, and so does `xgetest`. `--generate` copies the engine's files from `xge/lib/` of the repository it was built from.

## Source map

| Part | Files | What |
|---|---|---|
| reading | `load/xml.cpp` | the tags into a `GameData`, `readGame()`; checks of shape |
| | `load/merge.cpp` | the [kit and from=](#the-kit-and-from) on the XML (`layOut`, `addWaiting`), as `merge.xsl` does |
| | `load/expr.cpp` | values, `<formula>`, building `Object`s and `State`s, pictures (`buildPicture`) |
| | `load/calc.cpp` | the arithmetic in a value's text: numbers, names, `+ - * /`, a sign and brackets, compiled once into steps that read each name where it lives |
| | `load/check.cpp` | the load-time checks of every name a command uses, and where each command may go (`ruleFor`) |
| | `load/xsd.cpp`, `load/spelling.cpp` | the weak validator; "did you mean" (`didYouMean`, `listOf`) |
| the game | `game/game.cpp` | the interpreter: objects, the state stack, scenes, conditions, resets, paths, spawns, and the answers to `FrameRules` (what the file says happens at an edge or a touch), carried out through `CommandExecutor` |
| | `game/scenes.cpp` | `layOutScenes`: owned objects named by path, states `from=` others, names written in full |
| | `game/command.cpp` | raw command tags into typed `Command`s; the table of arithmetic operations (`operationShape`) |
| | `game/object.*`, `game/states.*`, `game/data.h`, `game/random.cpp` | the data model; the one random number source |
| the frame | `play/frame.cpp` | `playOneFrame`: the order of a frame and the moving |
| | `play/geometry.cpp`, `play/collision.cpp` | the arithmetic of touching (box, circle, swept, the pixel walk, `contactOf`) and of the verbs; the same for `Object`s |
| | `play/verbs.cpp`, `play/executor.cpp` | what a verb does to an object; picking the verb and its target |
| | `play/maze.cpp` | walking a map's tiles |
| | `game/cells.cpp` | maps of [cells](#cells): spawning into cells, cell steps and turns (the kick tables), settling, the checks |
| pictures | `picture/bitmap.cpp`, `picture/svg.cpp`, `picture/font.cpp`, `picture/color.cpp` | lines and bitmap rows into pixels, flips and turns; SVG with lunasvg; font files and the built-in 8x8 font; color names |
| loop and libraries | `engine/engine.cpp`, `engine/backends.cpp`, `engine/keys.cpp`, `engine/folder.cpp` | the loop (`loop()`, or `step()` and `render()`), keys and controls, sounds a frame asked for; choosing libraries by name; key names; the games folder and `findSchema` |
| | `window/*`, `audio/*`, `xml/*` | each kind's interface and factory, one file per library; `audio/sound.cpp` is the synthesizer |
| programs | `xge/cli/source/` | `cli.cpp` (options, finding the game), `generate.cpp` (`--generate`), `main.cpp` |
| | `xge/gui/source/` | `main_window`, `game_session` (a game and its engine on a timer), `game_stage`, `game_view`, `qt_window` (the Qt renderer), `key_queue`, `options_dialog`, `session_options`, `app_settings` (`xgegui.ini`), `inspector` (the tree), `theme` |
| generator | `games/generators/windows-cpp/` | `generate.xsl`, `merge.xsl`, `data.xsl`, `program.xsl`, `text.xsl`, `engine.xml`, `modules/game_parts.h` |
| tests | `xge/tests/` | one `test_<topic>.cpp` per area: each verb and part of the format (`test_xml_format`, `test_places`, `test_formulas`, `test_kit`, `test_spawn`, `test_scenes`, ...), the backends and the command line, and each game played frame by frame (`test_<game>`). `played.h` is what game tests share, `invaders_fixture.h` a frozen first Space Invaders, `scratch.h` a scratch name per run. `xgetest "[.speed]"` prints every game's load and frame times. `test_xml_libraries` (every game through all four XML libraries, a few games a test) and the slowest game tests come first in `tests.cmake`, so ctest starts them first |

## Known limitations

**Movement and collisions**

- Movement is in pixels per frame and timers count frames, so a machine that cannot keep up slows the game. Acceleration is constant per object; `<drag>` and `<grip>` are what slow a thing by itself.
- Only touches between objects are swept. A screen edge is checked by position before the move, so something faster than a window's width a frame can miss it.
- A pair where nothing moves is not looked at, so a rule against something standing still runs only when the object moves, rides, hops or lands.
- A surface faces a way only as drawn: a `pixel` touch reads its slope from four pixels round the point, so a line drawn with stairs of one-pixel steps is a slope, not stairs. Nothing turns about a point other than its middle (a flipper), and a turning thing lends nothing of its turn to what it hits.
- A `pixel` collision is exact to half a pixel of motion; lines meant to touch should be at least 2 pixels thick. Text and images cannot be `pixel`.
- `unless=` names a class, not one object. A rule on a screen edge cannot have a `<condition>` yet (a state's condition can test the same thing).
- `<ride />` lends velocity a frame at a time while touching; it is not attached, so a rider meeting an edge is handled by that edge's rule.
- `wrap()` assumes a lane is spaced with its objects' size in mind (a lane of `size`-wide things repeats every window width plus `size`).

**Jumping, steering and paths**

- The jumps are an instant step (`<hop>`), a timed straight line with an optional drawn arc (a timed `<hop>` or `<jump>`), and a fixed arc under a pull (`<leap>`). There is no air control.
- `<chase>` goes straight at its target and `<aim>` straight at where it is, through walls and with no lead.
- A maze walker steers as the arcade's ghosts do, by the next tile nearest a target: there is no search, so a target behind a wall can leave it going round. Pac-Man's ghosts follow the arcade's first-level schedule of scatter and chase on every level, and the four-second idle rule counts thirty dots rather than letting the next ghost out at once.
- A path is straight legs at one speed, with numbers fixed at load; home is the object's place in the file, so a formation cannot sway. A path cannot aim.
- Enemies fire on timers from wherever they are: an invader behind others fires through them.
- Cell pieces jump a cell at a time; nothing glides between cells (Centipede's arcade segments move a pixel at a time).
- A `<go>` on cells keeps the speed it was given, so Snake's speed-up takes hold at the next turn; a chain cannot wrap round the map's edge.
- On a map of cells, rows can be asked whether they are full and lines whether they are alike, but not groups of alike pieces touching (Puyo Puyo); a check cannot count its rounds (a chain's bonus) or the pieces it found; there is no hold (it wants a send that swaps), lock delay or ghost piece, and pieces spawn into the top rows with no hidden rows above. A spawn's `<random />` is a plain pick, not a bag of seven.
- `<facing>` is one of four ways (diagonals come only from two held keys), and never turns the picture.

**Pictures and sound**

- Pictures drawn by the engine are fixed at load (an SVG at one scale; to show two sizes, make two sprites); they can be turned by a heading, swapped for another look, recoloured by a palette swap, flipped by a mirror and holed.
- An animation has one interval for all its frames, cannot run once and stop, and cannot be started by a collision. An object with a `<heading>` cannot take a palette swap.
- Turning keeps the original and the one picture shown: about twice the memory. A big drawing costs its bounding box, four bytes a pixel (Lunar Lander's moon is about 580 KB).
- Text is drawn with a font file, unsmoothed; the built-in 8x8 font is only a fallback, and its widths differ from the real font's.
- An object's own variable named `width` or `height` shadows its size.
- Music is a chip's: parts of one wave each, no volume envelope, no stereo, no tempo of its own (a length can use a variable). A state's looping music restarts after a pause and cannot be silenced while the state is current. A sound is made when the window opens, so its notes cannot follow a variable.

**Values**

- Values not in a command or a condition's test are fixed at load. A class condition cannot compare each object with that object's own other variable (a state's condition has no `self`).
- `<formula>` has no `tangent`, square root or power, and no "if": a comparison is a 1 or a 0 to multiply by. An operation is a row in `operationShape`, a case in `evaluateOperation` and the schema.
- A `<divide>` by something not known while loading gives 0, and only a position or a timer's interval is worked out again later.
- Expressions are text to the schema, so a typo in one is found when the game loads, not by validation. The weak validator covers only the XSD the schema uses.
- The window's size and framerate are plain numbers, read once; the size cannot change while a game runs.

**States, turns, demos**

- A `<random>` is drawn again only when its object comes back into play; entering a scene draws nothing, and a game-level `<random>` is drawn once.
- A state shown still is frozen whole, so a demo plays as a game of its own, not behind an attract screen. An object shown by `<show object>` in two states runs in both.
- A state with scenes cannot be copied with `from=`, and a copied condition or timer can be replaced but not taken away.
- Players taking turns see only their own game, so a screen cannot show both scores at once.
- A demo records the keys, not the mouse or a stick, and game-level `<random>`s are not drawn from its seed; a demo recorded with an older version of a game plays the old keys into the new one.
- A table of the best has no place numbers unless each member's text has its own, and its numbers do not line up in a proportional font. A saved file holds numbers and looks, not where a kept object is.

**Generating**

- A generated program is the engine with the game built in, so it shares the engine's limits and has none of its own. It always carries lunasvg, even for a game with no `<svg>`.
