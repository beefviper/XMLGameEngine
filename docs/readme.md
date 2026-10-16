# XMLGameEngine: current design

This document describes what the engine does **today**, as read from the source on 2026-10-04 (before that, games were written as function-call attribute strings; see [designs/01](designs/01-vision.md)). For the reasoning behind these choices, the alternatives that were considered, and ideas that are not built yet, see [designs/00-designs.md](designs/00-designs.md).

XMLGameEngine is a video game description language (VGDL) written in XML, plus a C++ engine that loads a game description and runs it. A game is one `.xml` file. The description is declarative: there are no loops and no `if` statements in it, and no function calls either. Behavior comes from a fixed vocabulary of verbs (the tags `<bounce />`, `<stick />`, `<die />`, ...) that the engine knows how to carry out. In this document, and in the design notes, a verb is sometimes written as `bounce()` for short; in a game file it is always the tag.

## Running a game

```
xgecli                       # loads "pong"
xgecli breakout              # a bare name gets ".xml" appended
xgecli pong.xml              # a name with an extension is used as given
xgecli -g pong -w sdl2 -x tinyxml2
xgecli --game pong --window raylib --xml pugixml
xgecli pong -a sdl2          # sounds played by SDL2 (or -a none for silence)
xgecli galaxian --record     # play, and write what was played into the file as its <demo>
```

The game is a bare argument or `-g` / `--game`; both are looked for the same way: as given (a path, or a name in the current directory), then its file name in the working directory, then its file name in `descriptions/` of the games folder (the first folder with both `descriptions/` and `assets/` in it, looking at the working directory, the program's folder and the folder above it, each itself and then its `games/`; the program then runs from the games folder, so it can be started from anywhere, and a game's asset paths are relative to it). If it is not found the program prints an error and exits. `-w` / `--window` picks the window library (`sfml3`, `raylib`, `sdl2`, `opengl`; default `sfml3`) `-x` / `--xml` the XML library (`xerces`, `tinyxml2`, `pugixml`, `rapidxml`; default `xerces`) and `-a` / `--audio` the sound library (`sfml3`, `raylib`, `sdl2`, `none`; default `sfml3`), not case sensitive. Only the libraries the program was built with can be named ([Backends](#backends)): a default build has `sfml3` and `xerces` (and `none` for sound), `--help` lists what there is, and another is an error that says which there are. Any sound library goes with any window library. A short option takes its value attached or after a space (`-gpong`, `-g pong`); a long option needs the space (`--game pong`, not `--game=pong`). Each option can be given once, the game only once (bare or with `-g`), and `-h` / `--help` prints the usage. `--record` (long form only) plays the game as usual and, when the window is closed, writes what was played into the game file as its [`<demo>`](#attract-mode), replacing the one it had; it cannot go with `--generate`. The program starts by printing the file, window library, XML library and sound library it chose, one to a line. See [cli](designs/52-cli.md). Shipped games: `games/descriptions/pong.xml`, `games/descriptions/breakout.xml`, `games/descriptions/spaceinvaders.xml`, `games/descriptions/spaceinvaders2.xml` (the same game drawn from an SVG sprite sheet), `games/descriptions/frogger.xml`, `games/descriptions/spacerace.xml` (two players, W/S and Up/Down, first to two points), `games/descriptions/kaboom.xml` (A/D or Left/Right; the Mad Bomber paces the rooftop and drops bombs as he goes, catch them through three waves, three missed bombs end the game, 90 points win), `games/descriptions/freeway.xml` (two players, W/S and Up/Down, first to five crossings), `games/descriptions/depthcharge.xml` (A/D or Left/Right to move, Space to drop, Space to start; sink all nine submarines before eight charges are wasted) `games/descriptions/astrosmash.xml` (A/D or Left/Right to move, Space to fire, Space to start; shoot 20 rocks before five land) and `games/descriptions/lunarlander.xml` (Up or W for the main thruster, Left/Right for the side ones, Space to start; set the lander down on the green pad slower than the safe speed, with fuel to spare, and do not touch anything else), `games/descriptions/asteroids.xml` (Left/Right or A/D turn, Up or W thrusts, Space fires; break every rock before three ships are lost), `games/descriptions/berserk.xml` (W A S D or the arrows walk the man and set the way he faces, Space fires that way, one shot at a time, P or Escape pauses, Space starts; four mazes of electrified walls with robots that patrol and fire at the man, a bonus for clearing a room, Evil Otto coming for him if he lingers, and an exit in each outer wall for the next room; after the arcade game Berzerk, see [games-classic](designs/54-games-classic.md)), `games/descriptions/demonattack.xml` (A/D or Left/Right, Space to start and to fire; a small game first written by another AI from the schema alone, see [games-classic](designs/54-games-classic.md); the demons fire back), `games/descriptions/pong_min.xml` (Pong with nothing but two paddles and a ball, W/S and Up/Down, a point puts the ball back in the middle; the game [`--generate windows-cpp`](#generating-a-program) was grown on), `games/descriptions/frostbite.xml` (W/S or Up/Down jump between the shore and four rows of drifting ice, A/D or Left/Right walk; land on white ice to build the igloo before the cold gets you, then walk in; see [timers](designs/09-timers.md)), `games/descriptions/galaxian.xml` (A/D or Left/Right, Space to start and to fire, P or Escape pauses; the fleet flies in on [paths](#paths) and dives at you, three ships; a new fleet when one is shot down; the best score and a table of the best five, with initials, are [kept](#keeps) from game to game and on disk; 2 on the title for two players [taking turns](#taking-turns); left on the title it plays itself, its [attract mode](#attract-mode)), `games/descriptions/donkeykong.xml` (A/D or Left/Right walk, W/S or Up/Down climb the ladders, Space starts and jumps, P or Escape pauses; climb past Kong's barrels to Pauline, three times, with three lives), and six after Atari 2600 games ([games-atari](designs/56-games-atari.md#five-atari-2600-games)): `games/descriptions/pitfall.xml` (A/D or Left/Right run, Space starts and jumps, W/S climb, P or Escape pauses; four jungle screens, the scenes of one state built from a kit of pieces, with logs, a tar pit, a crocodile pond, a fire, scorpions in the tunnel and a treasure on each, three minutes and three lives), `games/descriptions/missilecommand.xml` (W A S D or the arrows move the sight, Space launches, P or Escape pauses; the base `<aim>`s at the sight, a counter-missile bursts there, missiles `<chase>` the nearest city, three waves), `games/descriptions/combat.xml` (two players: W/S drive, A/D turn, F or Left Shift fires; Up/Down, Left/Right, Right Shift or Right Control; Space pauses; tanks with headings among walls, first to five hits), `games/descriptions/airseabattle.xml` (two players: A/D swing the gun and W fires; Left/Right and Up; Space pauses; four lanes of planes, two minutes, most hits wins, and the end screen says who) and `games/descriptions/megamania.xml` (A/D or Left/Right slide, Space fires, P or Escape pauses; three waves that wrap round the sides and drop bombs while the energy runs down, three ships), and `games/descriptions/atlantis.xml` (A or Left fires the left post, W or Up the middle one, D or Right the right one; Space, P or Escape pauses, Space starts; Gorgon ships drop a lane each pass and ray the city from the lowest, three waves, the city gone is game over); `games/descriptions/pacman.xml` (W A S D or the arrows steer, Space or P pauses, Space starts; the maze is a [map](#maps), four ghosts hunt Pac-Man each their own way, a power pellet turns them blue, eat every dot to win, three lives); and `games/descriptions/adventure.xml` (W A S D or the arrows walk, Space puts down what is carried, P or Escape pauses, Space starts; one room of Adventure: `<carry />` the key to the castle gate, the sword to the dragon and the chalice into the hall).

The XML, window and sound libraries are chosen in C++ with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`; `xgecli` passes what `-x`, `-w` and `-a` named, Xerces, SFML3 and SFML3 when they are not given. `xgegui` (the Qt application) takes the game the same way, `xgegui pong`, or opens a file dialog in `games/descriptions/` when none is named; its Options dialog picks the video library, the XML parser and the sound library. See [Backends](#backends).

A game file that is wrong (it does not match the schema, an expression will not evaluate, a command names a state or object the game does not have) stops the load with a message saying where; `xgecli` prints it and exits, `xgegui` shows it and carries on.

## Generating a program

```
xgecli pong --generate windows-cpp                     # writes pong-windows-cpp/
xgecli pong --generate windows-cpp -o out/pong         # or into a folder named
```

`--generate <target>` does not play the game: it turns the game file into a program of its own that plays it, and exits. `-o` / `--output` names the folder (default `<game>-<target>` in the directory `xgecli` was started from) and is an error without `--generate`. `-g` stays the game. The target, `windows-cpp`, writes a C++ program on SFML 3 that is the engine with the game built in. The work is done by XSLT stylesheets in `games/generators/windows-cpp/`, run by libxslt (EXSLT's `exsl:document` writes the files), once the engine has loaded the game as it would to play it (a game it will not load is not generated); `xgecli` runs them and copies in the files they list:

- `source/<game>.cpp` (`data.xsl`): the game written out as the engine's own data, the `GameData` (`game_data.h`) `readGame` makes of the file, each tag the line of C++ that makes what the engine makes of it: the window, the variables, every object, every state, the sounds and the paths, with the expressions as written. A `<group>` is written as the file has it: what the group gives, a function for each `<member>`, `<row>`, `<column>` or `<cell>` that changes something, and a line for each member or cell naming the parts that change it. One function returns the game, `<game>Game()` (`pongMinGame()` for `pong_min.xml`). The lines call the small functions of `source/game_parts.h` (`modules/game_parts.h`, copied unchanged).
- `source/main.cpp`: makes the `Game` from it, an `SFMLWindow` and an `SFMLAudio` (silent if there is no sound device), and runs `Engine::loop`, as `xgecli` does.
- `engine/` (listed in `engine.xml`): the engine's own files, copied unchanged from `xge/lib/` in the repository `xgecli` was built from: `Game`, `Engine`, `game_expr` (with exprtk), the collisions, commands, objects, states, keys, geometry, pictures (`bitmap`, `svg` with lunasvg) and sounds, and SFML's window and audio. Left out: reading XML (`game_xml`, the XML libraries, `xsd_lite`), the other window and sound libraries, and the factories that choose among them (`window.cpp`, `audio.cpp`, `engine_backends.cpp`).
- `assets/`: the font, if it shows text, and the pictures and drawings it names. `CMakeLists.txt` finds SFML 3, exprtk and lunasvg or fetches them, compiles with the engine's warnings, makes the game Visual Studio's startup project, and puts the built game, its DLLs and its `assets/` in `output/`. Also `README.md` and a `.gitignore` (`build/` and `output/`).

So it plays as `xgecli` and `xgegui` do: the data is the engine's reading of the file, and from the data on it is the same code. `xge/tests/test_generated_games.cpp` checks every game: building its test program, `xgetest-generated` (left out of an ordinary build; `cmake --build build --target xgetest-generated`, or `scripts/shell/windows/games-test-all.bat`), generates each one, compiles its data in, compares it with what `readGame` reads from the file, every field, and plays it beside the game read from its file, frame by frame. A folder made for a game that cannot be generated is removed. Without libxslt `xgecli` is built without `--generate` and says so. See [generator](designs/40-generator.md).

## Game file layout

A game file has one `<game>` root with three children, and four optional ones (`<sounds>`, `<paths>`, `<objects>`, which a game whose states own all its objects leaves out, and [`<keeps>`](#keeps)), in this order, as enforced by [xgedef.xsd](../xge/xgedef.xsd):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../../xge/xgedef.xsd">
  <window name="..."> ... </window>
  <variables> <variable name="...">...</variable> ... </variables>
  <sounds>    <sound name="..."> ... </sound> ... </sounds>   (optional)
  <paths>     <path name="..."> ... </path> ... </paths>      (optional)
  <objects>   <object name="..."> ... </object> <group name="..."> ... </group> ... </objects>   (optional)
  <keeps>     <keep object="..." /> ... </keeps>              (optional)
  <states>    <state name="..."> ... </state> ... </states>
</game>
```

The game's `<objects>` are seen by every state. A [`<state>`](#state) can own objects of its own, and hold [`<scenes>`](#scenes) that own theirs; the game's own objects are kept for what several states share (a game may need none of them once its states own their objects, which is left for later).

### Attributes and content

The rule the whole format follows: **an attribute names or picks something; everything else is the content of an element.**

- Attributes are labels that cannot be computed: `name`, `class`, `object`, `variable`, `state`, `action`, `button`, `edge`, `direction`, `unless`, and for sounds `sound`, `wave`, `pitch` and `to`. `<inc variable="paddle1.score" />`, `<collision edge="left">`, `<input button="space">`.
- Everything else, a size, a color, a position, a flag, is written between tags. `<radius>10</radius>`, `<fullscreen>false</fullscreen>`.
- What something *does* is a list of command tags, run in the order written. There are no `;`-separated strings.

### Values

Wherever the content of an element is a number (a position, a velocity, a size, a variable's value, a threshold, a step) it is a **value**, and a value is one of three things:

- **An expression written out as text.** Ordinary arithmetic, with the names below: `window.width.center - title.width / 2`, `margin`, `ball.radius * 2`, `30`. It is evaluated by exprtk at load time.
- **One value tag**, which makes the number: `<random min="-7" max="7" />`, a random number from `min` to `max` (in either order; both ends are themselves expressions). Any place that takes a value takes it:

```xml
<velocity>
  <x><random min="-7" max="7" /></x>
  <y><random min="-3" max="3" /></y>
</velocity>
```

A value holds text or a tag, not both, and not two tags (`<x>5 <random .../></x>` is an error naming the element). A `<random>` is drawn at load and drawn again whenever its object comes back into play: it is reset (a `<reset />` in a collision, `<reset object>`, `<reset scene>` of its scene, or a reset of the whole game), revealed (`<reveal>`), released (`<release>`) or fired (`<fire>`). So each Pong serve is new, and each rock released from a pool falls its own way. A `<random>` in a game-level variable is drawn once.

- **Arithmetic written as tags**, an `<equation>` or a `<formula>`: the same sums as the expression text, with nothing left in a string for a tool to parse. They are value tags too, so they go wherever `<random>` does. See [Arithmetic as tags](#arithmetic-as-tags).

The names an expression can use:

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | window height, window width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| any global `<variable>` | its value. They are worked out in the order written, so a variable can use the ones above it |
| any `objectName.variableName` | that object's variable (available regardless of the order objects appear in the file) |
| `objectName.width`, `objectName.height` | the width and height of any object (its sprite's footprint); an object refers to itself by its own name, like any other field. Meant for `<position>`. An object's own `<variable>` named `width` or `height` wins over its size. A circle's or rectangle's size is known from its sprite, so the position is exact at load. A text's or image's size is only known once the window has measured it, so its position is finished then, and worked out again whenever the size changes (a score gaining a digit); until then `printGame` shows it as unknown. |

The other exprtk math functions (`min`, `max`, `sqrt`, ...) work too, because exprtk brings them; Berserk uses `max` to keep its waits from shrinking below a floor (`max(6, 14 - player.depth)`). Nothing else about the format depends on them. (Neither tag has a `min` or `max` yet; see [Arithmetic as tags](#arithmetic-as-tags).)

### Arithmetic as tags

The same arithmetic has three spellings, and all three give the same number: expression text (`window.width.center - title.width / 2`), an `<equation>` and a `<formula>`. They sit side by side in one game, even in one object; use whichever reads best. Pong's title is placed with an `<equation>` and Breakout's with `<formula>`s, so each is played by a real game.

Both are built from the same four operations, each with a first operand and a second, named for what they are:

| Operation | First operand | Second operand | Means |
|---|---|---|---|
| `<add>` | `augend` | `addend` | first + second |
| `<subtract>` | `minuend` | `subtrahend` | first - second |
| `<multiply>` | `multiplicand` | `multiplier` | first * second |
| `<divide>` | `dividend` | `divisor` | first / second |

An operand is a **name or a number**, never an expression: `window.width.center`, `title.width`, `xoffset`, `2`, `-110`, `0.5`. Anything more is an operation of its own, which is how precedence is written: the operations are worked out in the order they are given, innermost first, so there is nothing to parse.

**`<equation>`** is a list of steps, run in order, one operation each, the operands written as attributes. A step may be given a `name`, and the steps after it (in the same equation) use that name for its answer; the last step is the answer of the whole equation. A step name is letters, digits and underscores with no dots, is local to the equation, and is used before a variable or object value of the same name; two steps cannot share one.

```xml
<x>
  <equation>
    <divide   name="half" dividend="title.width" divisor="2" />
    <subtract minuend="window.width.center" subtrahend="half" />
  </equation>
</x>
```

**`<formula>`** is one operation whose operands are elements, each holding a name, a number, a `<random>` or another operation. The first operand is combined with each of the others in turn, left to right, so `a - b - c` is one `<subtract>` with a `<minuend>` and two `<subtrahend>`s, and `a / b / c` one `<divide>` with two `<divisor>`s. There must be a first operand and at least one second.

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

A step of an `<equation>` takes its operands as attributes and an operation inside a `<formula>` as elements, and each form says so when it is written the other way. Which operations may sit where is checked by the schema (both validators), and what an operand may be, and the names, by the loader, with a message that names the element (`object 'title' > <position> > <x> > <equation> > <add>: "a + b" is not a name or a number; ...`). Everything is worked out when the game loads, like any value, and a size-dependent position (a text centered by its own width) is finished once the window has measured it, as it is for text. `printGame` shows an equation or formula as infix with its brackets.
**Division by 0.** A divisor that is certainly 0 when the game loads (a number, a global variable, or a step made only of those) stops the load, with a message that names the operation (`object 'o' > <equation> > <divide>: the divisor is 0`). A divisor that is 0 only because its value is not known yet (another object's variable, which reads 0 until that object is built, or the size of a text or image before the window has measured it) gives 0 for the answer and says nothing; a position or a timer's interval that depends on it is worked out again when it is known. While the game runs (a timer's interval worked out again, a position finished once a text is measured) a divisor of 0 gives 0 and a warning on standard error, once for each place, and the game carries on. The schema cannot see what a name holds, so it cannot refuse a divisor; only the engine does. The expression text is unchanged and divides by 0 as it always did.

### `<window>`

`<window name="Pong">` (the title), then in this order:

| Element | Meaning |
|---|---|
| `<width>`, `<height>` | Size in pixels. Plain whole numbers: the expressions in the rest of the file are worked out against the window's size, so these cannot be expressions themselves |
| `<background>` | A color name, e.g. `color.black` |
| `<fullscreen>` | `true` or `false` |
| `<framerate>` | Frames per second (0-255) |

### `<variables>`

Global named numbers. Each `<variable name="margin">30</variable>` becomes a constant that any expression in the file can use (for example `margin`, `ball.radius`). Names may contain dots, and a name written so is always that variable, even where its first part names an object (a state's `ball` and the game's `ball.radius`). A name exprtk already has for a function (`floor`, `min`, `max`, `abs`, `sqrt`, ...) cannot be used: an expression reads it as the function and the load stops (`Expected a '(' at start of function call to 'floor'`), which is why Pitfall's tunnel floor is the variable `tunnel`. The content is a value, so `<variable name="b">a * 2</variable>` and `<variable name="start"><random min="1" max="3" /></variable>` both work. Declaring a name twice keeps the later value.

### `<sounds>`

Optional. The bleeps, bloops and little tunes of an old 8-bit machine, written as notes: nothing is loaded from a sound file. Each `<sound name="...">` is something a `<play sound="..." />` command (see [Commands](#commands)) can start. Its children, in this order: an optional `<loop />`, an optional `<volume>` (a value, 0 for silent to 1 for the loudest; 0.3 if left out), then any mix of `<note>`s and `<rest>`s, played one after the other.

```xml
<sounds>
  <sound name="wall" wave="square">
    <note pitch="A3">0.04</note>
  </sound>
  <sound name="score" wave="triangle">
    <volume>0.4</volume>
    <note pitch="E5" to="E3">0.35</note>
  </sound>
  <sound name="gameover" wave="square">
    <note pitch="G4">0.2</note>
    <note pitch="F#4">0.2</note>
    <note pitch="E4" to="D#4">0.5</note>
    <rest>0.1</rest>
    <note pitch="2000" wave="noise">0.15</note>
  </sound>
</sounds>
```

- **`<note pitch="..." to="..." wave="...">seconds</note>`**: a tone. The content is how long it lasts, a value in seconds (so `<note pitch="C5">beat / 2</note>` works with a variable `beat`), more than 0 and at most 10. `pitch` is a note name with an octave, a letter `A` to `G`, an optional `#` (sharp) or `b` (flat), then the octave: `C4` is middle C and `A4` is 440 Hz, as on a piano; or a plain number of hertz (`440`, `97.5`). `to` (optional) is a pitch to slide to while the note plays, evenly in semitones: a bloop that falls, a laser that rises. `wave` (optional) is the note's own wave instead of the sound's.
- **`<rest>seconds</rest>`**: silence that long, between notes.
- **`<loop />`**: the sound plays over and over, from its end straight back to its start, instead of once. A `<play>` of it switches it on, and it stays on until a `<silence sound="..." />` (anywhere a `<play>` can go), the reset of the whole game, or, when the `<play>` is on a key or in the action a key triggers, until that key is let go, as a `<move>` is held: Asteroids' and Lunar Lander's engines hum while thrust is held. Asking for a loop that is already on leaves it playing; one silenced and asked for again in the same frame (one of two keys for it let go, the other still down) plays on. A state change lets go of what the held keys were driving, so a pause is quiet, and the new state's held keys start their loops again.

  ```xml
  <sound name="thrust" wave="noise">
    <loop />
    <volume>0.15</volume>
    <note pitch="150">1</note>
  </sound>
  ...
  <action name="thrust">
    <accelerate direction="up" burn="fuel">thrust</accelerate>
    <play sound="thrust" />
  </action>
  ...
  <condition object="lander" variable="fuel">
    <atmost>0</atmost>
    <silence sound="thrust" />
  </condition>
  ```

- **`wave`** on a `<sound>` is the wave of every note that does not give its own: `square` (the default; the classic beep), `triangle` (softer, the bass of an old console), `sawtooth` (buzzy), `sine` (pure and soft, which no 8-bit chip had) or `noise` (hiss and crashes: a new random level every half cycle of the pitch, so a high pitch is a hiss and a low one a rumble).

Every sound is made into samples (16 bit, one channel, 44100 a second) by the engine when the window opens, the same for every sound library ([sound](designs/33-sound.md)). Each note fades in and out over a couple of milliseconds so it does not click. Each sound has one voice, like a channel of a sound chip: playing a sound that is still playing starts it again from the beginning, and different sounds play over each other. A sound asked for more than once in one frame is played once.

A wave, pitch or length the engine cannot use, a `<volume>` outside 0 to 1 or after a note, a `<loop />` after anything, a sound with no notes, two sounds of one name, and a `<play>` or `<silence>` naming no sound all stop the game loading with a message saying where (`sound 'wall' > <note> 1: pitch="H2" is not a pitch; ...`).

### `<object>`

An object is anything that can be drawn, moved, or collided with: a ball, a paddle, a score readout, a title. Attributes: `name` (required, how other elements refer to it) and `class` (optional, a group label that collision rules and conditions can match, like a CSS class).

Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<sprite>` | yes | What the object looks like: one shape (see [Sprites](#sprites)). An object with an `<animation>` has several, each with a `name`; so does one with several [looks](#looks) |
| `<animation>` | no | Which of the object's sprites are shown, in what order, and for how many seconds each: see [Animation](#animation). Objects, and groups and their members |
| `<position>` | yes | Starting position, in pixels from the top-left: `<x>` and `<y>`, each a value. May use an object's own size by its name, `title.width` and `title.height`, to place it by its size, for example a text called `title` centered: `<x>window.width.center - title.width / 2</x>` |
| `<velocity>` | no | Starting velocity, in pixels per frame: `<x>` and `<y>`, each a value. Left out, the object stands still; a half left out is 0 (`<velocity><y>3</y></velocity>` falls straight down) |
| `<acceleration>` | no | A constant pull: `<x>` and `<y>`, each a value, added to the velocity once every frame before anything moves, for as long as the object is shown. Gravity is an acceleration with only a `<y>`. `<stop />` takes it away and a reset gives it back. Objects and groups (Donkey Kong's barrels fall); `<land />` is how a falling object stands on something |
| `<heading>` | no | The way the object faces, in degrees clockwise from straight up (0 is up, 90 is right). It lets `<turn>` and `<thrust>` work, and the sprite (of lines, a `<bitmap>` or an `<svg>`) is then drawn turned to the heading, to the nearest whole degree, so a `pixel` collision follows it. A collision `<reset />` puts it back. Objects only |
| `<drag>` | no | A value from 0 up to (not including) 1: the fraction of its velocity the object loses every frame, after its thrust is added. Objects only |
| `<facing>` | no | `up`, `down`, `left` or `right`: the way the object faces, without turning its picture. It follows the last `<move>`, `<hop>` or `<jump>` it makes, and a `<fire>` leaves the middle of that side moving that way, at the projectile's own speed: a man in a maze shoots the way he last walked, and an alien with `<facing>down</facing>` drops its bombs below it. A reset faces it the way it started. Objects and groups |
| `<maze>` | no | `<maze map="maze">` and its `<wall>`s: the object walks that [map](#maps)'s tiles, steered by `<go>`, `<hunt>` and `<wander>`. Objects and groups |
| `<hidden>` | no | `true`: the object starts out of play (not drawn, no collisions, not counted by a condition's `remaining`) until a `<release>`, a `<reveal>` or a `<fire>` brings it in. Objects, and groups (after `<velocity>`) |
| `<drawn>` | no | `false`: the object is in play (moves, collides, counts its timers) but is never drawn: an exit, a trigger zone, a `status` object that only holds variables. Objects and groups (after `<hidden>`); a [scene](#scenes) can change it on a piece it takes |
| `<collisions>` | no | Left out, the object touches nothing (as `<enabled>false</enabled>`). Otherwise `<enabled>` (`true`/`false`), an optional `<lockstep>` (`true`), an optional `<type>` (`box`, the default, or `pixel`), then zero or more `<collision>` rules. See [Collisions](#collisions) |
| `<actions>` | no | Named actions the object can perform, each `<action name="up"><move direction="up">step</move></action>`: the commands are what it does. States bind keys to these names |
| `<variables>` | no | Variables owned by this object, each `<variable name="score">0</variable>`. Other expressions refer to them as `objectName.variableName`, e.g. `paddle1.score` |
| `<timers>` | no | Things the object does on its own, every so many seconds or once after a while: see [Timers](#timers). Objects and groups |

An object with `class="projectile"` starts invisible (used for bullets).

### `<group>`

A `<group>` is several objects that share a description, written once: a lane of logs, a row of debris, the pads along the top of Frogger, a wall of bricks. It sits beside `<object>` under `<objects>`. It holds the parts its members have in common, in the same order as an object (`<sprite>`s, an `<animation>`, `<position>`, `<velocity>`, `<acceleration>`, `<facing>`, `<hidden>`, `<collisions>`, `<actions>`, `<variables>`, `<timers>`, each optional), then one or more `<member>`s, or, laid out in [columns and rows](#columns-and-rows), the `<row>`s, `<column>`s and `<cell>`s that change them, or one or more [groups inside it](#groups-inside-a-group). `<heading>` and `<drag>` are for objects only. Attributes: `name` (required) and `class` (optional, and every member's class).

A `<member>` says only what is its own: its `<sprite>`s and `<animation>`, a `<position>` and a `<velocity>`, and an optional `name`. **Whatever a member leaves out it takes from its group**, and after that it must be as complete as an `<object>` (a missing part is a load error that names the member). A `<position>` or `<velocity>` can give just an `<x>` or just a `<y>`, so a lane gives its row once and each member gives where along it it starts:

```xml
<group name="logrow3" class="logs">
  <sprite>
    <rectangle>
      <width>3 * cell - 2 * inset</width>
      <height>body</height>
      <color>color.brown</color>
    </rectangle>
  </sprite>
  <position>
    <y>3 * cell + inset</y>
  </position>
  <velocity>
    <x>-1</x>
  </velocity>
  <collisions>
    <enabled>true</enabled>
    <collision edge="horizontal">
      <wrap />
    </collision>
  </collisions>
  <member><position><x>20</x></position></member>
  <member><position><x>272</x></position></member>
  <member><position><x>524</x></position></member>
</group>
```

A member is an ordinary object: it is loaded as one object of its own (`logrow3.1`, `logrow3.2`, `logrow3.3`: the group's name, a dot, and its number counting from 1 in the order written, unless the member says `name="..."`), it is drawn, moved and collided with on its own, and each `<wrap />` or `<die />` happens to that one member. A group only shares what is written. Members are drawn in the order written, at the place in the file where the group stands. The group's name means every member wherever the file names an object (`<show object="logrow3" />`, `object="pads"` in a rule or condition, `<reset object="pads" />`), and a member can be named on its own (`object="logrow3.2"`). A group with `<lockstep>true</lockstep>` in its `<collisions>` moves as one block (the invaders).

A `<sprite>` a member gives changes the group's sprite of the same `name` (or, with no name, the group's with none). Of the same shape it need give only what changes, and keeps the rest: `<sprite><rectangle><width>10</width></rectangle></sprite>` is the group's rectangle, 10 wide. Of another shape, or as `<line>`s, it is a sprite of its own and must be complete. A name the group has not got adds a sprite.

#### Groups inside a group

A group can hold `<group>`s instead of members or cells, and they take from it the same way a member takes from its group: **whatever an inner group leaves out it takes from the one around it**. Frogger's ten lanes all wrap round at the sides, so the `<wrap />` is written once, in a group of all ten; the river's lanes share a log and the road's a car, each said once in a group between:

```xml
<group name="lanes">
  <collisions>
    <enabled>true</enabled>
    <collision edge="horizontal"><wrap /></collision>
  </collisions>
  <group name="river" class="logs">
    <sprite><rectangle><width>4 * cell - 2 * inset</width><height>body</height><color>color.brown</color></rectangle></sprite>
    <group name="logrow2">
      <position><y>2 * cell + inset</y></position>
      <velocity><x>1.5</x></velocity>
      <member><position><x>100</x></position></member>
      <member><position><x>502</x></position></member>
    </group>
    <group name="logrow3">
      <sprite><rectangle><width>3 * cell - 2 * inset</width></rectangle></sprite>
      ...
    </group>
  </group>
  <group name="traffic" class="hazard"> ... </group>
</group>
```

- **What replaces, what adds.** One thing (a half of the `<position>`, `<velocity>` or `<padding>`, `<columns>`, `<rows>`, the `<animation>`, `<acceleration>`, `<facing>`, `<hidden>`, the `class`) is replaced by the inner group's. A `<sprite>` changes the outer's of its name, as a member's does, so `logrow3` above gives only a width. A list adds: the inner group's collision rules run after the outer's, and its `<actions>`, `<variables>` and `<timers>` join the outer's (an action or variable of the same name replaces it). An inner `<collisions>` says its own `<enabled>`, and `<lockstep>` and `<type>` when it changes them.
- **Names.** The members are named after their own group (`logrow3.2`, or `top.3.1` for a cell), and every group's name means all the members inside it: `<show object="lanes" />` shows all ten lanes, `object="river"` in a rule is every log. A group with `<lockstep>` makes a block of the members of each innermost group: in Breakout the wall gives the bricks' columns, gap, size and colour, the grey row above is one inner group and the marching block another.
- A group that holds groups has no `<member>`s, `<row>`s, `<column>`s or `<cell>`s of its own (a load error otherwise). Groups nest as deep as is wanted.

#### Columns and rows

A group can lay its members out instead of listing them: `<columns>` and `<rows>` (whole numbers) make a cell for each place, and an optional `<padding>` (`<x>` and `<y>`) is the gap between them. They come first in the group, before what the cells share, and the group's `<position>` is the top left of the first cell. A cell is a slot the size of its row's sprite; the cells of a row follow one another left to right, and the rows follow one another down, each as tall as its sprite.

```xml
<group name="bricks" class="bricks">
  <columns>9</columns>
  <rows>6</rows>
  <padding><x>5</x><y>5</y></padding>
  <sprite>
    <rectangle>
      <width>width</width>
      <height>height</height>
      <color>color.red</color>
    </rectangle>
  </sprite>
  <position><x>margin</x><y>margin / 2</y></position>
  <velocity><x>0</x><y>0</y></velocity>
  <collisions>...</collisions>
  <row number="2"><sprite><rectangle><color>color.orange</color></rectangle></sprite></row>
  <row number="3"><sprite><rectangle><color>color.yellow</color></rectangle></sprite></row>
  ...
</group>
```

Each cell is an ordinary object named after the group, its column and its row, counting from 1 (`bricks.5.3`), and they are made the top row first, left to right. After what the group says, three tags change what the cells they pick have, from the general to the particular:

| Tag | Picks | Can change |
|---|---|---|
| `<row number="...">` | rows: `2`, several (`2 4 6`), `odd` or `even` | `<sprite>`s, `<animation>`, `<velocity>`, `<padding>`, `<variables>` |
| `<column number="...">` | columns, the same way | the same; its `<padding>` gives only an `<x>` |
| `<cell row="..." column="..." name="...">` | one cell, and may name it (`name` optional) | `<sprite>`s, `<animation>`, `<velocity>`, `<variables>` |

A row's `<padding>` is its own gap: `<y>` the gap above it, `<x>` the gaps between its cells. A column's `<x>` is the gap to its left. The first row and column have no gap before them. A sprite changes the group's as a member's does (above), so a row of another color says just `<color>`; a cell whose sprite is another size than its row's sits in the middle of its slot, and its neighbours stay where they are. A variable a row gives is that row's (Breakout's top rows could score more). A `<cell>` wins over the rows and columns that pick it, but a row and a column (or two rows) that pick one cell must not change the same thing in it: which should win is not something to guess, so the game does not load, and the message says to change it in one of them, or in a `<cell>`. `<collisions>`, `<actions>` and `<timers>` stay the group's, so every cell acts the same way. Space Invaders is one group of 11 columns and 5 rows: the squids' two sprites and animation are the group's, and `<row number="2 3">` and `<row number="4 5">` give the crabs' and octopuses' rows of text and color. Frogger's lane markings are a group of 13 by 4 with no changes at all.

A group either lists `<member>`s or has `<columns>` and `<rows>`, not both. Whole numbers are needed because they say how many cells there are and what they are called, which is settled as the file is read.

### Maps

A `<map>` is level data drawn as text: each `<row>` is a row of tiles, one character to a tile, so the file shows the level as it looks. It stands in any `<objects>` (the game's, a state's, a scene kit's or a scene's own) beside objects and groups:

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

`<tile>` is the size of a tile (each a value above 0), `<position>` the map's top left, and the rows must all be as wide. Then come the groups and objects that say what each character is, each picking one with `char="x"`:

- **A group** has a member on every tile of its character, in the middle of the tile, named by number in reading order (row by row, each left to right): `dots.1`, `dots.2`, ... A group on a map has no `<position>`, layout, `<member>`s, rows, columns or cells (the map says where), but everything else a group has. It can hold groups, which pick characters themselves and change its sprites by name, as [inner groups](#groups-inside-a-group) do: Pac-Man's walls are one group, class `wall`, of fourteen pieces, each a character and a bitmap (a line through the middle of the tile, joined to the walls beside it: `-` `|`, the corners `r` `7` `L` `J`, the tees `T` `^` `E` `3`, the ends `[` `]` `n` `u`).
- **An object** picks a character that is on exactly one tile, where it starts, in the middle; it has no `<position>`.

A character nothing picks is an empty tile; two pieces may not pick one character. Rows are trimmed like any text, so an empty tile at either end of a row is a character nobody picks (Pac-Man's `,`), not a space. The map's name means everything on it, as a group's does (`<show object="maze" />`), outermost of its names. A mistake says where: a row of the wrong width, a character picked twice, a group whose character is on no tile, an object whose character is on more than one, a `char=` outside a map or a `<position>` on one.

**Walking a map.** An object with `<maze map="maze">` (after `<facing>`; objects and groups) walks that map's tiles: it keeps to the middle of its lane, chooses its way only in the middle of a tile, and never goes onto a tile that holds a wall to it. What is a wall is the `<wall>`s inside: `<wall class="wall" />` is every object of that class on the map that is in play; `<wall class="door" sprite="normal" />` is one only while the walker shows its `normal` [look](#looks), which is how a ghost's eyes go in through the door of the ghost house that stops the ghost. Off the map (a tunnel to the window's edge) it goes straight on, and the window's edge can `<wrap />` it round. How it chooses comes from the last of three commands it was given:

- `<go direction="left">speed</go>`, on a key (in an object's action, on the press): it turns that way at the first tile where it can, carrying on straight until then, stops in the middle of a tile when a wall is ahead, and turns straight round at once. Pac-Man.
- `<hunt object="pacman">`: at every tile, of the ways open other than back, the one whose next tile is nearest a target; a dead end turns it round. That is how the arcade's ghosts chase, with no search of the maze, and how they get stuck is part of how they play. The target is the middle of the nearest one in play of that name or group; `<ahead>4</ahead>` tiles in front of the way that one faces; with `<with object="blinky" />`, as far past that point again as the partner is short of it (Inky closing in from the other side to Blinky); and with `<retreat object="corner">8</retreat>`, while the hunter is within 8 tiles of what it hunts, the middle of the corner instead (Clyde). `<speed>` is required, the rest optional, in that order. A tie goes up, then left, then down, then right.
- `<wander><speed>1.25</speed></wander>`: of the same ways, one at random (a frightened ghost).

The walker faces the way it goes, and turns its `<heading>` if it has one (Pac-Man's mouth). `<hunt>` and `<wander>` work on an object's timer, a path's step or a collision with another object. A `<reverse />` turns it round where it is; a `<stop />` or a reset stops it, and it stands until told again; on a [path](#paths) or in a jump it is the path or the jump that moves it, and it goes on choosing from the tile it is left on. `<go>`, `<hunt>` and `<wander>` on an object that walks no map stop the load. A tile is found from the middle of the walker, so a walker of any size works; it should go less than half a tile a frame.

### Sprites

A `<sprite>` holds one shape. Colors are named (`color.red`, below); a `<color>` left out is `color.white`. A sprite may be given a `name`, which an object needs only when it has several sprites: for an [`<animation>`](#animation) to choose between, or as [looks](#looks) for `<become>`.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | A filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | A filled rectangle |
| `<text>` | one or more pieces, `<content>` (a fixed label), `<number>` and `<letter>` (values), in any order, then `<size>`, `<color>` | Text. `<content>PONG</content>` is written as is. `<number>paddle1.score</number>` shows a number: when the value is exactly one `owner.variable` it is live and redraws when that variable changes; any other value (`paddle1.score + 1`, a `<random>`) is worked out once. `<letter>` shows a number as a letter, 0 is A and 25 is Z, and round again either way (26 is A, -1 is Z), live the same way: initials are three variables an `<inc>` and a `<dec>` change. The pieces are put together with a space between them, except that letters side by side make one word: `<content>SCORE</content><number>ship.score</number>` reads `SCORE 120`, three `<letter>`s and a `<number>` `ABC 120`. In a [group](#group), a piece that names the group's variable (`table.score` in the group `table`) shows each member's own |
| `<image>` | `<path>`, then `<flip>` (`horizontal` or `vertical`, optional) | An image file |
| `<line>` (one or more) | `<from>` and `<to>` (each an `<x>` and a `<y>`), then `<color>` and `<thickness>` (both optional) | Straight lines, all in one sprite. See [Lines](#lines) |
| `<bitmap>` | one or more `<row>`s of `.` and `*`, then `<scale>`, `<color>` and `<flip>` (all optional) | A picture written as rows of text. See [Bitmaps](#bitmaps) |
| `<svg>` | `<path>`, then `<x>`, `<y>`, `<width>`, `<height>` (all four or none), `<scale>`, any number of `<hide>`, and `<flip>` (all optional) | A drawing, or a part of one, from an SVG file. See [SVG pictures](#svg-pictures) |

### Lines

A sprite of one or more `<line>`s is a drawing. Each `<line>` goes from a point to a point, in pixels from the **top left of the sprite** (so the coordinates are never negative), and may have a `<color>` (default `color.white`) and a `<thickness>` in pixels (a value, at least 1, default 1):

```xml
<sprite>
  <line><from><x>10</x><y>2</y></from><to><x>20</x><y>2</y></to></line>
  <line><from><x>20</x><y>2</y></from><to><x>25</x><y>7</y></to><color>color.red</color><thickness>2</thickness></line>
</sprite>
```

The lines are drawn once, when the game loads, in the order written (a later one covers an earlier one where they meet), into a bitmap the size of what was drawn: as far right and down as the furthest endpoint plus the thickness, from 0, 0. Endpoints are rounded to whole pixels and a line is drawn with no gaps, stamping a square of its thickness along it. Every pixel nothing was drawn on is transparent. That bitmap is the sprite: the window backends show it as a picture, its size is the object's size (so `name.width` and `name.height` work as for any shape, with no window needed), and a collision of [type pixel](#collisions) looks at the same pixels, so what is drawn is exactly what is tested. A `<random>` in a coordinate is drawn once, so the picture and its size agree.

A sprite of lines cannot be mixed with another shape. One object can be a whole drawing: Lunar Lander's moon is one object of fifteen lines, its lander another, its pad a single thick line.

A shape repeated in rows and columns (Breakout's bricks, the invaders) is a group laid out in [columns and rows](#columns-and-rows).

### Pictures the engine draws

A sprite of `<line>`s, a `<bitmap>` and an `<svg>` are three ways of describing a picture, and all three go the same way, once, when the game loads (`game_expr::buildSpriteParams`):

1. **Read** what the file describes: line ends and thicknesses, rows of characters, or a part of a drawing.
2. **Draw** it into the engine's own `Bitmap`, the pixels every window backend uploads and shows and a `pixel` collision tests. No backend draws one itself, so they all show the same picture.
3. **Flip** it, if a `<bitmap>` or an `<svg>` has `<flip>horizontal</flip>` or `<flip>vertical</flip>`: the pixels are mirrored and the size stays the same.
4. **Turn** it, for an object with a `<heading>`: the picture is kept, and what is shown is the kept picture turned to the heading, a whole degree at a time, made again only when the heading moves by a degree. A drawing of lines is kept as lines and drawn again at each heading, which keeps its edges sharp; a `<bitmap>` or `<svg>` has its finished pixels turned.

An `<image>` is not drawn by the engine (each backend loads the file), and its `<flip>` is done by the backend.

### Bitmaps

A sprite of one `<bitmap>` is a picture written as rows of text, one character to a pixel: a period (`.`) is clear and an asterisk (`*`) is solid, drawn in the sprite's `<color>` (default `color.white`). Every character becomes a block of `<scale>` by `<scale>` real pixels (a value, a whole number of at least 1, default 1), so a few characters make a chunky sprite:

```xml
<sprite>
  <bitmap>
    <row>..*...*..</row>
    <row>..*****..</row>
    <row>.**.*.**.</row>
    <row>*********</row>
    <scale>5</scale>
    <color>color.green</color>
  </bitmap>
</sprite>
```

That picture is 45 pixels wide and 20 tall. The rows are measured from the top left, must all be the same length and may hold nothing but `.`, `*` and the characters of the bitmap's other colors: a space or any other character, an empty row, or rows of different lengths stop the load with a message that names the row and the character (`row 3 of a bitmap has 'o' as character 2`). A bitmap is drawn once, when the game loads, into the same kind of bitmap a sprite of [lines](#lines) is, so everything said there holds for it: its size is the object's size (`name.width` and `name.height` work with no window), every window backend shows it as a picture, and a collision of [type pixel](#collisions) tests exactly the solid pixels. The cells of a group that draw the same bitmap share the one picture. After the `<color>` of its `*`s, a bitmap can have more colors, each `<color char="w">color.white</color>`: every `w` is a pixel of that color, so a ghost's body is its color and its eyes white with blue pupils (`<color char="b">color.darkblue</color>`). The character is anything but `.` and `*`, one to a color. A sprite that changes a group's (an inner group's, a member's) gives only the colors it changes. On an object with a `<heading>` the picture is drawn once like this and then that finished picture is turned to the heading the object faces, to the nearest whole degree (360 headings, 0 and 360 being the same): each pixel of the turned picture takes the one pixel of the original that lies under it, so chunky pixels stay chunky and nothing is blended. The object keeps the original and the one picture it shows, and draws a new one only when its heading moves to another whole degree. Every heading comes out as the same square, big enough for the picture at any angle (for a bitmap 11 by 8 characters at `<scale>` 5 that is a square of 68), and that square is the object's size, as with [lines](#lines).

### SVG pictures

A sprite of one `<svg>` is a drawing from an SVG file, made into a picture when the game loads, the same way a `<bitmap>` is. `<path>` is the file, found like an `<image>`'s (relative to the folder the program runs from). `<scale>` is how many real pixels one unit of the drawing is (a value above 0, default 1; it need not be a whole number), where a unit is what the drawing's own `width` and `height` are written in:

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

`<x>`, `<y>`, `<width>` and `<height>` (values, in the drawing's units, all four or none) pick a part of the drawing to take, so one sheet of sprites can serve many objects: the part goes to the top left of the picture, and the picture is `<width>` by `<height>` times `<scale>`, rounded up to whole pixels. Leave them all out for the whole drawing. Each `<hide>` is the id of an element of the drawing to leave out (and everything inside it), for a sheet that carries a backdrop or a grid of its own; an id nothing has is ignored.

The picture keeps the drawing's own colors, antialiased, with soft edges coming out as partly transparent pixels, so there is no `<color>`. Everything after the drawing is the same as for a `<bitmap>`: the picture is made once, its size is the object's size (`name.width` and `name.height` work with no window), every window backend shows it as a picture (the engine draws it itself, with a library used only by `svg.cpp`, so no backend loads an SVG: SFML and raylib could not, SDL2 and OpenGL could), the cells of a group that take the same part share the one picture, it can be a frame of an [animation](#animation), and a collision of [type pixel](#collisions) tests its pixels, a pixel counting as solid if anything at all was drawn on it, so a faint edge counts. A picture cut close round what is drawn also has a close-fitting box for the collisions that are not `pixel`. A missing or unreadable file, a part with no size or entirely outside the drawing, and a scale of 0 or less stop the load with a message that names the object and the file.

An `<svg>` on an object with a `<heading>` is turned the way a `<bitmap>` is (the finished picture, a pixel at a time, nothing blended), so the soft edges of the drawing turn into hard stepped ones; drawing the SVG again at each heading would be smoother, and is not done. The drawing is fixed when the game loads: it cannot change size or color while the game runs. What can be drawn is whatever lunasvg draws; the shapes, polygons, strokes, `<use>`, `<defs>`, opacity and rotation of the shipped sprite sheet are what the tests exercise.

### Animation

An object normally has one `<sprite>`. One that has several gives each a `name` and follows them with an `<animation>`, which says which are shown, in what order, and for how long:

```xml
<object name="crab">
  <sprite name="open"> <bitmap> ... </bitmap> </sprite>
  <sprite name="closed"> <bitmap> ... </bitmap> </sprite>
  <animation>
    <interval>1</interval>
    <frame sprite="open" />
    <frame sprite="closed" />
  </animation>
  <position>...</position>
  ...
</object>
```

`<interval>` is how many **seconds** each picture is shown (a value above 0, so `0.25` works), and each `<frame sprite="name" />` picks one of the object's own sprites by its name. There are at least two frames, and the same sprite may come up more than once (`open`, `closed`, `open`, `wide`). The object starts on the first frame and goes round and round. Seconds are turned into frames of the game when the game loads, with the window's `<framerate>` (a second at 60 frames a second is 60 frames, and a window with no framerate has nothing to count seconds in); like the speeds in the game, which are in pixels per frame, the animation counts frames of the game and does not read a clock, so a game that runs slower than its framerate animates slower too.

The frames must all be pictures (a `<bitmap>`, an `<svg>` or `<line>`s, not a circle, rectangle, text or image) of the same size. Every sprite the object has must be shown by its animation. (Several named sprites with no animation are not an error: they are the object's [looks](#looks).) An object with a `<heading>` can be animated too: whichever frame is showing is drawn at the heading the object faces, and the frames must come out as the same square when turned (equal sized bitmaps always do).

Only an object that is shown by the current state and in play (not dead, not hidden) moves on, so a pause or a menu holds the picture where it was, and a `<reset />` puts it back on the first picture, from the start of its time. Every cell of a group has a count of its own, and they all start together, so a block of aliens changes picture as one. A collision of type pixel tests the picture that is showing. In a `<group>`, a member, row, column or cell that gives sprites changes the group's of the same names, and one that gives an `<animation>` has that instead of the group's; the names in an animation are looked up among the sprites it ends up with. Space Invaders is the example: its aliens are one group whose rows change the two named bitmaps, so the block marches, bounces and steps down together while each kind flaps on its own sprites. A picture drawn the same way for many cells (the same rows, scale and color, or the same part of an svg) is drawn once and shared.

### Looks

An object with several named sprites and no `<animation>` has **looks**: it starts as the first, and `<become sprite="name" />` shows another from then on (a reset brings back the first). `<become sprite="blue" object="row1" />` changes every object of that name or group at once, and with `from="normal"` only those of them showing `normal`: a power pellet turns the ghosts blue but leaves an eaten one's eyes as they are. A collision rule, about another object or about a screen edge, can carry `sprite="name"`: it runs only while its object shows that look, looked at as the rule comes (a rule before it may have changed it), which makes a look a simple state. Frostbite's rows of ice are groups that start white; a floe's rule

```xml
<collision object="bailey" sprite="white">
  <become object="row1" sprite="blue" />
  <inc variable="bailey.score">10</inc>
  <reveal object="igloo" />
</collision>
```

runs once, when Bailey lands on a white row, and turns the whole row blue. A look can also count hits: Breakout's grey top row is a group of two looks, `whole` and `cracked`, with `<collision object="ball" sprite="cracked"><die /></collision>` and then `<collision object="ball" sprite="whole"><become sprite="cracked" /></collision>`. Rules run in the order written and each looks at the look as it is then, so the cracked rule comes first; the other way round, the first hit would crack a brick and the second rule would take it out at once. Looks are pictures of any shape kind (a rectangle in another colour, another bitmap); an object with a `<heading>` cannot have them, and a look cannot be a text showing a `<number>`.

A look is also how one state is told something by the state before it. Space Race, Combat and Freeway each have one `wins` state whose `winner` has a look per player; the condition that ends the game picks the look and then pushes the state, and the full reset that starts the next game brings back the first look:

```xml
<condition object="rocket2" variable="score">
  <atleast>2</atleast>
  <become object="wins.winner" sprite="player2" />
  <push state="wins" />
</condition>
```

A text whose look changes is measured again, so a position worked out from its width (`window.width.center - winner.width / 2`) keeps it centred.

### Timers

```xml
<timers>
  <timer>
    <every><random min="0.3" max="1.4" /></every>
    <reverse />
  </timer>
  <timer>
    <every>drop1</every>
    <fire object="bombs1" />
  </timer>
</timers>
```

A `<timer>` (with an optional `name=`, by which a [scene](#scenes) copied with `from=` puts another in its place) goes off after a number of seconds, `<every>` time (again and again) or once `<after>` that long, and runs the commands after it. An object's timers (in its `<timers>`, after `<variables>`; a group's are every member's) count while it is shown and in play; a state's (after its `<conditions>`) count while it is the current state, so a pause stops them and they carry on afterwards. A reset starts them over. The interval is worked out again every time the timer starts over, after its commands have run, so `<random>` waits a different time each round (Kaboom's bomber changes direction whenever he likes) and an expression can follow a variable the commands change. Seconds are turned into frames of the game with the window's `<framerate>`; a timer never waits less than one frame, and `<after>0</after>` goes off on the first frame it counts.

An object's timer with `sprite="blue"` follows that [look](#looks): it counts only while the object shows it, and starts over whenever a `<become>` gives the object that look, even the look it already shows. So looks and timers make modes. Pac-Man's ghosts have a `blue` timer `<after>0</after>` that turns them round and sets them wandering, and another `<after>fright</after>` that makes them `normal` again; a second pellet gives the blue ghosts `blue` again, and their fright starts over; each ghost's `normal` timer `<after>0</after>` sets its hunt going again, whenever it becomes normal. A state's or a scene's timer has no look to follow.

On an object, a timer can `<fire>` (from that object: enemies firing back), `<reverse />`, `<move>`, `<die />`, `<stop />`, `<release>` and do a bare `<reset />` (the object back where it started); anywhere, it can `<push>`, `<pop />`, `<reset object>`, `<trigger>`, `<inc>`, `<dec>`, `<play>`, `<silence>`, `<become>` and `<reveal>`, and in a state a bare `<reset />` resets the whole game. A clock the player can see is a timer that `<inc>`s a variable a text shows; Frostbite's cold is a state timer taking a degree a second off the thermometer.

### Paths

```xml
<paths>
  <path name="dive">
    <speed>3</speed>
    <step><x>-24</x><y>-24</y><play sound="dive" /></step>
    <step><x>80</x><y>120</y><fire object="bombs" /></step>
    <step><x>-56</x><y>568</y></step>
    <home />
  </path>
</paths>
```

A `<path>` is a way through the window, written once in the game's `<paths>` (after `<sounds>`, before `<objects>`) and flown by any object that `<follow>`s it. It has a `<speed>` (pixels a frame, above 0), an optional `<start>` (an `<x>` and a `<y>`: the object is put there when it sets off; without one it sets off from where it is), then its legs, flown one after the other: a `<step>` moves the object by its `<x>` across and `<y>` down from wherever that step began, in a straight line at the path's speed, and runs the commands after its `<x>` and `<y>` as it sets off (the follower's own, as on one of its timers: `<fire>`, `<play>`, `<inc>`); `<home />` flies it back to where it started the game, its `<position>`, from wherever it has got to. At the end of the path the object comes to rest. Every number is worked out once, when the game loads, and any variable can be used.

`<follow path="dive" />` (in an object's collision rule, action or timer) sets that object off; `<follow path="in" object="aliens"><stagger>0.15</stagger></follow>` (anywhere, and the only form in a state) sets off every object of that name or group that is in play, in the order written, each `<stagger>` seconds after the one before (0 if left out), waiting at the path's start until its turn: a line of aliens flying in one behind another. An object already on a path finishes it first, so a timer that keeps asking does nothing until it is done. A reset, a `<die />`, a `<stop />` and a `<reveal>` end a path. A follower is moved by its velocity like anything else (the path sets the velocity each frame), so it collides on the way, and a `<wrap />` carries it round without changing what is left of the step: Galaxian's dive adds up to the window's height plus the alien's down, which is exactly what wrapping at the bottom takes off again, so a diver that misses comes back in from the top and ends in its own place.

### Commands

Commands are tags, and where they are meaningful is what the table says. Any list of them (inside a `<collision>`, an `<action>`, an `<input>`, a `<condition>` or a `<timer>`) is run in the order written, for example `<inc variable="paddle2.score" />` then `<reset />`.

| Command | Where it is meaningful | Effect |
|---|---|---|
| `<bounce />` | collision | Reverses velocity away from the touched edge |
| `<deflect>angle</deflect>` | collision (another object) | A bounce off the other object at an angle set by where it hit: the object's middle level with the middle of the touched side leaves straight out, level with either end of that side (or past it) leaves at `angle` degrees towards that end, and in between in proportion. The speed is kept. The content is a value from 0 up to, not including, 90. Pong's ball has `<deflect>45</deflect>`, so a hit on the top of a paddle goes back up at up to 45 degrees ([collisions](designs/20-collisions.md)) |
| `<stick />` | collision | Clamps the object inside the screen edge it touched and stops only the velocity heading into that edge; the other axis keeps going, so an object pressed against the bottom wall still slides left or right. Re-applied after the frame's move, so a stuck object never ends a frame outside the screen |
| `<wrap />` | collision (screen edge) | Once the object has gone completely off the screen through that edge and is still heading that way, puts it back in from the opposite edge, one window width (or height) plus its own size along, so anything spaced along a lane keeps its spacing. While any of it is still in view nothing happens |
| `<ride />` | collision (another object) | Lends the object the touched object's velocity for that frame, on top of its own: a frog on a log rides along with it. Worked out again every frame from whatever is still being touched, so an object that steps off is at rest |
| `<carry />` | collision (another object) | The object picks up the one it touched and takes it wherever it goes, held where it was when picked up, until a `<drop />`. The carried thing is lent the carrier's motion, so it is swept and meets what is on its way (a key reaching a gate), then put exactly where it is held. One thing at a time: picking up another puts the first down where it is, and taking one that another object carries takes it from that one. The carrier or the carried thing out of play (dead, not shown), or either reset, and it is put down. Adventure's hero: `<collision class="item"><carry /></collision>` |
| `<track />` | object `<action>` triggered by an `<input axis>` | The object goes where the control points ([Input](#input)) |
| `<drop />` | object `<action>` (on the press), collision, object timer | Puts down what the object carries, where it is. It is not picked up again until the two have stopped touching, so the carrier can walk off it |
| `<die />` | collision | Disables the object's collisions and hides it; it stops moving and being drawn until something brings it back (`<fire>` re-launching a bullet, `<reset />`) |
| `<reset />` | collision (screen edge or another object) | Puts the object back as it started: its starting position and velocity (the velocity is left alone while a held key is moving it), with any `<random>` in its start, or in its `<variables>`, drawn again. Pong's ball serves anew after every point |
| `<reset />` | state input or condition | Full game reset: every object's position, velocity, variables, visibility and collisions go back to how they started (a bullet in flight is put away, a dead alien is back), and the state stack collapses to the first state; every [scene](#scenes) starts again, and each state's first scene is current |
| `<reset object="name" />` | anywhere: collision, state input, condition, timer | Resets that one object (or every member or cell of a group, for its name) the same way. From a collision it acts on another object: Kaboom's bomb hitting the ground resets the wave's whole pool of bombs, the explosion |
| `<reset scene="name" />` | anywhere: collision, state input, condition, timer | Puts every object of that [scene](#scenes) back as it started (with its `<random>`s drawn again), and its timers too; what was carried off from it comes back. Whether it is current does not change. `<reset way="out" />` resets the scene the current scene's way `out` leads to: Berserk's exits put the next room back as it started, so it has its robots, just before entering it |
| `<enter scene="name" />` | anywhere: collision, state input, condition, timer | Makes that scene the current one of its state: the scene left stops where it is, and is found as it was when entered again. What one of the state's own objects carries goes with it. The name is a scene's, plain inside its state (`room2`) or as a path (`playing.room2`); a scene that does not exist stops the load. `<enter way="east" />` instead enters where the current scene's [way](#scenes) `east` leads, so one kit piece leads somewhere different in every scene; one of `scene=` and `way=` is given |
| `<inc variable="owner.variable" />` or `<inc variable="owner.variable">amount</inc>` | anywhere: collision, condition, state input, timer | Adds 1, or the amount (a value, so an expression works), to that variable and refreshes any text bound to it |
| `<dec variable="owner.variable" />` or `<dec variable="owner.variable">amount</dec>` | anywhere: collision, condition, state input, timer | Takes 1, or the amount, off that variable and refreshes any text bound to it. The variable may go below zero; a condition with `<atmost>` is what notices it has run out |
| `<rank variable="owner.variable" into="group" />` | anywhere `<copy>` can go | The object `variable=` names takes its place in a table of the best, the group's members in order, by that variable ([`<keeps>`](#keeps)) |
| `<copy variable="owner.variable" into="owner.variable" />` | anywhere: collision, condition, state input, timer | The variable `into` names takes the number `variable` holds as the command runs, and any text bound to it shows it: a best score following the score (see [`<keeps>`](#keeps)). Both are an object's variable, checked when the game loads |
| `<move direction="up">step</move>` (also `down`, `left`, `right`) | collision, or an object `<action>` | In a collision: shifts the object (or everything in lockstep with it) once. In an object action: sets a held-key velocity (see [Input](#input)). The content is a value |
| `<hop direction="up">distance</hop>` (also `down`, `left`, `right`) | object `<action>` | A one-shot jump of `distance` pixels for each press of the key (see [Input](#input)). The content is a value |
| `<jump direction="down"><distance>90</distance><seconds>0.35</seconds></jump>` (also `up`, `left`, `right`) | object `<action>` | A jump that takes time: once for each press, the object travels the `<distance>` that way over `<seconds>` (0.3 if left out), both values, in a straight line. While it is in the air it touches no other object (it cannot be hit, ride or drown until it lands, and it passes over what is in between), and a second jump waits for the landing; the frame it lands it meets what it landed on. A jump that would land off the screen is not made. Frostbite's jump between rows of ice |
| `<land />` | collision (another object) | Coming down onto the other object's top (touching its top edge, not going up), the object stands on it: it is put on the top, its fall stops, and it is on the ground until the next frame's landing is worked out, which a `<leap>` needs. A leap ends, and the keys held then decide the way across. Touching any other side, or going up, does nothing, so a platform can be walked past at its ends and jumped up through from below. With a falling `<acceleration>` an object stands on whatever it has this rule for, walks off an open end and falls to the next. Donkey Kong's girders, for Jumpman and the barrels |
| `<leap>height</leap>` | object `<action>` | A jump up under the object's own pull, for each press: it leaves the ground at the speed that rises about `height` pixels (a value) against its `<acceleration>`, and falls back until a `<land />`. Only from the ground and not on a ladder; a press in the air does nothing. The way across is the one it had at the take-off: a key pressed or let go in the air does not steer it until it lands. The object needs an `<acceleration>` with a `<y>` above 0 and a `<collision>` with `<land />`, or the game does not load. Donkey Kong's jump |
| `<climb direction="up" class="ladder">step</climb>` (or `down`) | object `<action>` | Held like a `<move>`: while the object stands at an object of that class (its middle over it, its feet between that object's top and bottom) and is on the ground, it gets on, lined up with its middle, and goes `step` pixels a frame (a value) that way. On it, nothing pulls it, it does not `<land />`, a key left or right does not take it off, and with no key held it stays put. Reaching the top or the bottom it gets off, standing there. A ladder runs from the top of one platform to the top of the next. The class must be one some object has |
| `<reverse />` | anywhere an object is acting: collision, timer | The object's velocity turns round, both ways at once. Kaboom's bomber changing direction on a timer |
| `<become sprite="name" />` or `<become sprite="name" object="other" />` | anywhere (in a state's input, condition or timer it needs `object=`) | The object, or every object of that name or group, shows its [look](#looks) of that name, and the timers that follow that look start over. With `from="look"` (and `object=`), only those showing that look change |
| `<reveal object="name">count</reveal>` | anywhere | Brings back the first `count` (default 1) out-of-play objects of that name or group (hidden at the start, or taken out by `<die />`), each where it started and at its own starting velocity. Where `<release>` puts them in the middle of the object running it, `<reveal>` puts them where they belong: Frostbite's igloo built a block at a time, a door that appears, a fish that swims back |
| `<accelerate direction="up" burn="fuel">amount</accelerate>` (also `down`, `left`, `right`) | object `<action>` | A thruster: while the key is held, the object's velocity changes by `amount` (a value) every frame in that direction, where `<move>` would set the velocity itself. The optional `burn` names one of the object's own `<variable>`s; 1 is taken off it every frame the thrust is on (and a text bound to it follows), and while it is 0 or below the thrust does nothing. See [Input](#input) |
| `<stop />` | collision (screen edge or another object) | The object comes to rest where it is and stays there: its velocity goes to 0, and it is no longer pulled by its `<acceleration>` or pushed by a held `<accelerate>` or `<move>`, until a reset gives the acceleration back. A landing |
| `<turn direction="left">degrees</turn>` (or `right`) | object `<action>` | While the key is held, the object's heading changes by that many degrees every frame (left is counterclockwise). Needs a `<heading>` on the object |
| `<thrust burn="fuel">amount</thrust>` | object `<action>` | Like `<accelerate>`, but along the way the object faces instead of along an axis; `burn` works the same. Needs a `<heading>` on the object |
| `<release object="name">count</release>` | collision (screen edge or another object) | Puts the first `count` (default 1) out-of-play objects of that name or group back in play, centered on the object running the rule, at their own starting velocity. Fewer left in the pool gives what is there. This is how a rock breaks into smaller ones |
| `<turns><player state="name" /> ...</turns>`, `<pass />`, `<out />` | state input, condition or state timer | Players [taking turns](#taking-turns): start them, pass to the next, and put this one out |
| `<demo />` | state input, condition or state timer | The game's recorded `<demo>` plays, a new game played by itself, until a key is pressed: the [attract mode](#attract-mode) |
| `<push state="name" />` / `<pop />` | state input or condition | Push a state / pop back. The state named must be one of the game's. `<pop />` with only the first state left does nothing |
| `<pop state="name" />` | state input or condition | This state goes and that one takes its place, so moving on (Air-Sea Battle's time up to its result) does not grow the stack; moving between places of one state is `<enter scene>` |
| `<trigger object="name" action="up" />` | state input | Runs one of that object's named `<action>`s. The object and the action must exist |
| `<play sound="name" />` | anywhere: collision (screen edge or another object), object `<action>`, state input or condition | Plays one of the game's [sounds](#sounds). In an action or an input it plays on the press, not on the release; a sound that loops plays while the key is held. The game only asks for the sound; the engine plays everything asked for once the frame's keys, collisions and conditions have run |
| `<silence sound="name" />` | anywhere `<play>` can go | Stops one of the game's sounds if it is playing: above all one that [loops](#sounds), which otherwise plays until something silences it |
| `<follow path="name" />` or `<follow path="name" object="name"><stagger>seconds</stagger></follow>` | without `object=`: an object's collision rule, action or timer; with it, anywhere | The object, or every object of that name or group in play (each `stagger` seconds after the one before), flies that [path](#paths) |
| `<chase object="name"><speed>1.5</speed><near>100</near></chase>` | an object's timer, or a collision with another object | The object heads straight for the middle of the nearest one in play of that name or group, at `<speed>` pixels a frame (a value), facing that way; within `<near>` pixels of it (left out: 0) it stops instead. It acts once: a timer that repeats it keeps it on the trail, and how often is how quickly it turns. With none in play it carries on as it was. Berserk's Evil Otto |
| `<aim object="name" />` | an object's timer, or a collision with another object | The object turns to the nearest one in play of that name or group: its next `<fire>`s go straight at where that one's middle was, at any angle, from the shooter's middle, at the projectile's own speed, and its `<facing>` turns to the nearest of the four ways (a `<heading>` turns exactly). Kept until it aims again, is reset, or a key moves, hops or jumps it. Berserk's robots aim, then fire |
| `<go direction="left">speed</go>` | object action | A [maze walker](#maps) turns that way at the first tile it can and keeps going, `speed` pixels a frame, until a wall stops it; back the way it came, at once. On the press |
| `<hunt object="name"><speed>2</speed><ahead>4</ahead><with object="partner" /><retreat object="corner">8</retreat></hunt>` | an object's timer, a path's step, or a collision with another object | A [maze walker](#maps) chooses, at every tile from now on, the open way (never back) whose next tile is nearest its target, as the arcade's ghosts do |
| `<wander><speed>1</speed></wander>` | an object's timer, a path's step, or a collision with another object | A [maze walker](#maps) takes an open way at random at every tile, never back |
| `<fire object="projectile" />` | object action, or an object's timer | Launches the named projectile object; from a shooter that has aimed, along the aim (see `<aim>`); from a shooter with a `<facing>`, from the middle of the side it faces, moving that way at the projectile's own speed; otherwise from the shooter's top-center (the middle of the projectile over the middle of the shooter's top edge), moving with the projectile's own `<velocity>`. A projectile is not drawn, moved or collided with until it is fired, and is put away again by `<die />` (hitting a target, or `edge="all"`). The name may be a `<group>`: the first member that is out of play is the one launched, so a group of four is four shots in flight. A shooter with a `<heading>` fires from its nose, along the heading, at the speed of the projectile's `<velocity>`; with none, one projectile name can only be in flight once |

A command the engine does not know, or one missing an attribute it needs, stops the game loading with a message that says where (`object 'ball' > <collisions> > <collision>: unknown command <explode>`). So does a command that names something the game does not have: a state (`<push state="pasued" />`), an object or one of its actions (`<trigger>`), a projectile (`<fire>`), an object to reset (`<reset object="...">`), a sound (`<play>`, `<silence>`), an object to `<reveal>`, or a look to `<become>` that the object has not got. These are checked once every object and state has been built, so a name used before the thing it names appears in the file is fine. When the name is a likely typo of one the game has, the message says which (`(did you mean 'paused'?)`).

The schema accepts some mistakes the engine could only ignore, so the loader refuses them too, each with a message saying what to write instead:
- a key the engine does not have (`<input button="spcae">`: "there is no key named 'spcae' (did you mean 'space'?). The keys are: ...");
- a color that is not one of the `color.` names, in a sprite, a `<line>` or the window's `<background>`;
- a command in a place where it does nothing (a `<move>` in an `<input>`, which needs a `<trigger>` of an `<action>`; a `<push>` in a `<collision>`; a `<stick />` against another object), listing what can go there;
- an `<inc>` or `<dec>` of a variable no object has (`<inc variable="score">` with `score` among the game's fixed `<variables>`, or `o.lifes` where `o` has `lives`), listing the object's variables.

**Colors:** `color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`, and the muted `color.grey`, `color.darkgrey`, `color.lightgrey`, `color.brown`, `color.orange`, `color.purple`, `color.darkblue`, `color.darkgreen`, `color.forestgreen`. Any other name is fully transparent.

### `<keeps>`

A full `<reset />` (a new game) puts every object back as it started. The objects `<keeps>` names are passed over: they keep what they hold, their variables, place and looks, from one game to the next, until the program is closed. Each `<keep object="..." />` names an object or a group (every member of it); one that names nothing stops the load. `<reset object="...">` still resets a kept object by name.

Galaxian keeps its best score this way. `best` is an object of the game's own, shown by the title and by play, and a condition of play copies the score into it whenever the score is above it:

```xml
<objects>
  <object name="best">
    <sprite><text><number>best.score</number> ... </text></sprite>
    ...
    <variables><variable name="score">0</variable></variables>
  </object>
</objects>
<keeps>
  <keep object="best" />
</keeps>
...
<condition object="ship" variable="score">
  <above object="best" variable="score" />
  <copy variable="ship.score" into="best.score" />
</condition>
```

**Kept on disk.** `xgecli`, `xgegui` and a generated program write what a game keeps (each kept object's variables and look) to `saves/<game>.xml` in the games folder whenever a new game starts and when the program closes, and read it when the game is loaded (`Engine::keepIn`), so a best score or a table lasts from one run to the next. A game that keeps nothing writes nothing; the tests write nothing unless they ask. Delete the file to start again.

**A table of the best.** A group whose members are the places, the best first, and `<rank variable="entry.score" into="table" />`: the entry (an object with a `score` and whatever else a place holds, its initials) takes the first place whose `score` is below its own, all its variables going into that place, and the places from there move down, the last one falling off; a tie goes below. Whether a score makes the table is a condition, `<above object="table.5" variable="score" />`. Galaxian keeps its best five: a game over above the fifth asks for initials, one state a letter (W and S, or Up and Down, change it, Space goes on), then ranks the entry and starts a new game:

```xml
<group name="table">
  <sprite><text><letter>table.first</letter><letter>table.second</letter><letter>table.third</letter><number>table.score</number> ... </text></sprite>
  <variables> first, second, third, score: 0 </variables>
  <member><position><y>tabletop</y></position></member>
  ...
</group>
...
<input button="space d right">
  <rank variable="entry.score" into="table" />
  <reset />
</input>
```

### Taking turns

Two players (or more) take turns the way the arcade machines did: each plays a game of their own, and the machine swaps them. `<turns>` starts it, with a `<player>` for each player:

```xml
<input button="num2">
  <push state="playing" />
  <turns>
    <player state="player1" />
    <player state="player2" />
  </turns>
</input>
```

Each player's game starts as the game is when `<turns>` runs: every object but the ones the game [keeps](#keeps) (a best score, a table, shared by everyone), every state's and scene's timers, and the current scenes. The state current then (`playing`) is the one the turns are played in. Player 1 plays first. A `<player>`'s `state=` (optional) is pushed whenever that player's turn begins, its timers started over: a screen saying whose turn it is, over their game drawn still, that pops itself after a while.

- **`<pass />`**: the turn goes to the next player still in. This player's game is put away exactly as it is, the next one's comes back as they left it, the states above the one the turns are played in are taken off, and their `state=` is pushed. With one player, or no one else still in, nothing happens. Galaxian passes whenever a ship is lost.
- **`<out />`**: this player is out of the game, and the turn passes the same way. When another player is still in, nothing after `<out />` in its list runs; when no one is, the turns are over and what follows runs: `<out /><reset />` is "the next player, or a new game". With no turns it does nothing, so the same file plays one player too.

`<turns>`, `<pass />` and `<out />` go where a state's commands go (an `<input>`, a `<condition>`, a state's `<timer>`), so a swap never happens in the middle of a frame's moves; a collision counts what happened in a variable and a condition passes. A full `<reset />` ends the turns.

### Attract mode

Left alone on the title, an arcade machine plays itself. A game's `<demo>`, after its `<states>`, is a game recorded as it was played: the seed its random numbers came from, how many frames it ran, and each key pressed and released, on the frame it happened:

```xml
<demo>
  <seed>20261016</seed>
  <frames>1200</frames>
  <press button="space">30</press>
  <release button="space">34</release>
  <press button="left">200</press>
  ...
</demo>
```

`<demo />`, where a state's commands go, plays it: a new game, its random numbers drawn from the seed, its keys pressed on their frames, so it plays out exactly as it was recorded. A key pressed at the window ends it (and does nothing else), as does the recording running out; then a new game starts, what the game [keeps](#keeps) as it was before the demo (a demo's score is never the best). Galaxian's title runs `<demo />` on a timer after ten seconds:

```xml
<timers>
  <timer><after>10</after><demo /></timer>
</timers>
```

`xgecli <game> --record` makes one: it plays the game from a new seed and, when the window is closed, writes the `<demo>` into the game file, in place of the one there (`withDemo`, in `game_xml.h`, keeping the file's line endings). A `<demo />` in a game with no `<demo>` stops the load. Galaxian's was written by hand; one recorded replaces it.

### `<state>`

A state is a mode of the program: a menu, the game being played, a pause screen, a game-over screen. Attribute: `name`. Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<objects>` | no | The state's own objects and groups, written as the game's are. They are in play (drawn, moving, colliding, counting their timers) whenever the state is the current one, with no `<show>`, in every scene of it |
| `<scenes>` | no | The places the state is played in, one current at a time: see [Scenes](#scenes) |
| `<shows>` | no | A list of `<show object="name" />`: objects of the game (or of another state) drawn and updated while the state is current, as if they were its own. `<show set="name" />` shows a shared [show set](#show-sets). `<show state="name" />` draws that state's objects, and those of its current scene, **still** underneath: not moving, not colliding, their timers not counting, so a pause or a game over is drawn over the frozen game |
| `<inputs>` | yes | A list of `<input button="key">`, each holding the commands that key runs, e.g. `<push state="playing" />` or `<trigger object="paddle1" action="up" />`. `keys="name"` takes a shared [key set](#input) first; the state's own `<input>`s go over it |
| `<conditions>` | no | A list of `<condition>`, checked every frame. `<condition set="name" />` checks a shared set there. See [Conditions](#conditions) |
| `<timers>` | no | Timers that count while this is the current state. See [Timers](#timers) |

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

What is drawn is drawn in two layers: first what is shown still (`<show state>`), then what is in play, each in the order the objects are written, a later one over an earlier (`drawFrame` in `window.h`, which a generated game calls too). A state's own objects come after its scenes' objects, so they are drawn over them.

### Scenes

A state that is played in several places (Berserk's rooms, Pitfall's jungle screens, Kaboom's waves, Adventure's castle and field) holds them as `<scenes>`: one place at a time is its **current scene**, and only its objects are in play, alongside the state's own.

```xml
<state name="playing">
  <objects>
    <object name="hero"> ... </object>                  <!-- in every scene -->
  </objects>
  <scenes>
    <objects>                                           <!-- the kit: pieces scenes take -->
      <object name="topwall" class="wall"> ... </object>
      <group name="robots" class="robot"> ... </group>
    </objects>
    <scene name="room1">
      <ways>
        <way name="out" scene="room2" />                <!-- where this room's way out leads -->
      </ways>
      <variables>
        <variable name="pace">2</variable>                <!-- read inside as pace -->
      </variables>
      <objects>
        <object name="topwall" />                       <!-- taken as it is -->
        <group name="robots">                           <!-- taken and changed -->
          <velocity><x>pace</x></velocity>
        </group>
        <object name="exit"> ... </object>               <!-- this scene's own -->
      </objects>
      <conditions> ... </conditions>                    <!-- checked while it is current -->
      <timers> ... </timers>
    </scene>
    <scene name="room2" from="room1">                   <!-- all of room1, but its pace -->
      <variables>
        <variable name="pace">3</variable>
      </variables>
    </scene>
  </scenes>
  ...
</state>
```

- **The kit** is an optional `<objects>` at the top of `<scenes>`, written like any other. Its pieces are never in play themselves. A scene takes one by naming it in its own `<objects>`: empty, as it is; with parts inside, changed: whole `<sprite>`s (the sprite of that name is replaced), the `<x>` or `<y>` of its `<position>` or `<velocity>`, `<hidden>`, `<drawn>` and its `<variables>`. A piece is taken by a name written in the kit: an object's, a group's (the whole group) or a member's own `name`; a numbered member (`rocks.2`) cannot be taken alone. Anything else a scene's `<objects>` holds is its own, written in full.
- **Each scene gets its own copies.** Two scenes that take the robots each have robots of their own, which move, die and are counted on their own.
- **`from="room1"`** makes a scene start as a copy of an earlier one: its objects (what that scene took and owned), its `<variables>`, its `<ways>`, its `<conditions>` and its `<timers>`. It can then take and change the objects like kit pieces, and add to the rest: what it writes is added, and a variable, way, condition or timer **of the same name** as a copied one takes that one's place, where it was (order matters for conditions: the first that holds runs). A condition or timer is named with `name=` (`<condition name="quota" ...>`, `<timer name="launch">`); unnamed ones are only ever added, and two of one name in one list stop the load. A copied condition or timer is read in the new scene: its plain names are that scene's variables and objects, and its ways that scene's ways. So Kaboom's, Megamania's and Atlantis's later waves say only their numbers, their pictures and where their way `next` leads, and the last wave writes a condition of the same name that wins; Atlantis's last wave also puts its own `launch` timer in place of the copied one. A copied condition or timer cannot be taken away, only replaced.
- **A scene's `<variables>`** are its own numbers, read by the objects it holds (a robot's speed in room2 is room2's `pace`).
- **`<conditions>` and `<timers>`** in a scene are checked and counted only while it is current, after the state's own.
- **What a condition counts.** A condition with `class=`, or with `object=` naming what every scene has (`robots`, which is `playing.robots`), looks at the state's own objects, the current scene's and the game's: a wave is cleared when its own enemies are gone, whatever is left in the others. Named by its path (`room2.robots`, `playing.room2.robots.3`), it is that scene's, counted wherever the game is, so a condition can ask whether another room still has robots.
- **Moving between scenes** is the command [`<enter scene="room2" />`](#commands), run wherever the move happens: an exit's collision rule, a condition on the score, a timer. What an object of the state carries goes with it into the new scene.
- **Ways** let a kit piece lead somewhere different in each scene that takes it. A scene's optional `<ways>` (first, before its `<variables>`) name where each of its ways leads: `<way name="east" scene="screen2" />`. A command then says the way instead of the scene: `<enter way="east" />` enters the scene the *current* scene's `east` leads to, and `<reset way="east" />` puts that scene back as it started. So Pitfall's side strips and Berserk's exits are written once, in the kit, and each screen or room says only where its ways go. A way's scene is one of the same state's, by name or path; a command inside a scene that names a way the scene does not have, a way no scene has, two ways of one name in a scene, and a way to a scene that does not exist all stop the load. A way the current scene lacks at run time (a state-level rule, say) does nothing, with a warning.
- **A scene left stays as it was** (positions, variables, what was dropped there) until it is entered again. `<reset scene="room1" />` puts it back as it started. A game's `<reset />` starts every scene again and makes each state's first scene current.

### Names

A state's own objects are named by their path: `playing.hero`; a scene's by the state, the scene and the name, `playing.room1.robots`, and a member as before, `playing.room1.robots.3`. A scene's variable is `playing.room1.pace`. A name written in the file is looked for from the inside out, from where it is written: the scene's own, then its state's, then the game's, and last as the end of exactly one path (`room1.robots`, or `hero` for `playing.hero` anywhere). So the rules inside a scene say `robots` and mean their own, and a label at the game level can show `room2.tally.hits`.

Each object of a scene also answers to `state.name` (`playing.robots`): the robots of every scene of the state. A state-level rule that says `robots` means them all, whichever scene is current. Where a short name could still mean two things (a `rock` in two states), the shortest path wins, and two as short stop the load (`'rock' could be playing.rock, practice.rock; write the path of the one meant`). A name given twice where both can be seen (a scene's object named like one of its state's, or a state's or a scene's named like one of the game's) stops the load too, as do two scenes of one name, the mistakes with [ways](#scenes) above, a `from=` naming no scene before it, and a take of a piece neither the kit nor the `from=` scene has.

### Show sets

Objects several states show are written once, as a named `<shows>` set at the top of `<states>` (with the `<keys>` and [`<conditions>`](#conditions) sets, before the first `<state>`), holding `<show>`s like a state's. A state takes a set with `<show set="name" />` among its own `<show>`s, one set to a `<show>`, and adds whatever else it shows. A set can take a set written before it. Now that states own their objects, a set is for objects of the game that several states show: Air-Sea Battle's two end screens (time up, then the result) each draw the battle still and take TIME UP and "spacebar to play again" from one `end` set:

```xml
<states>
  <shows name="end">
    <show object="timeup" />
    <show object="again" />
  </shows>
  <state name="result">
    <objects> ... </objects>                  <!-- its own words -->
    <shows>
      <show state="battle" />
      <show set="end" />
    </shows>
    ...
  </state>
```

A `<show>` names an `object`, a `set` or (in a state's own `<shows>`) a `state`, one of them; a set named before it is written, or two sets with one name, are turned away when the game loads.

What a state shows is drawn in the order the objects are written in the file, a later one over an earlier, whatever order its `<show>`s are in.

The first state in the file is the starting state. States form a **stack**: `<push state="name" />` pushes a state, and `<pop />` pops back to the previous one. The starting state is never popped: a `<pop />` with only it left does nothing. `<pop state="name" />` puts that state in place of the current one (the starting state too), for a game that moves from one screen to the next (Air-Sea Battle's clock running out puts its result screen in its place); places inside one mode are [scenes](#scenes).

## Input

An `<input button="w"><trigger object="paddle1" action="up" /></input>` names a key and the commands it runs. Objects never mention keys, and states never mention what an action does, so remapping a key means editing one attribute in one state. `button` can name several keys separated by spaces, `<input button="a left">`, for the same commands.

**Key sets.** Bindings several states share are written once, as a named `<keys>` set at the top of `<states>` (before the first `<state>`), holding `<input>`s like a state's. A state takes one or more with `<inputs keys="walk">` (several names separated by spaces, each over the one before), and its own `<input>`s go over the set's, key by key, so a state can use the set and still give a key a job of its own:

```xml
<states>
  <keys name="walk">
    <input button="a left"><trigger object="hero" action="left" /></input>
    <input button="d right"><trigger object="hero" action="right" /></input>
    <input button="p escape"><push state="paused" /></input>
  </keys>
  <state name="playing"> ... <inputs keys="walk" /> ... </state>
  <state name="practice"> ... <inputs keys="walk" /> ... </state>
```

No shipped game uses one now: each game's keys are bound by its one playing state.

The shipped games share one convention: Space starts, pauses and unpauses, and plays again after a game over (in games where Space fires, such as Space Invaders, Astrosmash and Depth Charge, pausing is P or Escape, and Space still unpauses); player one plays on W, A, S and D (the arrow keys are a second way in the one-player games, and player two's keys in the two-player ones). The games written after Pong and Breakout also pause on P, and most of them on Escape; in Pong and Breakout Escape leaves the settings screen (S on the main menu).

Key names are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more listed in `xge/lib/source/keycode.cpp`.

**The mouse and gamepads.** Their buttons are keys too, bound the same way: `mouse.left`, `mouse.right`, `mouse.middle`, and for two pads `pad1.a`, `pad1.b`, `pad1.x`, `pad1.y`, `pad1.lb`, `pad1.rb`, `pad1.back`, `pad1.start`, `pad1.up`, `pad1.down`, `pad1.left`, `pad1.right` (and `pad2.` the same), named for an Xbox pad. A pad's directions are its d-pad, or its left stick pushed past halfway. Kaboom, Breakout and Pac-Man bind `pad1.start` beside Space and the pad's directions beside the keys (`<input button="a left pad1.left">`).

**Controls that point.** `<input axis="mouse.x">` (or `mouse.y`, `pad1.x`, `pad1.y`, `pad2.x`, `pad2.y`) runs its `<trigger>` whenever that control moves, and a `<track />` in the action it triggers puts the object where the control points: its middle at the mouse, or as far across the window as the stick is pushed, kept whole inside the window, along the axis's own direction (`.x` across, `.y` up and down). That is the Atari's paddle controller. It runs only when the control moves, so the keys can still move the same object while the mouse rests, and the mouse off the window moves nothing. An axis input runs only `<trigger>`s, and goes in a state's own `<inputs>`, not in a `<keys>` set. Kaboom's bucket and Breakout's paddle follow the mouse:

```xml
<action name="steer"><track /></action>
...
<input axis="mouse.x"><trigger object="bucket" action="steer" /></input>
```

The window reports the mouse and the pads each frame (`Window::controls`), and `Engine::handleControls` turns what changed into presses, releases and tracking. Every window library reads both, but Qt (`xgegui`'s own) reads the mouse only, while the view has the keyboard.

While a key bound to a `<move>` in an object action is held, that direction's step is recorded. On every key change the velocity along that key's axis is recomputed from the two directions of the axis, so holding Down and tapping Up cancels out and releasing Up resumes Down. Left/right and up/down are independent axes, so two keys can make a diagonal, and a key on one axis leaves the other alone: walking does not stop a fall. During a `<leap>` the way across is left as it was until the landing.

The current state decides what a held key means. When the state changes, keys that are still down stop driving whatever the old state bound them to, and start driving what the new state binds them to. In Breakout, holding Left in `playing` and pressing Space stops the paddle, because `paused` binds only Space; unpausing while Left is still down moves it again with no re-press, and a Left first pressed during the pause starts moving it on unpause. Letting go of a key always stops what it was driving, unless another key still down drives the same action (an action bound to both W and Up keeps going while either is held). Only continuous bindings (an action's `<move>`) resume this way; state changes and one-shot commands such as `<fire>` run only when the key is actually pressed, so holding Space through the main menu does not pause the game. Alternatives that were considered are in [designs/08](designs/08-input.md#held-keys-across-state-changes).

**Accelerate.** `<accelerate direction="up">amount</accelerate>` in an object `<action>` is held like a `<move>`, and is resumed the same way after a state change, but it adds to the velocity instead of setting it: each frame the key is down, every direction held contributes its `amount` (so opposite thrusters cancel, and two directions make a diagonal push), on top of the object's own `<acceleration>`, before the frame's move. What was gained stays: let go of the key and the object drifts on at the speed it reached. `<thrust>` is the same along the object's `<heading>` (one per action, so a forward and a back held together add up, and `<turn>` is worked out first, so a thrust is along the heading it has just turned to), and `<turn>` changes the heading by a fixed amount a frame while held; an object's `<drag>` then takes a fraction of the speed away each frame, so thrust has a top speed. With `burn="variable"` each thruster that is on takes 1 off that variable of the object's every frame, and does nothing once it is gone.

**Hop.** `<hop direction="up">distance</hop>` (and `down`, `left`, `right`) in an object `<action>` is a one-shot jump, not a held move. Pressing the key queues a hop of `distance` pixels; the next frame's move makes it before collisions are worked out, so the object is judged where it lands, and it is refused (the object stays where it is) if it would leave the window. Holding the key does nothing more and releasing it does nothing, and a state change never repeats it: only continuous bindings are resumed, so a key held through a pause has to be pressed again to hop. Two hops asked for in one frame keep the later one, so a hop is always one step in one direction.

## Collisions

Each object has `<collision>` rules. A rule's content is a list of command tags, run in order. There are two kinds:

- **Screen edge:** `<collision edge="left">`, `"right"`, `"top"`, `"bottom"`, plus the groupings `"vertical"` (top and bottom), `"horizontal"` (left and right) and `"all"`. Several rules that touch the same edge all run, in the order written. An edge is no object, so an edge rule with `class=` or `object=` stops the game loading; it can carry `sprite=` and `unless=` (below).
- **Object against object:** name what the rule applies to with `class="..."` (a kind of other object, matched against that object's `class` attribute) and/or `object="..."` (one named object), for example `<collision class="bricks"><die /></collision>`. A `<collision>` with no selector at all applies to anything. A `<collision>` with `edge` is always a screen-edge rule.

Detection (pure geometry, `CollisionDetector`) is kept apart from response (`CommandExecutor`). Object-against-object collisions are **swept**: each object moves along its own path for the frame, and the detector finds the moment two of them first touch (rectangles as boxes, a circle against the other object's box with rounded corners), so a small or fast object cannot jump over a thin one between frames. The earliest touch in the whole frame is handled first: everything moves up to that moment, the pair's rules run, and the rest of the frame is played with whatever velocities they left, so a bounce spends the remaining part of the frame heading away. Each pair reacts at most once per frame, and the pairs are those of what is in play when the frame begins: something a rule brings into play (a `<release>`d burst) touches nothing until the next frame. An object that no reaction changed ends the frame exactly one whole step on, however many reactions the others had (its place is worked out from where it began, not by adding up the pieces). A turned picture or a changed text has its new size at once (`Window::prepare`, called from `Engine::step`), not after the next draw, so the frame's edges and touches see it. The detector reports which edge of the other object was touched, and each side of the pair then sees the edge from its own point of view. Screen edges are still checked by position, before the move.

**Type.** `<type>` in `<collisions>` says how the object's shape is tested: `box` (the default: a rectangle as a rectangle, a circle as a circle, everything else as its bounding box) or `pixel`. A pair in which either object is `pixel` is first swept as boxes, exactly as above, and then looked at more closely: from the moment the boxes touch, the pair is walked along its path half a pixel at a time until some pixel drawn by one lies on a pixel drawn by the other, and that moment (found to a fraction of a pixel) is the hit. If the pixels never meet, there is no hit, however long the boxes overlapped. Pixels are asked about at their centres, at the positions the objects really are at. A `pixel` object is its sprite's bitmap (a sprite of [lines](#lines), a `<bitmap>` or an `<svg>`); an object of any other type in the pair counts as solid all over its shape. A circle or a rectangle can be `pixel` too, which is solid as itself; text and images cannot (a load error), since what they look like is only known to a window backend. Each object says its own type, so a game with a pixel lander and pixel terrain gives both `<type>pixel</type>`. The same test decides `unless=` (is it touching something of that class right now).

The edge reported for a pixel hit is the one the motion came in through (the side of the other object that the two were moving toward each other across), or the boxes' edge when they were already touching; a pixel touch knows where pixels met, not which way a surface faces, so `<bounce />` on a pixel hit reflects along the axis of the relative motion.

Rules that apply:

- Only objects that are **shown** in the current state and have `enabled="true"` take part.
- **What a rule can do.** Against a screen edge: `<bounce />`, `<stick />`, `<reset />`, `<die />`, `<stop />`, `<move>`, `<inc>`, `<dec>` and `<wrap />`. Against another object: `<bounce />`, `<deflect>`, `<die />`, `<stop />`, `<reset />`, `<move>`, `<inc>`, `<dec>`, `<ride />` and `<carry />`. Both: `<reverse />`, `<reset object>`, `<become>`, `<reveal>`, `<release>`, `<drop />`, `<play>` and `<silence>`.
- **`sprite`.** A rule, about another object or a screen edge, can carry `sprite="name"`: it runs only while its own object shows that [look](#looks). `<stick />` and `<wrap />` are about a screen edge and do nothing in a rule about another object, and `<ride />`, `<carry />` and `<deflect>` are about another object.
- **`<slower>` and `<faster>`.** A rule about another object may begin with `<slower>N</slower>` and/or `<faster>N</faster>` (values), before its commands: it only runs while the object's own speed (the length of its velocity, with what it rides) is **under** N (`slower`) or **N or more** (`faster`) at the moment of the touch. The speed is taken once, before any of the object's rules for this touch run, so a rule that `<stop />`s it does not change which rules after it are run. Two rules about the pad with the same number, one `slower` and one `faster`, are a landing and a crash: exactly N counts as fast. A screen-edge rule has no speed filter (a load error).
- **`unless`.** A rule, about another object or a screen edge, can carry `unless="class"`: it is passed over while the object is, at that same moment, touching something in play of that class. Frogger's river is `<collision class="water" unless="logs"><dec variable="frog.lives" /><reset /></collision>`: water costs the frog a life, unless it is also on a log. It looks at where things are right now, so it does not matter which of the two touches was handled first. On an edge, `<collision edge="bottom" unless="paddle"><die /></collision>` loses a ball at the bottom only when no paddle is under it.
- A pair is skipped unless at least one of the two is moving, and unless one of them has a rule that answers to the other (its `class`/`object`, or a rule with no selector). An object counts as moving if its velocity is not zero, if it is riding something, or if it has just hopped or is jumping, so an object that lands somewhere by hopping or jumping is judged there even though nothing else in the pair moves. An object in the middle of a `<jump>` is in no pair at all.
- `<lockstep>true</lockstep>` puts all members or cells of a `<group>` in lockstep: they share a lockstep number. Cells in lockstep never collide with each other, `<move>` in a collision moves all of them, and one hitting the left or right screen edge with `<bounce />` moves the whole block (this is how the invaders march). Being in lockstep changes none of the geometry: every cell is swept on its own, so a bullet only ever meets the cells that are still alive, and there is no bounding box around the block.
- Every cell of a group in columns and rows is its own object, named after the group with its column and row counted from 1: a group called `aliens` has `aliens.1.1`, `aliens.2.1`, ... `aliens.11.5`. The group's name still works where the whole set is meant: `<show object="aliens"/>`, a rule or condition `object="aliens"`, `<reset object="aliens" />`; a cell can be named on its own (`object="aliens.3.2"`). A listed group's members are named by number (`logrow3.2`); see [`<group>`](#group).

## Conditions

```xml
<condition class="paddle" variable="score">
  <atleast>15</atleast>
  <push state="gameover" />
</condition>
```

Checked once per frame while the state is current. An optional `name=` lets a [scene](#scenes) copied with `from=` put another in its place. It fires when any object matching `class` and/or `object` (both optional, combined with AND) has a variable named `variable` that has reached the `<atleast>` value (greater than or equal). The first match runs its commands and stops checking for that frame. The threshold is an ordinary value, so it is not limited to 0-255 and can be an expression.

The other forms:

```xml
<condition class="aliens">
  <remaining>0</remaining>
  <push state="gameover" />
</condition>
```

`<remaining>` counts objects instead of reading a variable. It fires when no more than that many of the matching objects are still in play, that is still visible (`<die />` hides an object). `0` means they are all gone, which is how Space Invaders is won: `class="aliens"` covers every cell of the group, and `object="aliens.3.2"` would watch a single one. `<atmost>` reads a variable from above: `<condition object="frog" variable="lives"><atmost>0</atmost>...</condition>` fires when the variable has fallen to that value or below, which is how a lives counter that goes down with `<dec>` ends the game (Frogger). `<above>`, `<below>` and `<equal>` are more than, less than and the same as. A condition uses exactly one of `<atleast>`, `<atmost>`, `<above>`, `<below>`, `<equal>` or `<remaining>` (the schema enforces this).

**Comparing two variables.** Each of the five tests on a variable can compare it with another variable instead of a number: `variable=` on the test names it and `object=` whose, and it is read every frame. Without `object=` it is the tested object's own, so `<condition class="player" variable="score"><above variable="best" />` compares each player with themselves. Air-Sea Battle says who won this way:

```xml
<condition object="gun1" variable="score">
  <above object="gun2" variable="score" />
  <become object="result.outcome" sprite="yellow" />
  <pop state="result" />
</condition>
<condition object="gun2" variable="score">
  <above object="gun1" variable="score" />
  <become object="result.outcome" sprite="white" />
  <pop state="result" />
</condition>
<condition object="gun1" variable="score">
  <equal object="gun2" variable="score" />
  <become object="result.outcome" sprite="draw" />
  <pop state="result" />
</condition>
```

A test with `variable=` has no number as well; an object or variable the game does not have stops it loading, saying which (with a suggestion for a likely typo), and `<remaining>` takes only a number. If the filter matches no object at all, the game warns when it loads, since the condition would fire at once. Ending the game with a `gameover` state and starting over with a bare `<reset />` on that screen works as in Pong; `<reset />` also brings the dead aliens back.

Besides changing the state, a condition can `<inc>` and `<dec>` a variable, `<become>`, `<reveal>`, `<play>`, `<silence>` and `<trigger>`. A condition that changes the variable it reads can count and start over: Frostbite's `<condition object="bailey" variable="blue"><atleast>4</atleast>` turns the four rows of ice white again and takes 4 off the count.

**Condition sets.** Conditions several states check are written once, as a named `<conditions>` set at the top of `<states>` (with the `<keys>` and `<shows>` sets, before the first `<state>`), holding `<condition>`s like a state's. A state takes a set with `<condition set="name" />` among its own conditions, and the set's conditions are checked right there, in order, as if written in its place; since the first condition that holds is the only one run in a frame, where the `<condition set>` stands matters. A set can take a set written before it. A `<condition set>` has nothing else, no test, no filter and no commands. No shipped game uses one now (the conditions several places shared are the playing state's, checked in every scene of it); two modes that end the same way would:

```xml
<states>
  <conditions name="ending">
    <condition object="status" variable="lives">
      <atmost>0</atmost>
      <push state="gameover" />
    </condition>
  </conditions>
  <state name="playing">
    ...
    <conditions>
      <condition set="ending" />
    </conditions>
  </state>
```

## What happens when a game runs

1. **Parse and validate.** The XML backend loads the file. If the file names a schema, it is validated: Xerces does full XSD validation ("strong"); the other three backends use a small built-in validator for the subset of XSD this project uses ("weak", `xsd_lite`). `printGame()` reports which one ran.
2. **Evaluate.** exprtk evaluates every value once (an expression text, or a value tag such as `<random>`, which is drawn here). Objects (a group's members and cells among them), their variables, states and sounds are built. Nothing here needs a window or a sound device.
3. **Open the window and the sound.** `Engine` creates the window backend and the audio backend (if the sound library will not start, it prints a warning and the game plays silently), makes every sound into samples and hands them to the audio backend, measures each object's real size for drawing and collisions, finishes the position of any text or image that uses `objectName.width` or `objectName.height`, then pushes the first state. The program prints the game twice, once before this step (sizes and size-dependent positions shown as unknown) and once after.
4. **Loop.** An object is *shown* (in play) when it is visible and is the current state's own, or its current scene's, or one the state `<show>`s. Each frame: read key changes and run the current state's bindings for them; count the frame towards the next picture of every shown object that has an [animation](#animation); count the frame on every [timer](#timers) of a shown object, of the current state and of its current scene, and run the ones that go off; set the velocity of every shown object on a [path](#paths) for its part of the path (starting its next leg, and running that leg's commands, when one is done); change every shown object's velocity by its acceleration and held thrust; run the screen-edge rules; make any queued hops; lend what is carried its carrier's motion; move every shown object by its velocity, and by what it is being carried at, running the object-against-object rules at each touch on the way (per frame, not scaled by time); put each carried thing where it is held; check conditions (the state's, then its current scene's); play the sounds the frame asked for, and start or stop its loops; clear; draw what a `<show state>` shows still, then the shown objects, each but those that are not `<drawn>`; present.

## Backends

| Job | Interface | Implementations |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml_document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keyboard | `Window` (`window.h`) | SFML3 (default), Raylib, SDL2, OpenGL (GLFW) |
| Sound | `Audio` (`audio.h`) | SFML3 (default), Raylib, SDL2, None (silent) |

A program is built with some of these, not all: SFML 3 and Xerces alone by default, every one with `-DXGE_ALL_BACKENDS=ON` or `-DBUILD_TESTING=ON`, each other one with its own `XGE_WITH_<NAME>` (see the [readme](../readme.md#backends)). The enums always name all of them, so a name on the command line or in a settings file still means something; what is missing is only the library. Each factory says what it has: `WindowFactory::available(backend)`, `availableBackends()`, `defaultBackend()` (SFML 3, Xerces, SFML 3's sound, or the first built when those are left out) and `name(backend)`, and the same on `XmlDocumentFactory` and `AudioFactory` (`None` is always built). `create()` for a backend that is not built throws, saying which are. A backend's source files are only compiled when it is built, and its `XGE_WITH_<NAME>` definition is what the factory looks at.

One more library is used by the engine itself and is not a backend: lunasvg draws the SVG files an `<svg>` sprite names, into the same kind of bitmap a `<bitmap>` makes, so no window backend knows about SVG ([pictures](designs/30-pictures.md)). Only `svg.cpp` includes it.

`Game` and `Engine` only ever see the interfaces. Each interface has a factory that is the single place that knows every implementation (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`).

The sound library is chosen apart from the window library, and any goes with any (OpenGL, which has no sound of its own, included). `Game` never makes a noise: `<play>` only asks for a sound (`Game::requestSound`), or switches on one that loops (`Game::loopingSounds`), and `Engine` hands what a frame asked for to its `Audio`, starting the loops that came on (`Audio::loop`) and stopping those that went off or were silenced (`Audio::stop`), so a game runs and is tested with no sound device. The engine makes every sound's samples itself (`synthesize()`, `sound.h`), so a sound is the same whichever library plays it; a backend only hands the samples to its library: an `sf::SoundBuffer` and `sf::Sound` (SFML 3), a `Sound` made with `LoadSoundFromWave` (raylib, which opens its sound device apart from its window; a loop is a `Music` stream read from the samples written as a WAV file in memory, handed more of each frame by `Audio::update`), or a small mixer of its own in an SDL audio callback (SDL2, which has no mixer without SDL_mixer). The SDL2 window and the SDL2 sound each start and stop only their own part of SDL, so either can go first. `Engine::replaceAudio()` swaps the sound library of a running game, which is how the Options dialog changes it, and `Engine::silence()` stops what is playing, which `xgegui` does when the game is paused. `NullAudio` plays nothing: the tests use it, and so does `-a none`. Build-time dependency selection is in `scripts/cmake/` (see the `FORCE_LOCAL_*` options in `options.cmake`).

Every window backend opens a window of its own, which is what `xgecli` uses. `xgegui` shows the game in one of two layouts ([cli](designs/52-cli.md)): in one window, drawn by a renderer of its own that draws with Qt (`QtWindow`, the default), or in two, where the main window holds only the controls and the tree and the game is in a window of its own, opened by the chosen library (SFML3, SDL2, raylib or OpenGL) or by the Qt renderer. `Engine::replaceWindow()` swaps the window of a running game for another without touching the game, which is how the Options dialog changes the video library. A front end that has paused the game but keeps its window calls `Engine::pump()` so the window can still be moved and closed, and `Engine::isWindowOpen()` tells it when the user closed it.

Nothing in `xgegui` uses OpenGL through Qt, so a library's OpenGL context is the only one on the thread. (The game view was a `QOpenGLWidget` once, and the two kinds of context, which Qt tracks by its own record, drew into each other; see [cli](designs/52-cli.md).)

## Building: library and programs

The engine (everything in `xge/lib/source/` and `xge/lib/include/`) is a library, the `xgelib` CMake target. Two programs use it: `xgecli` (`xge/cli/`) and `xgegui` (`xge/gui/`, the Qt application, built only when Qt 6 is found), and so do the tests (`xgetest`, in `xge/tests/`). Every project has the same layout, a folder with `source/` and `include/` in it; the games and everything they are made of are in `games/` (`descriptions/`, `assets/`, `generators/`), and the schema is `xge/xgedef.xsd`. `xgedata` is the target that copies `games/` next to the programs, in the same folders, and the schema to the top of the output folder; a game names its schema as `../../xge/xgedef.xsd`, right in the repository, and the engine looks for it by its file name in the folders above the game when it is not there (`findSchema`, `data_folder.h`). `--generate` copies the engine's files from `xge/lib/` in the repository the program was built from. Programs, games and assets go in `output/<config>` at the top of the repository (`output/Debug`, `output/Release`), and the DLLs (`.so` files on Linux) and Qt's plug-ins in `output/<config>/libraries`, set up in `scripts/cmake/output.cmake` (on Windows the programs find them through a manifest, see [backends](designs/50-backends.md)); the build directory keeps only the compiler's and linker's own files. `output/` is ignored by git. The library knows nothing about the command line, so a different front end only needs its own `main()`.

The library is static by default: each program has the engine's code copied into it, so `xgecli` is one self-contained file. `-DXGE_BUILD_SHARED=ON` builds it as a shared library instead (`libxgelib.so`, or `xgelib.dll` on Windows), which each program loads when it starts; a shared build needs the library file to be found next to the program or on the system's library path. On Windows the DLL exports every class in the headers (`WINDOWS_EXPORT_ALL_SYMBOLS`) rather than each being marked by hand. The third-party libraries are `PUBLIC` dependencies of the library, because the engine's own headers include theirs. See [backends](designs/50-backends.md).

## Source map

| File | Responsibility |
|---|---|
| `xge/cli/source/main.cpp`, `xge/cli/source/cli.cpp` | xgecli, the command line program: read the options, find the game file, build `Game` and `Engine` with the chosen backends. The only code outside the engine library |
| `xge/cli/source/generate.cpp`, `games/generators/windows-cpp/` | `--generate`: load the game as the engine does, run the target's stylesheets on it with libxslt, and copy in the module, engine files and assets they list ([Generating a program](#generating-a-program)) |
| `xge/gui/source/*.cpp` | xgegui, the Qt application ([cli](designs/52-cli.md)): `main_window` (the window, the File and View menus, the question about two windows), `game_session` (a loaded game and its engine, run from a timer; play, pause, step, reset, and changing the libraries), `game_stage` (where the Qt renderer's picture is: the left pane, or a window of its own), `game_view` (the widget a picture is shown in), `key_queue` (the keyboard, read by Qt), `qt_window` (the `Window` that draws with QPainter), `options_dialog` and `session_options` (the video library, XML parser and sound library choice), `app_settings` (`xgegui.ini`, next to the program), `inspector` (the controls and the tree of game data) |
| `game_xml.cpp` | Walk the parsed XML tags into a `GameData` (`game_data.h`: raw window/variable/object/state data, `RawValue`, `RawCommand`, `RawSprite`), `readGame()`; a `<group>` is read here as one raw object per member. `Game(GameData)` works one out, so `game.cpp` reads no XML |
| `game_expr.cpp` | exprtk symbol table and evaluation of raw values (expressions, `<random>`, `<equation>`, `<formula>`) into `Object`s and `State`s |
| `command.cpp` | Turn raw command tags into typed `Command`s; the table of arithmetic operations (`operationShape`) and a value printed as text (`valueText`) |
| `scenes.cpp` | `layOutScenes`, run first by `Game`'s constructor on data read from a file or written by a generated program alike: makes each state's and scene's objects (takes from the kit, `from=`, changes), names them by their path, adds the scenes' variables, and writes every name the game uses out in full, looked for from the inside out |
| `game.cpp` | The interpreter: objects, state stack, current scenes (`enterScene`, `resetScene`, what runs and what is drawn still), conditions, resets, paths (`follow`, `applyPaths`), and the answers to `FrameRules` (what the file says happens at an edge, on a touch, which rules hold), which it carries out through `CommandExecutor`. It does none of the frame's work itself |
| `frame.cpp` | `playOneFrame`: the order every frame runs in (animations, timers, paths, climbing, pull and thrust, the window edges, the move with its sweep, sticks, conditions) and the moving, asking a `FrameRules` (`frame.h`) what the game says at each moment. The engine's `Game` answers from the file; a game written as code could answer in code (tried and set aside, [generator](designs/40-generator.md#prototype-the-game-as-decisions-on-the-engines-modules)) |
| `maze.cpp` | Walking a [map](#maps)'s tiles (`walkMazes`, run by `playOneFrame` after the paths): the lane, the turn in the middle of a tile, the walls, and what `<go>`, `<hunt>` and `<wander>` choose |
| `verbs.cpp` | What a verb does to an `Object` once something has decided it happens (`bounce`, `deflect`, `stick`, `wrap`, `ride`, `die`, `stop`, `restart`, `holdMove`), and making an object (`makeObject`, `startAt`); `CommandExecutor` and generated games both call them |
| `random.cpp` | The one random number source (`seedRandom`, `randomBetween`, `fixRandomSeed`), drawn from by a `<random>` and by generated games alike |
| `geometry.cpp` | The arithmetic of touching and of the verbs, on boxes and velocities with no `Object` in it: box, circle and swept tests, the pixel walk, the sweep of two things and the loop that plays a frame from touch to touch (`sweepThings`, `playFrame`, templates in `geometry.h`), the window's edges, and what bounce, deflect, stick, wrap, land, climb, chase, aim, fire and a path's step do. A generated program compiles it unchanged |
| `collision_detector.cpp` | `geometry.cpp`'s tests for `Object`s: their shapes, and the pixel pass for type `pixel` |
| `builtin_font.cpp` | The 8x8 font stored in the program (`rasterizeText`), which draws text into a bitmap when a backend cannot load its font file |
| `bitmap.cpp` | Draws a sprite's `<line>`s (`rasterizeLines`) or `<bitmap>` rows (`rasterizeRows`) into an RGBA bitmap, the pixels both the window backends and pixel collisions use, and flips (`flipBitmap`) and turns (`turnBitmap`, `rasterizeTurned`) them |
| `svg.cpp` | Draws a part of an SVG file (`rasterizeSvg`) into an RGBA bitmap, with lunasvg; the only file that includes it |
| `command_executor.cpp` | Carries out a typed command: picks the verb and its target, then calls `verbs.cpp` (or `geometry.cpp`) for what it does |
| `sound.cpp` | Pitch names, wave names, and `synthesize()`: a sound's notes made into 16-bit samples, the same for every audio backend |
| `audio.cpp`, `audio_*.cpp` | `AudioFactory` and the sound backends (SFML 3, raylib, SDL2; `NullAudio` is in `audio.h`) |
| `engine.cpp` | Frame loop (`loop()`, or `step()` and `render()` for a front end that owns the event loop), key handling (`HeldKeys`, `keycode.h`), drawing (`drawFrame`, `window.h`), and playing the sounds each frame asks for |
| `engine_backends.cpp` | Engine's constructor that picks the window and sound libraries by name, through the factories; apart so a program built for one library leaves the factories out |
| `object.h`, `states.h`, `color.cpp`, `keycode.cpp` | Data model, named colors, key names |
| `window_*.cpp`, `xml_*.cpp`, `xsd_lite.cpp` | Backends and the weak validator |
| `xge/tests/` | Catch2 tests (opt-in with `BUILD_TESTING`): collision geometry and swept collision, command parsing, conditions, input resolution, `stick()`, collision rules, lockstep bounce, size expressions, engine key handling, object variables, the new verbs (`dec`, `hop`, `wrap`, `ride`, `unless`, `atmost`, colors), the tag format, its rejections and the names commands use (`test_xml_format`), arithmetic as tags (`test_equations`: the three spellings agreeing, each operation in both forms, chains, nesting, step names, every rejection with its message, a divisor of 0 at load and while running, both schema checkers, and Pong's and Breakout's titles), groups (`test_group`: expansion, overrides, names, lockstep, columns and rows with what a row, column or cell changes, gaps, picks, clashes, errors, both schema checkers), lines, pixel collisions, acceleration, thrust and the speed filters (`test_lines_and_pixels`), bitmaps, animations and Space Invaders as written with them, with both schema checkers (`test_bitmap_sprites`; `invaders_fixture.h` keeps a copy of the first, plain Space Invaders for the tests that are about a block of cells and not about that game), the built-in font, the command line and the data folder search, sound (`test_sound`: pitch names, the synthesizer, `<sounds>` and its rejections, Pong asking for its sounds, the eight games that were silent each having theirs and playing each, `Engine` and a recording `Audio`), the one path of drawn pictures and `<flip>` (`test_pictures`), timers, facing, jumps, `<reverse />`, looks, `<reveal>`, conditions that count, key sets, `<chase>` and `<aim>`, `<copy>` and `<keeps>`, a `<text>` of pieces with `<letter>`s, a group's text showing each member's own, and `<rank>` (`test_gameplay_verbs`), `Engine::pump()` and `isWindowOpen()` (`test_engine_input`), the mouse and gamepads (`test_controls`: their buttons as keys, a stick as directions, `<input axis>` and `<track />` on Kaboom's bucket, and the mistakes), Breakout (`test_breakout`: the top row cracking at one hit and going at the next, and whole again after a restart; the sounds its ball asks for off a wall, the paddle and the bricks), and Frogger, Space Race, Kaboom, Freeway, Depth Charge, Astrosmash, Lunar Lander and Asteroids (`test_asteroids`: headings, turning, thrust and drag, the pool of shots, `release`, wrapping, losing ships, winning) played frame by frame , Demon Attack (`test_demonattack`: the three waves, a demon splitting in two, a big one taking two hits, bombs on the cannon, three cannons and game over), Berserk (`test_berserk`: the man's four looks and shots, robots patrolling and firing at the man, the exits, a room put back as it started when entered again, and the room bonus, Otto chasing him, extra men, pausing and game over), paths and Galaxian (`test_galaxian`: steps, home, a start, a stagger, a step's commands, what ends a path, the mistakes, the fleet flying in, a dive that wraps and comes home, a new fleet, the best score kept, initials for the table of the best and the table ranked, what is kept written to a file and read back, two players taking turns with a game each, and the attract mode: the title playing its demo, a key ending it, and a game recorded playing back the same frame for frame), Frostbite (`test_frostbite`: jumps, rows turning blue, the igloo, the cold, drowning, the geese and the fish), Donkey Kong (`test_donkeykong`: standing on girders, walking off an open end, a leap's height and fixed arc, ladders up and down, a barrel's way down to the oil drum, a barrel jumped and one not, saving Pauline), and the five Atari games: Pitfall! (`test_pitfall`: screen to screen and round, a log, the tar pit leapt and fallen into, the crocodiles and their jaws, crossing the pond, the ladder, the treasures, the clock), Missile Command (`test_missilecommand`: missiles chasing cities and turning when one goes, a counter-missile bursting at the sight, a burst taking a missile, a city lost, the waves), Combat (`test_combat`: driving, backing and turning along the heading, a shell from the gun, a hit, a wall), Air-Sea Battle (`test_airseabattle`: the lanes going round, a gun swung and fired at an angle, a plane shot down for the right player and coming back, the clock, the winner's look on the one result screen) Megamania (`test_megamania`: a wave wrapping, bombs, a shot, the energy, the cookies bouncing, the waves won) and Atlantis (`test_atlantis`: the launch timer, a ship dropping a lane, the lowest lane wrapping and raying the city, each post's angle, a hit, the copied and replaced launch and quota, the waves won, the city lost), Pac-Man (`test_pacman`: the map's pieces on their tiles, Pac-Man going, turning at the first gap, stopping at a wall and turning back, the tunnel, Blinky hunting him without stepping on a wall, the ghosts leaving the house, a pellet turning the ghosts blue and its timers, a ghost eaten and its eyes going home, game over and a win, what a `<map>`, a walker and a look's timer turn away, a bitmap's `<color char>`, and both schema checkers), Adventure (`test_adventure`: picking up and carrying, one thing at a time, putting down and walking off, the key opening the gate on its way, the sword and the dragon, the two rooms walked between, the chalice carried home from the field, the pause drawing the room still), Pong (`test_pong`: each state's own objects, the pause and the game over drawn over the game still), and states and scenes (`test_scenes`: a state's own objects, each scene's copies and `<enter>`, names from the inside out, scene variables, scoped class conditions, `<reset scene>`, ways (`<enter way>`, `<reset way>`, and their mistakes), `from=` copying ways, conditions and timers with named ones replaced, counting another scene by path, what is carried going along, `<drawn>`, `<show state>`, and every mistake the loader turns away). Each game's tests also check what its states own and that its pause draws the game still |

## Known limitations

- Object-object collision knows only the four edges of the other object (and, for a pixel hit, the side the motion came in through); there are no verbs beyond the table above (the jumps are a step (`hop`), a straight line (`<jump>`) and a fixed arc with no air control (`<leap>`); no shooting patterns or AI strategies beyond `<chase>` and `<aim>`).
- A maze walker finds its way only as the arcade's ghosts do, by the next tile nearest a target: there is no search of the maze, so a target behind a wall can leave it going round. Pac-Man's ghosts have no scatter mode (their timed trips to the corners), each ghost eaten is worth the same 200 (an `<inc>`'s amount is fixed when the game loads), and the ghosts leave the house on timers, not on how many dots have been eaten. A map's tiles are one character each, and a map cannot be taken from a scene's kit and changed.
- A path is straight steps, not curves: a loop is a few steps round it, and the steps are flown at one speed, with no easing. A path cannot aim (a dive weaves the same way whatever the ship does) and its numbers are fixed when the game loads. Home is the object's place in the game file; a formation that moves (Galaxian's sways from side to side) would need home to move with it. A follower that is stopped or pushed on the way (a `<stick />`) still counts the step as flown.
- Enemies fire on timers from wherever they are: an invader behind others fires through them (in the arcade game only the bottom one of a column fires). `<chase>` goes straight at its target and `<aim>` straight at where it is, through walls and with no lead; a Berserk robot walks its corridor whether or not the man is near. Firing needs a pool of hidden projectiles to take from; with all of them in flight a timer that goes off does nothing.
- A condition's number (`<atleast>`, `<atmost>`, `<above>`, `<below>`, `<equal>`, `<remaining>`), and the amount of an `<inc>` or `<dec>`, are values worked out once when the game loads, so they cannot follow a variable (`<atleast>player.nextlife</atleast>` is a constant, and a condition written to move its own goalposts fires every frame); to follow one, compare with it: `<atleast object="player" variable="nextlife" />`, and to set one variable to another's number as it is now, `<copy>` it. A timer's `<every>` and `<after>`, and the ends of a `<random>` in them, are worked out again each round and can; a count that has to start again is kept in a variable of its own object, which `<reset object>` of that object puts back (Berserk's tally of the robots shot in a room, and its score since the last extra man).
- Timers count frames, like everything else, so a machine that cannot keep up slows the clock with the game. There is no way to show how long a timer has left, other than a timer that counts a variable down.
- A look is a whole picture; there is no way to recolour one. An object with a `<heading>` or an `<animation>` has no looks, and looks do not combine with an animation (an animated object that changes look).
- Players taking turns see only their own game: the other player's score is in their game, put away, so a screen cannot show both scores at once, and the table only hears of a score through the condition of the player whose game is current.
- A demo records the keys, not the mouse, a pad's stick or a paddle, and plays the same only from the start of a game: random numbers drawn when the file loads (a game-level `<random>` variable) are not drawn again from its seed, so a game that uses them at the start can play out differently. A demo recorded with a game that has since changed plays the old keys into the new game.
- The table of the best has no place numbers (1. 2. 3.) unless each member's text is written with its own, and its numbers do not line up in columns in a proportional font. A saved file holds numbers and looks, not where a kept object is.
- `<facing>` is one of four directions; a picture does not turn with it (use `<heading>` for that).
- Sound is short effects and little tunes, not music: one wave at a time per sound (no chords inside one sound; two sounds at once do play together), no looping background tune, no stopping a sound from the game file, no volume envelope beyond the click-free fade (a note does not decay like a plucked string), and no stereo. A sound is made when the window opens, so its notes cannot change with a variable while the game runs. It starts on the frame it is asked for, so it is as late as the sound library's buffer (a few hundredths of a second).
- An object that is not visible is not moved and does not collide, whatever its velocity; that is how a bullet waits, unseen and still, to be fired. One that should run unseen (an exit, a trigger zone) is `<drawn>false</drawn>` instead.
- A state drawn still under another (`<show state>`) is frozen whole: nothing of it animates or counts its timers, so a [demo](#attract-mode) is played as a game of its own, not behind an attract screen. An object shown by `<show object>` in two states runs in both.
- Going to a scene is always a command written where the move happens (an exit's rule, a condition), naming the scene or one of the current scene's ways; nothing says which way the hero came in beyond where the state's own objects are. A value cannot yet be handed from one state to another other than by reading it by path (`playing.score`).
- An object's own `<variable>` named `width` or `height` shadows its measured size (`objectName.width` then reads the variable). A non-colliding spelling is under consideration; see [designs/02](designs/04-values-and-names.md).
- A text or image placed with `objectName.width` / `objectName.height` is re-placed only when the size of an object it names changes, so it is not re-centered after it has moved on its own, and other expressions (velocity, variables) see those sizes as 0 for unmeasured text and images.
- Text is drawn with `assets/tuffy.ttf`, found relative to the directory the program runs from (the games folder, which the programs move into). If that file cannot be found, each backend prints `error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead` once, and draws every text with the font stored in the program: 8 by 8 pixel glyphs for printable ASCII (anything else is a `?`), scaled to about `<size>` / 16 in blocks tall (so a letter is 8 times that tall) and half as wide, so it is chunky and monospaced and its width differs from the real font's ([pictures](designs/30-pictures.md)).
- Movement is in pixels per frame with no time step. Acceleration is constant per object (`<acceleration>`, `<accelerate>`); an object can face a heading and be pushed along it (`<heading>`, `<turn>`, `<thrust>`), and `<drag>` is the only thing that slows an object by itself apart from a rule or an opposing thrust.
- A `pixel` collision is only as exact as half a pixel of motion: a pair is looked at every half pixel along the longer way it moves in a frame, and two one-pixel lines can cross at a slant without sharing a pixel at all, so lines meant to be tested should be at least 2 pixels thick. It knows nothing about which way a surface faces, so there is no bounce off a slope. Text and images cannot be `pixel` yet (their pixels are only known to a window backend), so a fighting game with image sprites will need the backends to hand their pixels back. The bitmap of a sprite of lines is kept for every object that has one, four bytes a pixel, so one big drawing costs what its bounding box does (Lunar Lander's 800 by 182 moon is about 580 KB).
- A sprite of lines is measured from 0, 0 and fixed when the game loads: it cannot be moved or scaled, and it turns only on an object with a `<heading>`, in whole degrees (one picture at a time, always the same square size). Lines are straight, one color each, and have no fill.
- A `<bitmap>` is fixed when the game loads too; its colors are its `<color>`s, one to a character. A turning object keeps the original and the one picture it shows, so the memory it costs is about double, and a turn is done again only when the heading moves by a whole degree. The sprite pictures are separate bitmaps, so a game with many large animated sprites keeps them all in memory: four bytes a pixel for each frame, once per object (the cells of a grid share theirs).
- An `<svg>` is drawn once, when the game loads, at one scale: to show one at two sizes, make two sprites. A pixel collision counts any partly transparent edge pixel as solid. An animated object cannot have looks, so an animated alien cannot change picture on a hit; an explosion would be a pool released where it died. The banking, hit-flash, charged-bolt, explosion and saucer pictures on the Space Invaders 2 sheet are not used yet (see [pictures](designs/30-pictures.md)).
- An animation has one interval for all its frames, and counts frames of the game, not time (see [Animation](#animation)). There is no way yet to show a picture that depends on something other than time (a variable, the direction of travel, a hit), to run an animation once and stop, or to start it from a collision.
- Each cell of a group is its own object with its own count of frames. The cells stay in step because they start together (at load, and again at every reset) and only count while shown.
- A `<random>` is drawn when the game loads and again only when its object comes back into play: a reset (of it, of its [scene](#scenes), or of the game), a `<reveal>`, a `<release>` or a `<fire>` (see [Values](#values)). A `<random>` in a game-level or scene variable is drawn once, and entering a state or a scene draws nothing. There is no command that draws again without putting the object back in play, and nothing in the language creates a new object while the game runs, so things that fall are a fixed set that cycles, or a pool that is fired from, released or revealed. A `<timer>`'s own interval is drawn again every round.
- A hop is a jump, not a slide: the object is simply one step away, with no animation between and nothing touched on the way (a step is meant to be one lane). The arcing jump is `<leap>`.
- `ride()` lends velocity one frame at a time and only while touching. It is not attached: an object riding something that meets a screen edge is handled by that edge's rule like any other (Frogger's frog loses a life), and nothing pushes it into a wall.
- A pair in which nothing moves is not looked at (see the collision rules above), so a rule against something that stands still runs only when the object moves, rides something, hops or lands from a jump.
- `unless` names a class, not a single object.
- `wrap()` assumes what it wraps is spaced with its own size in mind (a lane of things that are `size` wide repeats every window width plus `size`), and the members of a lane (a `<group>`) are placed by hand, one `<x>` each, or evenly spaced as one row of `<columns>` with a `<padding>`: a group's cells are spaced by sizes and gaps, not spread across a width.
- Only the object-against-object test is swept. A screen edge is still checked by position before the move, so an object that moves more than a whole window's width in one frame is not caught by it; rotation is not modeled. A shape that is not a rectangle or circle (text, image) is treated as its bounding box, unless it is a sprite of lines tested by `pixel`.
- Expressions inside a value are not checked by the schema (they are text); a typo in one is found when the game loads (the load stops with a message naming where), not by validation. Structure, tag names, attributes and command verbs are checked by the schema; the names a command uses are checked by the engine when it loads.
- The weak validator (`xsd_lite`) covers only the XSD subset this project uses; Xerces is the full check. It follows a named type that contains itself (a `<formula>`'s operands), but not a group that does.
- `<equation>` and `<formula>` have four operations and no more: no `min`, `max`, `negate`, `abs`, `clamp`, `sign` or `pick` (the exprtk text has them). An operand is a name or a number, so a sum that is needed in two places is a named `<equation>` step; an `<equation>` has no `<random>` step (a `<formula>` operand can be one). Only Pong and Breakout use the tags; every other game still writes its arithmetic as text.
- A `<divide>` by something not known while the game loads (another object's variable, an unmeasured text's size) works out to 0 at load, and only a position or a timer's interval is worked out again afterwards, so an object `<variable>` made from such a division keeps the 0.
- The window's `width`, `height` and `framerate` are plain numbers, not values, and are read once, before the window opens. The size cannot be changed while a game runs, in `xgegui` either.
- `--generate windows-cpp` covers every game; a generated program is the engine with the game built in, so it shares the engine's limitations and has none of its own. It always carries exprtk and lunasvg, as the engine does, even for a game with no `<svg>`.
