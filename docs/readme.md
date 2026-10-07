# XMLGameEngine: current design

This document describes what the engine does **today**, as read from the source on 2026-10-04 (before that, games were written as function-call attribute strings; see [designs/01](designs/01-vision-and-format.md)). For the reasoning behind these choices, the alternatives that were considered, and ideas that are not built yet, see [designs/00-designs.md](designs/00-designs.md).

XMLGameEngine is a video game description language (VGDL) written in XML, plus a C++ engine that loads a game description and runs it. A game is one `.xml` file. The description is declarative: there are no loops and no `if` statements in it, and no function calls either. Behavior comes from a fixed vocabulary of verbs (the tags `<bounce />`, `<stick />`, `<die />`, ...) that the engine knows how to carry out. In this document, and in the design notes, a verb is sometimes written as `bounce()` for short; in a game file it is always the tag.

## Running a game

```
xgecli                       # loads "pong"
xgecli breakout              # a bare name gets ".xml" appended
xgecli pong.xml              # a name with an extension is used as given
xgecli -g pong -w sdl2 -x tinyxml2
xgecli --game pong --window raylib --xml pugixml
xgecli pong -a sdl2          # sounds played by SDL2 (or -a none for silence)
```

The game is a bare argument or `-g` / `--game`; both are looked for the same way: as given (a path, or a name in the current directory), then its file name in the working directory, then its file name in `games/` of the data folder (the first of the working directory, the program's folder and the folder above it that has both `games/` and `assets/`; the program then runs from there, so it can be started from anywhere). If it is not found the program prints an error and exits. `-w` / `--window` picks the window library (`sfml3`, `raylib`, `sdl2`, `opengl`; default `sfml3`) `-x` / `--xml` the XML library (`xerces`, `tinyxml2`, `pugixml`, `rapidxml`; default `xerces`) and `-a` / `--audio` the sound library (`sfml3`, `raylib`, `sdl2`, `none`; default `sfml3`), not case sensitive. Only the libraries the program was built with can be named ([Backends](#backends)): a default build has `sfml3` and `xerces` (and `none` for sound), `--help` lists what there is, and another is an error that says which there are. Any sound library goes with any window library. A short option takes its value attached or after a space (`-gpong`, `-g pong`); a long option needs the space (`--game pong`, not `--game=pong`). Each option can be given once, the game only once (bare or with `-g`), and `-h` / `--help` prints the usage. The program starts by printing the file, window library, XML library and sound library it chose, one to a line. See [design 12](designs/12-front-ends.md). Shipped games: `games/pong.xml`, `games/breakout.xml`, `games/spaceinvaders.xml`, `games/spaceinvaders2.xml` (the same game drawn from an SVG sprite sheet), `games/frogger.xml`, `games/spacerace.xml` (two players, W/S and Up/Down, first to two points), `games/kaboom.xml` (A/D or Left/Right; the Mad Bomber paces the rooftop and drops bombs as he goes, catch them through three waves, three missed bombs end the game, 90 points win), `games/freeway.xml` (two players, W/S and Up/Down, first to five crossings), `games/depthcharge.xml` (A/D or Left/Right to move, Space to drop, Space to start; sink all nine submarines before eight charges are wasted) `games/astrosmash.xml` (A/D or Left/Right to move, Space to fire, Space to start; shoot 20 rocks before five land) and `games/lunarlander.xml` (Up or W for the main thruster, Left/Right for the side ones, Space to start; set the lander down on the green pad slower than the safe speed, with fuel to spare, and do not touch anything else), `games/asteroids.xml` (Left/Right or A/D turn, Up or W thrusts, Space fires; break every rock before three ships are lost), `games/berserk.xml` (W A S D or the arrows walk the man and set the way he faces, Space fires that way, one shot at a time, P or Escape pauses, Space starts; four mazes of electrified walls with robots that patrol and fire at the man, a bonus for clearing a room, Evil Otto coming for him if he lingers, and an exit in each outer wall for the next room; after the arcade game Berzerk, see [design 10](designs/10-games-as-tests.md)), `games/demonattack.xml` (A/D or Left/Right, Space to start and to fire; a small game first written by another AI from the schema alone, see [design 10](designs/10-games-as-tests.md); the demons fire back), `games/pong_min.xml` (Pong with nothing but two paddles and a ball, W/S and Up/Down, a point puts the ball back in the middle; the game [`--generate windows-cpp`](#generating-a-program) was grown on), `games/frostbite.xml` (W/S or Up/Down jump between the shore and four rows of drifting ice, A/D or Left/Right walk; land on white ice to build the igloo before the cold gets you, then walk in; see [design 08](designs/08-timers-and-enemy-behavior.md)), `games/galaxian.xml` (A/D or Left/Right, Space to start and to fire, P or Escape pauses; the fleet flies in on [paths](#paths) and dives at you, three ships; a new fleet when one is shot down), `games/donkeykong.xml` (A/D or Left/Right walk, W/S or Up/Down climb the ladders, Space starts and jumps, P or Escape pauses; climb past Kong's barrels to Pauline, three times, with three lives), and five after Atari 2600 games ([design 10](designs/10-games-as-tests.md#five-atari-2600-games)): `games/pitfall.xml` (A/D or Left/Right run, Space starts and jumps, W/S climb, P or Escape pauses; four jungle screens, each a state, with logs, a tar pit, a crocodile pond, a fire, scorpions in the tunnel and a treasure on each, three minutes and three lives), `games/missilecommand.xml` (W A S D or the arrows move the sight, Space launches, P or Escape pauses; the base `<aim>`s at the sight, a counter-missile bursts there, missiles `<chase>` the nearest city, three waves), `games/combat.xml` (two players: W/S drive, A/D turn, F or Left Shift fires; Up/Down, Left/Right, Right Shift or Right Control; Space pauses; tanks with headings among walls, first to five hits), `games/airseabattle.xml` (two players: A/D swing the gun and W fires; Left/Right and Up; Space pauses; four lanes of planes, two minutes, most hits wins) and `games/megamania.xml` (A/D or Left/Right slide, Space fires, P or Escape pauses; three waves that wrap round the sides and drop bombs while the energy runs down, three ships).

The XML, window and sound libraries are chosen in C++ with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`; `xgecli` passes what `-x`, `-w` and `-a` named, Xerces, SFML3 and SFML3 when they are not given. `xgegui` (the Qt application) takes the game the same way, `xgegui pong`, or opens a file dialog in `games/` when none is named; its Options dialog picks the video library, the XML parser and the sound library. See [Backends](#backends).

A game file that is wrong (it does not match the schema, an expression will not evaluate, a command names a state or object the game does not have) stops the load with a message saying where; `xgecli` prints it and exits, `xgegui` shows it and carries on.

## Generating a program

```
xgecli pong --generate windows-cpp                     # writes pong-windows-cpp/
xgecli pong --generate windows-cpp -o out/pong         # or into a folder named
```

`--generate <target>` does not play the game: it turns the game file into a program of its own that plays it, with nothing of the engine in it, and exits. The target, `windows-cpp`, writes a C++ program on SFML 3: `main.cpp` (the game), `CMakeLists.txt` (its build, which uses an installed SFML 3 or downloads and builds one, and makes the game Visual Studio's startup project) and `README.md`, with the modules the game uses copied beside them (`physics.h`; `sound.h` and `sound.cpp`) and any font and pictures it uses copied into `assets/`. `-o` / `--output` names the folder (default `<game>-<target>` in the directory `xgecli` was started from) and is an error without `--generate`. `-g` stays the game.

It writes the game as a person would write it by hand: the header block, the includes (local, then SFML, then the standard library), the window's constants and the game's `<variables>` as tunables (`const float ballRadius = 10.0f;`, every name in camel case), the screens (`enum class Screen` and a stack of them, when there is more than one `<state>`), the objects as SFML shapes, texts and sprites with a velocity for those that move (`sf::CircleShape ball;`, `sf::Vector2f ballVelocity;`) and each `<group>` as a `std::vector` of them (`std::vector<sf::RectangleShape> debris1(3);`, with one velocity they share or a `std::vector` of one each), the sounds, the functions declared, `main` (the window, then the game loop: events, update, render), and the functions defined below it: `setup()` (the font, pictures, looks and sounds), `start()` (the game from the start), `pressed()` for keys pressed, one `update` function per screen (keys held, its objects, its conditions) and per object or group (its move and each of its rules as an `if` statement; a group's in a loop over its members), and only the small helpers the game uses. A game variable nothing uses is left out. The physics (`physics::past`, `physics::bounce`, `physics::wrap`, `physics::hop`, `physics::deflect`, `physics::width`, ...) and the sound (`sound::Sound`, made from its notes) are two small modules of their own beside `main.cpp`, nothing but SFML, copied only when the game uses them. [`games/pong.xml`](../games/pong.xml), menu, pause, score, pictures and sounds, comes out at about 480 lines; [`games/pong_min.xml`](../games/pong_min.xml) at about 230. It covers Pong, Space Race, Freeway, Breakout and Depth Charge so far (see [Known limitations](#known-limitations)).

The work is done by XSLT stylesheets in `generators/<target>/`, run by libxslt (with EXSLT's `exsl:document` writing the files); `xgecli` only runs them and copies the modules and assets they list. For `windows-cpp`, `generate.xsl` is the entry, `check.xsl` says what can be generated, `main.xsl` lays out `main.cpp` with one template per part (`generate-main` calls `generate-window` and `generate-game-loop`, which calls `generate-events`, `generate-update` and `generate-render`), `groups.xsl` works out each group's members or cells as the engine makes them (look, velocity, place) and writes their looks and layout, `values.xsl` turns a value into a C++ expression, `functions.xml` holds the helpers and `modules/` the physics and sound modules. `tables.xml` maps key and color names to SFML's. A tag the target cannot generate yet stops it with a message naming the tag and where it is (`windows-cpp cannot generate <lockstep> yet (in game > objects > group bricks > collisions > lockstep)`), and a folder made for it is removed again. `xgecli` built without libxslt says so when `--generate` is given. See [design 01](designs/01-vision-and-format.md).

## Game file layout

A game file has one `<game>` root with four children, and two optional ones (`<sounds>` and `<paths>`), in this order, as enforced by [xgedef.xsd](../xgedef.xsd):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="..."> ... </window>
  <variables> <variable name="...">...</variable> ... </variables>
  <sounds>    <sound name="..."> ... </sound> ... </sounds>   (optional)
  <paths>     <path name="..."> ... </path> ... </paths>      (optional)
  <objects>   <object name="..."> ... </object> <group name="..."> ... </group> ... </objects>
  <states>    <state name="..."> ... </state> ... </states>
</game>
```

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

A value holds text or a tag, not both, and not two tags (`<x>5 <random .../></x>` is an error naming the element). A `<random>` is drawn at load and drawn again whenever its object is reset (a `<reset />` in a collision, `<reset object>`, or a reset of the whole game), so each Pong serve is new; a `<random>` in a game-level variable is drawn once.

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

Global named numbers. Each `<variable name="margin">30</variable>` becomes a constant that any expression in the file can use (for example `margin`, `ball.radius`). Names may contain dots. A name exprtk already has for a function (`floor`, `min`, `max`, `abs`, `sqrt`, ...) cannot be used: an expression reads it as the function and the load stops (`Expected a '(' at start of function call to 'floor'`), which is why Pitfall's tunnel floor is the variable `tunnel`. The content is a value, so `<variable name="b">a * 2</variable>` and `<variable name="start"><random min="1" max="3" /></variable>` both work. Declaring a name twice keeps the later value.

### `<sounds>`

Optional. The bleeps, bloops and little tunes of an old 8-bit machine, written as notes: nothing is loaded from a sound file. Each `<sound name="...">` is something a `<play sound="..." />` command (see [Commands](#commands)) can start. Its children, in this order: an optional `<volume>` (a value, 0 for silent to 1 for the loudest; 0.3 if left out), then any mix of `<note>`s and `<rest>`s, played one after the other.

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
- **`wave`** on a `<sound>` is the wave of every note that does not give its own: `square` (the default; the classic beep), `triangle` (softer, the bass of an old console), `sawtooth` (buzzy), `sine` (pure and soft, which no 8-bit chip had) or `noise` (hiss and crashes: a new random level every half cycle of the pitch, so a high pitch is a hiss and a low one a rumble).

Every sound is made into samples (16 bit, one channel, 44100 a second) by the engine when the window opens, the same for every sound library ([design 09](designs/09-sound.md)). Each note fades in and out over a couple of milliseconds so it does not click. Each sound has one voice, like a channel of a sound chip: playing a sound that is still playing starts it again from the beginning, and different sounds play over each other. A sound asked for more than once in one frame is played once.

A wave, pitch or length the engine cannot use, a `<volume>` outside 0 to 1 or after a note, a sound with no notes, two sounds of one name, and a `<play>` naming no sound all stop the game loading with a message saying where (`sound 'wall' > <note> 1: pitch="H2" is not a pitch; ...`).

### `<object>`

An object is anything that can be drawn, moved, or collided with: a ball, a paddle, a score readout, a title. Attributes: `name` (required, how other elements refer to it) and `class` (optional, a group label that collision rules and conditions can match, like a CSS class).

Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<sprite>` | yes | What the object looks like: one shape (see [Sprites](#sprites)). An object with an `<animation>` has several, each with a `name`; so does one with several [looks](#looks) |
| `<animation>` | no | Which of the object's sprites are shown, in what order, and for how many seconds each: see [Animation](#animation). Objects, and groups and their members |
| `<position>` | yes | Starting position, in pixels from the top-left: `<x>` and `<y>`, each a value. May use an object's own size by its name, `title.width` and `title.height`, to place it by its size, for example a text called `title` centered: `<x>window.width.center - title.width / 2</x>` |
| `<velocity>` | yes | Starting velocity, in pixels per frame: `<x>` and `<y>`, each a value |
| `<acceleration>` | no | A constant pull: `<x>` and `<y>`, each a value, added to the velocity once every frame before anything moves, for as long as the object is shown. Gravity is an acceleration with only a `<y>`. `<stop />` takes it away and a reset gives it back. Objects and groups (Donkey Kong's barrels fall); `<land />` is how a falling object stands on something |
| `<heading>` | no | The way the object faces, in degrees clockwise from straight up (0 is up, 90 is right). It lets `<turn>` and `<thrust>` work, and the sprite (of lines, a `<bitmap>` or an `<svg>`) is then drawn turned to the heading, to the nearest whole degree, so a `pixel` collision follows it. A collision `<reset />` puts it back. Objects only |
| `<drag>` | no | A value from 0 up to (not including) 1: the fraction of its velocity the object loses every frame, after its thrust is added. Objects only |
| `<facing>` | no | `up`, `down`, `left` or `right`: the way the object faces, without turning its picture. It follows the last `<move>`, `<hop>` or `<jump>` it makes, and a `<fire>` leaves the middle of that side moving that way, at the projectile's own speed: a man in a maze shoots the way he last walked, and an alien with `<facing>down</facing>` drops its bombs below it. A reset faces it the way it started. Objects and groups |
| `<hidden>` | no | `true`: the object starts out of play (not drawn, no collisions, not counted by a condition's `remaining`) until a `<release>`, a `<reveal>` or a `<fire>` brings it in. Objects, and groups (after `<velocity>`) |
| `<collisions>` | yes | `<enabled>` (`true`/`false`), an optional `<lockstep>` (`true`), an optional `<type>` (`box`, the default, or `pixel`), then zero or more `<collision>` rules. See [Collisions](#collisions) |
| `<actions>` | no | Named actions the object can perform, each `<action name="up"><move direction="up">step</move></action>`: the commands are what it does. States bind keys to these names |
| `<variables>` | no | Variables owned by this object, each `<variable name="score">0</variable>`. Other expressions refer to them as `objectName.variableName`, e.g. `paddle1.score` |
| `<timers>` | no | Things the object does on its own, every so many seconds or once after a while: see [Timers](#timers). Objects and groups |

An object with `class="projectile"` starts invisible (used for bullets).

### `<group>`

A `<group>` is several objects that share a description, written once: a lane of logs, a row of debris, the pads along the top of Frogger, a wall of bricks. It sits beside `<object>` under `<objects>`. It holds the parts its members have in common, in the same order as an object (`<sprite>`s, an `<animation>`, `<position>`, `<velocity>`, `<acceleration>`, `<facing>`, `<hidden>`, `<collisions>`, `<actions>`, `<variables>`, `<timers>`, each optional except `<collisions>`), then one or more `<member>`s, or, laid out in [columns and rows](#columns-and-rows), the `<row>`s, `<column>`s and `<cell>`s that change them. `<heading>` and `<drag>` are for objects only. Attributes: `name` (required) and `class` (optional, and every member's class).

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
    <y>0</y>
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

### Sprites

A `<sprite>` holds one shape. Colors are named (`color.red`, below); a `<color>` left out is `color.white`. A sprite may be given a `name`, which an object needs only when it has several sprites: for an [`<animation>`](#animation) to choose between, or as [looks](#looks) for `<become>`.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | A filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | A filled rectangle |
| `<text>` | `<content>` (a fixed label) **or** `<number>` (a value), then `<size>`, `<color>` | Text. `<content>PONG</content>` is written as is. `<number>paddle1.score</number>` shows a number: when the value is exactly one `owner.variable` it is live and redraws when that variable changes; any other value (`paddle1.score + 1`, a `<random>`) is worked out once |
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

That picture is 45 pixels wide and 20 tall. The rows are measured from the top left, must all be the same length and may hold nothing but `.` and `*`: a space or any other character, an empty row, or rows of different lengths stop the load with a message that names the row and the character (`row 3 of a bitmap has 'o' as character 2`). A bitmap is drawn once, when the game loads, into the same kind of bitmap a sprite of [lines](#lines) is, so everything said there holds for it: its size is the object's size (`name.width` and `name.height` work with no window), every window backend shows it as a picture, and a collision of [type pixel](#collisions) tests exactly the solid pixels. The cells of a group that draw the same bitmap share the one picture. It has one color. On an object with a `<heading>` the picture is drawn once like this and then that finished picture is turned to the heading the object faces, to the nearest whole degree (360 headings, 0 and 360 being the same): each pixel of the turned picture takes the one pixel of the original that lies under it, so chunky pixels stay chunky and nothing is blended. The object keeps the original and the one picture it shows, and draws a new one only when its heading moves to another whole degree. Every heading comes out as the same square, big enough for the picture at any angle (for a bitmap 11 by 8 characters at `<scale>` 5 that is a square of 68), and that square is the object's size, as with [lines](#lines).

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

An object with several named sprites and no `<animation>` has **looks**: it starts as the first, and `<become sprite="name" />` shows another from then on (a reset brings back the first). `<become sprite="blue" object="row1" />` changes every object of that name or group at once. A collision rule about another object can carry `sprite="name"`: it runs only while its object shows that look, which makes a look a simple state. Frostbite's rows of ice are groups that start white; a floe's rule

```xml
<collision object="bailey" sprite="white">
  <become object="row1" sprite="blue" />
  <inc variable="bailey.score">10</inc>
  <reveal object="igloo" />
</collision>
```

runs once, when Bailey lands on a white row, and turns the whole row blue. Looks are pictures of any shape kind (a rectangle in another colour, another bitmap); an object with a `<heading>` cannot have them, and a look cannot be a text showing a `<number>`.

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

A `<timer>` goes off after a number of seconds, `<every>` time (again and again) or once `<after>` that long, and runs the commands after it. An object's timers (in its `<timers>`, after `<variables>`; a group's are every member's) count while it is shown and in play; a state's (after its `<conditions>`) count while it is the current state, so a pause stops them and they carry on afterwards. A reset starts them over. The interval is worked out again every time the timer starts over, after its commands have run, so `<random>` waits a different time each round (Kaboom's bomber changes direction whenever he likes) and an expression can follow a variable the commands change. Seconds are turned into frames of the game with the window's `<framerate>`; a timer never waits less than one frame.

On an object, a timer can `<fire>` (from that object: enemies firing back), `<reverse />`, `<move>`, `<die />`, `<stop />`, `<release>` and do a bare `<reset />` (the object back where it started); anywhere, it can `<push>`, `<pop />`, `<reset object>`, `<trigger>`, `<inc>`, `<dec>`, `<play>`, `<become>` and `<reveal>`, and in a state a bare `<reset />` resets the whole game. A clock the player can see is a timer that `<inc>`s a variable a text shows; Frostbite's cold is a state timer taking a degree a second off the thermometer.

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
| `<deflect>angle</deflect>` | collision (another object) | A bounce off the other object at an angle set by where it hit: the object's middle level with the middle of the touched side leaves straight out, level with either end of that side (or past it) leaves at `angle` degrees towards that end, and in between in proportion. The speed is kept. The content is a value from 0 up to, not including, 90. Pong's ball has `<deflect>45</deflect>`, so a hit on the top of a paddle goes back up at up to 45 degrees ([design 05](designs/05-collisions.md)) |
| `<stick />` | collision | Clamps the object inside the screen edge it touched and stops only the velocity heading into that edge; the other axis keeps going, so an object pressed against the bottom wall still slides left or right. Re-applied after the frame's move, so a stuck object never ends a frame outside the screen |
| `<wrap />` | collision (screen edge) | Once the object has gone completely off the screen through that edge and is still heading that way, puts it back in from the opposite edge, one window width (or height) plus its own size along, so anything spaced along a lane keeps its spacing. While any of it is still in view nothing happens |
| `<carry />` | collision (another object) | Lends the object the touched object's velocity for that frame, on top of its own: a frog on a log rides along with it. Worked out again every frame from whatever is still being touched, so an object that steps off is at rest |
| `<die />` | collision | Disables the object's collisions and hides it; it stops moving and being drawn until something brings it back (`<fire>` re-launching a bullet, `<reset />`) |
| `<reset />` | collision (screen edge or another object) | Puts the object back as it started: its starting position and velocity (the velocity is left alone while a held key is moving it), with any `<random>` in its start, or in its `<variables>`, drawn again. Pong's ball serves anew after every point |
| `<reset />` | state input or condition | Full game reset: every object's position, velocity, variables, visibility and collisions go back to how they started (a bullet in flight is put away, a dead alien is back), and the state stack collapses to the first state |
| `<reset object="name" />` | anywhere: collision, state input, condition, timer | Resets that one object (or every member or cell of a group, for its name) the same way. From a collision it acts on another object: Kaboom's bomb hitting the ground resets the wave's whole pool of bombs, the explosion |
| `<inc variable="owner.variable" />` or `<inc variable="owner.variable">amount</inc>` | anywhere: collision, condition, state input, timer | Adds 1, or the amount (a value, so an expression works), to that variable and refreshes any text bound to it |
| `<dec variable="owner.variable" />` or `<dec variable="owner.variable">amount</dec>` | anywhere: collision, condition, state input, timer | Takes 1, or the amount, off that variable and refreshes any text bound to it. The variable may go below zero; a condition with `<atmost>` is what notices it has run out |
| `<move direction="up">step</move>` (also `down`, `left`, `right`) | collision, or an object `<action>` | In a collision: shifts the object (or everything in lockstep with it) once. In an object action: sets a held-key velocity (see [Input](#input)). The content is a value |
| `<hop direction="up">distance</hop>` (also `down`, `left`, `right`) | object `<action>` | A one-shot jump of `distance` pixels for each press of the key (see [Input](#input)). The content is a value |
| `<jump direction="down"><distance>90</distance><seconds>0.35</seconds></jump>` (also `up`, `left`, `right`) | object `<action>` | A jump that takes time: once for each press, the object travels the `<distance>` that way over `<seconds>` (0.3 if left out), both values, in a straight line. While it is in the air it touches no other object (it cannot be hit, ride or drown until it lands, and it passes over what is in between), and a second jump waits for the landing; the frame it lands it meets what it landed on. A jump that would land off the screen is not made. Frostbite's jump between rows of ice |
| `<land />` | collision (another object) | Coming down onto the other object's top (touching its top edge, not going up), the object stands on it: it is put on the top, its fall stops, and it is on the ground until the next frame's landing is worked out, which a `<leap>` needs. A leap ends, and the keys held then decide the way across. Touching any other side, or going up, does nothing, so a platform can be walked past at its ends and jumped up through from below. With a falling `<acceleration>` an object stands on whatever it has this rule for, walks off an open end and falls to the next. Donkey Kong's girders, for Jumpman and the barrels |
| `<leap>height</leap>` | object `<action>` | A jump up under the object's own pull, for each press: it leaves the ground at the speed that rises about `height` pixels (a value) against its `<acceleration>`, and falls back until a `<land />`. Only from the ground and not on a ladder; a press in the air does nothing. The way across is the one it had at the take-off: a key pressed or let go in the air does not steer it until it lands. The object needs an `<acceleration>` with a `<y>` above 0 and a `<collision>` with `<land />`, or the game does not load. Donkey Kong's jump |
| `<climb direction="up" class="ladder">step</climb>` (or `down`) | object `<action>` | Held like a `<move>`: while the object stands at an object of that class (its middle over it, its feet between that object's top and bottom) and is on the ground, it gets on, lined up with its middle, and goes `step` pixels a frame (a value) that way. On it, nothing pulls it, it does not `<land />`, a key left or right does not take it off, and with no key held it stays put. Reaching the top or the bottom it gets off, standing there. A ladder runs from the top of one platform to the top of the next. The class must be one some object has |
| `<reverse />` | anywhere an object is acting: collision, timer | The object's velocity turns round, both ways at once. Kaboom's bomber changing direction on a timer |
| `<become sprite="name" />` or `<become sprite="name" object="other" />` | anywhere (in a state's input, condition or timer it needs `object=`) | The object, or every object of that name or group, shows its [look](#looks) of that name |
| `<reveal object="name">count</reveal>` | anywhere | Brings back the first `count` (default 1) out-of-play objects of that name or group (hidden at the start, or taken out by `<die />`), each where it started and at its own starting velocity. Where `<release>` puts them in the middle of the object running it, `<reveal>` puts them where they belong: Frostbite's igloo built a block at a time, a door that appears, a fish that swims back |
| `<accelerate direction="up" burn="fuel">amount</accelerate>` (also `down`, `left`, `right`) | object `<action>` | A thruster: while the key is held, the object's velocity changes by `amount` (a value) every frame in that direction, where `<move>` would set the velocity itself. The optional `burn` names one of the object's own `<variable>`s; 1 is taken off it every frame the thrust is on (and a text bound to it follows), and while it is 0 or below the thrust does nothing. See [Input](#input) |
| `<stop />` | collision (screen edge or another object) | The object comes to rest where it is and stays there: its velocity goes to 0, and it is no longer pulled by its `<acceleration>` or pushed by a held `<accelerate>` or `<move>`, until a reset gives the acceleration back. A landing |
| `<turn direction="left">degrees</turn>` (or `right`) | object `<action>` | While the key is held, the object's heading changes by that many degrees every frame (left is counterclockwise). Needs a `<heading>` on the object |
| `<thrust burn="fuel">amount</thrust>` | object `<action>` | Like `<accelerate>`, but along the way the object faces instead of along an axis; `burn` works the same. Needs a `<heading>` on the object |
| `<release object="name">count</release>` | collision (screen edge or another object) | Puts the first `count` (default 1) out-of-play objects of that name or group back in play, centered on the object running the rule, at their own starting velocity. Fewer left in the pool gives what is there. This is how a rock breaks into smaller ones |
| `<push state="name" />` / `<pop />` | state input or condition | Push a state / pop back. The state named must be one of the game's. `<pop />` with only the first state left does nothing |
| `<pop state="name" />` | state input or condition | This state goes and that one takes its place, so moving on (Kaboom's waves, Berserk's rooms) does not grow the stack |
| `<trigger object="name" action="up" />` | state input | Runs one of that object's named `<action>`s. The object and the action must exist |
| `<play sound="name" />` | anywhere: collision (screen edge or another object), object `<action>`, state input or condition | Plays one of the game's [sounds](#sounds). In an action or an input it plays on the press, not on the release. The game only asks for the sound; the engine plays everything asked for once the frame's keys, collisions and conditions have run |
| `<follow path="name" />` or `<follow path="name" object="name"><stagger>seconds</stagger></follow>` | without `object=`: an object's collision rule, action or timer; with it, anywhere | The object, or every object of that name or group in play (each `stagger` seconds after the one before), flies that [path](#paths) |
| `<chase object="name"><speed>1.5</speed><near>100</near></chase>` | an object's timer, or a collision with another object | The object heads straight for the middle of the nearest one in play of that name or group, at `<speed>` pixels a frame (a value), facing that way; within `<near>` pixels of it (left out: 0) it stops instead. It acts once: a timer that repeats it keeps it on the trail, and how often is how quickly it turns. With none in play it carries on as it was. Berserk's Evil Otto |
| `<aim object="name" />` | an object's timer, or a collision with another object | The object turns to the nearest one in play of that name or group: its next `<fire>`s go straight at where that one's middle was, at any angle, from the shooter's middle, at the projectile's own speed, and its `<facing>` turns to the nearest of the four ways (a `<heading>` turns exactly). Kept until it aims again, is reset, or a key moves, hops or jumps it. Berserk's robots aim, then fire |
| `<fire object="projectile" />` | object action, or an object's timer | Launches the named projectile object; from a shooter that has aimed, along the aim (see `<aim>`); from a shooter with a `<facing>`, from the middle of the side it faces, moving that way at the projectile's own speed; otherwise from the shooter's top-center (the middle of the projectile over the middle of the shooter's top edge), moving with the projectile's own `<velocity>`. A projectile is not drawn, moved or collided with until it is fired, and is put away again by `<die />` (hitting a target, or `edge="all"`). The name may be a `<group>`: the first member that is out of play is the one launched, so a group of four is four shots in flight. A shooter with a `<heading>` fires from its nose, along the heading, at the speed of the projectile's `<velocity>`; with none, one projectile name can only be in flight once |

A command the engine does not know, or one missing an attribute it needs, stops the game loading with a message that says where (`object 'ball' > <collisions> > <collision>: unknown command <explode>`). So does a command that names something the game does not have: a state (`<push state="pasued" />`), an object or one of its actions (`<trigger>`), a projectile (`<fire>`), an object to reset (`<reset object="...">`), a sound (`<play>`), an object to `<reveal>`, or a look to `<become>` that the object has not got. These are checked once every object and state has been built, so a name used before the thing it names appears in the file is fine. When the name is a likely typo of one the game has, the message says which (`(did you mean 'paused'?)`).

The schema accepts some mistakes the engine could only ignore, so the loader refuses them too, each with a message saying what to write instead:
- a key the engine does not have (`<input button="spcae">`: "there is no key named 'spcae' (did you mean 'space'?). The keys are: ...");
- a color that is not one of the `color.` names, in a sprite, a `<line>` or the window's `<background>`;
- a command in a place where it does nothing (a `<move>` in an `<input>`, which needs a `<trigger>` of an `<action>`; a `<push>` in a `<collision>`; a `<stick />` against another object), listing what can go there;
- an `<inc>` or `<dec>` of a variable no object has (`<inc variable="score">` with `score` among the game's fixed `<variables>`, or `o.lifes` where `o` has `lives`), listing the object's variables.

**Colors:** `color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`, and the muted `color.grey`, `color.darkgrey`, `color.lightgrey`, `color.brown`, `color.orange`, `color.purple`, `color.darkblue`, `color.darkgreen`, `color.forestgreen`. Any other name is fully transparent.

### `<state>`

A state is one screen: a menu, the playfield, a pause screen, a game-over screen. Attribute: `name`. Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<shows>` | yes | A list of `<show object="name" />`. Only these objects are drawn and updated while the state is current |
| `<inputs>` | yes | A list of `<input button="key">`, each holding the commands that key runs, e.g. `<push state="playing" />` or `<trigger object="paddle1" action="up" />`. `keys="name"` takes a shared [key set](#input) first; the state's own `<input>`s go over it |
| `<conditions>` | no | A list of `<condition>`, checked every frame. See [Conditions](#conditions) |
| `<timers>` | no | Timers that count while this is the current state. See [Timers](#timers) |

The first state in the file is the starting state. States form a **stack**: `<push state="name" />` pushes a state, and `<pop />` pops back to the previous one. The starting state is never popped: a `<pop />` with only it left does nothing. `<pop state="name" />` puts that state in place of the current one (the starting state too), for a game that moves from one screen to the next.

## Input

An `<input button="w"><trigger object="paddle1" action="up" /></input>` names a key and the commands it runs. Objects never mention keys, and states never mention what an action does, so remapping a key means editing one attribute in one state. `button` can name several keys separated by spaces, `<input button="a left">`, for the same commands.

**Key sets.** Bindings several states share are written once, as a named `<keys>` set at the top of `<states>` (before the first `<state>`), holding `<input>`s like a state's. A state takes one or more with `<inputs keys="bucket">` (several names separated by spaces, each over the one before), and its own `<input>`s go over the set's, key by key, so a state can use the set and still give a key a job of its own:

```xml
<states>
  <keys name="bucket">
    <input button="a left"><trigger object="bucket" action="left" /></input>
    <input button="d right"><trigger object="bucket" action="right" /></input>
    <input button="space p"><push state="paused" /></input>
  </keys>
  <state name="wave1"> ... <inputs keys="bucket" /> ... </state>
  <state name="wave2"> ... <inputs keys="bucket" /> ... </state>
```

The shipped games share one convention: Space starts, pauses and unpauses, and plays again after a game over (in games where Space fires, such as Space Invaders, Astrosmash and Depth Charge, pausing is P or Escape, and Space still unpauses); player one plays on W, A, S and D (the arrow keys are a second way in the one-player games, and player two's keys in the two-player ones). The games written after Pong and Breakout also pause on P, and most of them on Escape; in Pong and Breakout Escape leaves the settings screen (S on the main menu).

Key names are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more listed in `lib/source/keycode.cpp`.

While a key bound to a `<move>` in an object action is held, that direction's step is recorded. On every key change the velocity along that key's axis is recomputed from the two directions of the axis, so holding Down and tapping Up cancels out and releasing Up resumes Down. Left/right and up/down are independent axes, so two keys can make a diagonal, and a key on one axis leaves the other alone: walking does not stop a fall. During a `<leap>` the way across is left as it was until the landing.

The current state decides what a held key means. When the state changes, keys that are still down stop driving whatever the old state bound them to, and start driving what the new state binds them to. In Breakout, holding Left in `playing` and pressing Space stops the paddle, because `paused` binds only Space; unpausing while Left is still down moves it again with no re-press, and a Left first pressed during the pause starts moving it on unpause. Letting go of a key always stops what it was driving. Only continuous bindings (an action's `<move>`) resume this way; state changes and one-shot commands such as `<fire>` run only when the key is actually pressed, so holding Space through the main menu does not pause the game. Alternatives that were considered are in [designs/04](designs/04-states-conditions-and-input.md#held-keys-across-state-changes).

**Accelerate.** `<accelerate direction="up">amount</accelerate>` in an object `<action>` is held like a `<move>`, and is resumed the same way after a state change, but it adds to the velocity instead of setting it: each frame the key is down, every direction held contributes its `amount` (so opposite thrusters cancel, and two directions make a diagonal push), on top of the object's own `<acceleration>`, before the frame's move. What was gained stays: let go of the key and the object drifts on at the speed it reached. `<thrust>` is the same along the object's `<heading>`, and `<turn>` changes the heading by a fixed amount a frame while held; an object's `<drag>` then takes a fraction of the speed away each frame, so thrust has a top speed. With `burn="variable"` each thruster that is on takes 1 off that variable of the object's every frame, and does nothing once it is gone.

**Hop.** `<hop direction="up">distance</hop>` (and `down`, `left`, `right`) in an object `<action>` is a one-shot jump, not a held move. Pressing the key queues a hop of `distance` pixels; the next frame's move makes it before collisions are worked out, so the object is judged where it lands, and it is refused (the object stays where it is) if it would leave the window. Holding the key does nothing more and releasing it does nothing, and a state change never repeats it: only continuous bindings are resumed, so a key held through a pause has to be pressed again to hop. Two hops asked for in one frame keep the later one, so a hop is always one step in one direction.

## Collisions

Each object has `<collision>` rules. A rule's content is a list of command tags, run in order. There are two kinds:

- **Screen edge:** `<collision edge="left">`, `"right"`, `"top"`, `"bottom"`, plus the groupings `"vertical"` (top and bottom), `"horizontal"` (left and right) and `"all"`. Several rules that touch the same edge all run.
- **Object against object:** name what the rule applies to with `class="..."` (a kind of other object, matched against that object's `class` attribute) and/or `object="..."` (one named object), for example `<collision class="bricks"><die /></collision>`. A `<collision>` with no selector at all applies to anything. A `<collision>` with `edge` is always a screen-edge rule.

Detection (pure geometry, `CollisionDetector`) is kept apart from response (`CommandExecutor`). Object-against-object collisions are **swept**: each object moves along its own path for the frame, and the detector finds the moment two of them first touch (rectangles as boxes, a circle against the other object's box with rounded corners), so a small or fast object cannot jump over a thin one between frames. The earliest touch in the whole frame is handled first: everything moves up to that moment, the pair's rules run, and the rest of the frame is played with whatever velocities they left, so a bounce spends the remaining part of the frame heading away. Each pair reacts at most once per frame. The detector reports which edge of the other object was touched, and each side of the pair then sees the edge from its own point of view. Screen edges are still checked by position, before the move.

**Type.** `<type>` in `<collisions>` says how the object's shape is tested: `box` (the default: a rectangle as a rectangle, a circle as a circle, everything else as its bounding box) or `pixel`. A pair in which either object is `pixel` is first swept as boxes, exactly as above, and then looked at more closely: from the moment the boxes touch, the pair is walked along its path half a pixel at a time until some pixel drawn by one lies on a pixel drawn by the other, and that moment (found to a fraction of a pixel) is the hit. If the pixels never meet, there is no hit, however long the boxes overlapped. Pixels are asked about at their centres, at the positions the objects really are at. A `pixel` object is its sprite's bitmap (a sprite of [lines](#lines), a `<bitmap>` or an `<svg>`); an object of any other type in the pair counts as solid all over its shape. A circle or a rectangle can be `pixel` too, which is solid as itself; text and images cannot (a load error), since what they look like is only known to a window backend. Each object says its own type, so a game with a pixel lander and pixel terrain gives both `<type>pixel</type>`. The same test decides `unless=` (is it touching something of that class right now).

The edge reported for a pixel hit is the one the motion came in through (the side of the other object that the two were moving toward each other across), or the boxes' edge when they were already touching; a pixel touch knows where pixels met, not which way a surface faces, so `<bounce />` on a pixel hit reflects along the axis of the relative motion.

Rules that apply:

- Only objects that are **shown** in the current state and have `enabled="true"` take part.
- **What a rule can do.** Against a screen edge: `<bounce />`, `<stick />`, `<reset />`, `<die />`, `<stop />`, `<move>`, `<inc>`, `<dec>` and `<wrap />`. Against another object: `<bounce />`, `<deflect>`, `<die />`, `<stop />`, `<reset />`, `<move>`, `<inc>`, `<dec>` and `<carry />`. Both: `<reverse />`, `<reset object>`, `<become>`, `<reveal>`, `<release>` and `<play>`.
- **`sprite`.** A rule about another object can carry `sprite="name"`: it runs only while its own object shows that [look](#looks). `<stick />` and `<wrap />` are about a screen edge and do nothing in a rule about another object, and `<carry />` and `<deflect>` are about another object.
- **`<slower>` and `<faster>`.** A rule about another object may begin with `<slower>N</slower>` and/or `<faster>N</faster>` (values), before its commands: it only runs while the object's own speed (the length of its velocity, with what it is carried at) is **under** N (`slower`) or **N or more** (`faster`) at the moment of the touch. The speed is taken once, before any of the object's rules for this touch run, so a rule that `<stop />`s it does not change which rules after it are run. Two rules about the pad with the same number, one `slower` and one `faster`, are a landing and a crash: exactly N counts as fast. A screen-edge rule has no speed filter (a load error).
- **`unless`.** An object-against-object rule can carry `unless="class"`: it is passed over while the object is, at that same moment, touching something in play of that class. Frogger's river is `<collision class="water" unless="logs"><dec variable="frog.lives" /><reset /></collision>`: water costs the frog a life, unless it is also on a log. It looks at where things are right now, so it does not matter which of the two touches was handled first.
- A pair is skipped unless at least one of the two is moving, and unless one of them has a rule that answers to the other (its `class`/`object`, or a rule with no selector). An object counts as moving if its velocity is not zero, if it is being carried, or if it has just hopped or is jumping, so an object that lands somewhere by hopping or jumping is judged there even though nothing else in the pair moves. An object in the middle of a `<jump>` is in no pair at all.
- `<lockstep>true</lockstep>` puts all members or cells of a `<group>` in lockstep: they share a lockstep number. Cells in lockstep never collide with each other, `<move>` in a collision moves all of them, and one hitting the left or right screen edge with `<bounce />` moves the whole block (this is how the invaders march). Being in lockstep changes none of the geometry: every cell is swept on its own, so a bullet only ever meets the cells that are still alive, and there is no bounding box around the block.
- Every cell of a group in columns and rows is its own object, named after the group with its column and row counted from 1: a group called `aliens` has `aliens.1.1`, `aliens.2.1`, ... `aliens.11.5`. The group's name still works where the whole set is meant: `<show object="aliens"/>`, a rule or condition `object="aliens"`, `<reset object="aliens" />`; a cell can be named on its own (`object="aliens.3.2"`). A listed group's members are named by number (`logrow3.2`); see [`<group>`](#group).

## Conditions

```xml
<condition class="paddle" variable="score">
  <atleast>15</atleast>
  <push state="gameover" />
</condition>
```

Checked once per frame while the state is current. It fires when any object matching `class` and/or `object` (both optional, combined with AND) has a variable named `variable` that has reached the `<atleast>` value (greater than or equal). The first match runs its commands and stops checking for that frame. The threshold is an ordinary value, so it is not limited to 0-255 and can be an expression.

The other forms:

```xml
<condition class="aliens">
  <remaining>0</remaining>
  <push state="gameover" />
</condition>
```

`<remaining>` counts objects instead of reading a variable. It fires when no more than that many of the matching objects are still in play, that is still visible (`<die />` hides an object). `0` means they are all gone, which is how Space Invaders is won: `class="aliens"` covers every cell of the group, and `object="aliens.3.2"` would watch a single one. `<atmost>` reads a variable from above: `<condition object="frog" variable="lives"><atmost>0</atmost>...</condition>` fires when the variable has fallen to that value or below, which is how a lives counter that goes down with `<dec>` ends the game (Frogger). A condition uses exactly one of `<atleast>`, `<atmost>` or `<remaining>` (the schema enforces this). If the filter matches no object at all, the game warns when it loads, since the condition would fire at once. Ending the game with a `gameover` state and starting over with a bare `<reset />` on that screen works as in Pong; `<reset />` also brings the dead aliens back.

Besides changing the state, a condition can `<inc>` and `<dec>` a variable, `<become>`, `<reveal>`, `<play>` and `<trigger>`. A condition that changes the variable it reads can count and start over: Frostbite's `<condition object="bailey" variable="blue"><atleast>4</atleast>` turns the four rows of ice white again and takes 4 off the count.

## What happens when a game runs

1. **Parse and validate.** The XML backend loads the file. If the file names a schema, it is validated: Xerces does full XSD validation ("strong"); the other three backends use a small built-in validator for the subset of XSD this project uses ("weak", `xsd_lite`). `printGame()` reports which one ran.
2. **Evaluate.** exprtk evaluates every value once (an expression text, or a value tag such as `<random>`, which is drawn here). Objects (a group's members and cells among them), their variables, states and sounds are built. Nothing here needs a window or a sound device.
3. **Open the window and the sound.** `Engine` creates the window backend and the audio backend (if the sound library will not start, it prints a warning and the game plays silently), makes every sound into samples and hands them to the audio backend, measures each object's real size for drawing and collisions, finishes the position of any text or image that uses `objectName.width` or `objectName.height`, then pushes the first state. The program prints the game twice, once before this step (sizes and size-dependent positions shown as unknown) and once after.
4. **Loop.** Each frame: read key changes and run the current state's bindings for them; count the frame towards the next picture of every shown object that has an [animation](#animation); count the frame on every [timer](#timers) of a shown object and of the current state, and run the ones that go off; set the velocity of every shown object on a [path](#paths) for its part of the path (starting its next leg, and running that leg's commands, when one is done); change every shown object's velocity by its acceleration and held thrust; run the screen-edge rules; make any queued hops; move every shown object by its velocity, and by what it is being carried at, running the object-against-object rules at each touch on the way (per frame, not scaled by time); check conditions; play the sounds the frame asked for; clear; draw shown objects; present.

## Backends

| Job | Interface | Implementations |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml_document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keyboard | `Window` (`window.h`) | SFML3 (default), Raylib, SDL2, OpenGL (GLFW) |
| Sound | `Audio` (`audio.h`) | SFML3 (default), Raylib, SDL2, None (silent) |

A program is built with some of these, not all: SFML 3 and Xerces alone by default, every one with `-DXGE_ALL_BACKENDS=ON` or `-DBUILD_TESTING=ON`, each other one with its own `XGE_WITH_<NAME>` (see the [readme](../readme.md#backends)). The enums always name all of them, so a name on the command line or in a settings file still means something; what is missing is only the library. Each factory says what it has: `WindowFactory::available(backend)`, `availableBackends()`, `defaultBackend()` (SFML 3, Xerces, SFML 3's sound, or the first built when those are left out) and `name(backend)`, and the same on `XmlDocumentFactory` and `AudioFactory` (`None` is always built). `create()` for a backend that is not built throws, saying which are. A backend's source files are only compiled when it is built, and its `XGE_WITH_<NAME>` definition is what the factory looks at.

One more library is used by the engine itself and is not a backend: lunasvg draws the SVG files an `<svg>` sprite names, into the same kind of bitmap a `<bitmap>` makes, so no window backend knows about SVG ([design 07](designs/07-pictures-and-text.md)). Only `svg.cpp` includes it.

`Game` and `Engine` only ever see the interfaces. Each interface has a factory that is the single place that knows every implementation (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`).

The sound library is chosen apart from the window library, and any goes with any (OpenGL, which has no sound of its own, included). `Game` never makes a noise: `<play>` only asks for a sound (`Game::requestSound`), and `Engine` hands what a frame asked for to its `Audio`, so a game runs and is tested with no sound device. The engine makes every sound's samples itself (`synthesize()`, `sound.h`), so a sound is the same whichever library plays it; a backend only hands the samples to its library: an `sf::SoundBuffer` and `sf::Sound` (SFML 3), a `Sound` made with `LoadSoundFromWave` (raylib, which opens its sound device apart from its window), or a small mixer of its own in an SDL audio callback (SDL2, which has no mixer without SDL_mixer). The SDL2 window and the SDL2 sound each start and stop only their own part of SDL, so either can go first. `Engine::replaceAudio()` swaps the sound library of a running game, which is how the Options dialog changes it, and `Engine::silence()` stops what is playing, which `xgegui` does when the game is paused. `NullAudio` plays nothing: the tests use it, and so does `-a none`. Build-time dependency selection is in `scripts/cmake/` (see the `FORCE_LOCAL_*` options in `options.cmake`).

Every window backend opens a window of its own, which is what `xgecli` uses. `xgegui` shows the game in one of two layouts ([design 12](designs/12-front-ends.md)): in one window, drawn by a renderer of its own that draws with Qt (`QtWindow`, the default), or in two, where the main window holds only the controls and the tree and the game is in a window of its own, opened by the chosen library (SFML3, SDL2, raylib or OpenGL) or by the Qt renderer. `Engine::replaceWindow()` swaps the window of a running game for another without touching the game, which is how the Options dialog changes the video library. A front end that has paused the game but keeps its window calls `Engine::pump()` so the window can still be moved and closed, and `Engine::isWindowOpen()` tells it when the user closed it.

Nothing in `xgegui` uses OpenGL through Qt, so a library's OpenGL context is the only one on the thread. (The game view was a `QOpenGLWidget` once, and the two kinds of context, which Qt tracks by its own record, drew into each other; see [design 12](designs/12-front-ends.md).)

## Building: library and programs

The engine (everything in `lib/source/` and `lib/include/`) is a library, the `xgelib` CMake target. Two programs use it: `xgecli` (`cli/`) and `xgegui` (`gui/`, the Qt application, built only when Qt 6 is found), and so do the tests (`xgetest`, in `tests/`). Every project has the same layout, a folder with `source/` and `include/` in it; `xgedata` is the target that copies `games/`, `assets/` and `xgedef.xsd` next to the programs. Programs, games and assets go in `output/<config>` at the top of the repository (`output/Debug`, `output/Release`), and the DLLs (`.so` files on Linux) and Qt's plug-ins in `output/<config>/libraries`, set up in `scripts/cmake/output.cmake` (on Windows the programs find them through a manifest, see [design 11](designs/11-backends-build-and-layout.md)); the build directory keeps only the compiler's and linker's own files. `output/` is ignored by git. The library knows nothing about the command line, so a different front end only needs its own `main()`.

The library is static by default: each program has the engine's code copied into it, so `xgecli` is one self-contained file. `-DXGE_BUILD_SHARED=ON` builds it as a shared library instead (`libxgelib.so`, or `xgelib.dll` on Windows), which each program loads when it starts; a shared build needs the library file to be found next to the program or on the system's library path. On Windows the DLL exports every class in the headers (`WINDOWS_EXPORT_ALL_SYMBOLS`) rather than each being marked by hand. The third-party libraries are `PUBLIC` dependencies of the library, because the engine's own headers include theirs. See [design 11](designs/11-backends-build-and-layout.md).

## Source map

| File | Responsibility |
|---|---|
| `cli/source/main.cpp`, `cli/source/cli.cpp` | xgecli, the command line program: read the options, find the game file, build `Game` and `Engine` with the chosen backends. The only code outside the engine library |
| `cli/source/generate.cpp`, `generators/windows-cpp/` | `--generate`: run a target's stylesheets on a game file with libxslt and copy the modules and assets it lists ([Generating a program](#generating-a-program)) |
| `gui/source/*.cpp` | xgegui, the Qt application ([design 12](designs/12-front-ends.md)): `main_window` (the window, the File and View menus, the question about two windows), `game_session` (a loaded game and its engine, run from a timer; play, pause, step, reset, and changing the libraries), `game_stage` (where the Qt renderer's picture is: the left pane, or a window of its own), `game_view` (the widget a picture is shown in), `key_queue` (the keyboard, read by Qt), `qt_window` (the `Window` that draws with QPainter), `options_dialog` and `session_options` (the video library, XML parser and sound library choice), `app_settings` (`xgegui.ini`, next to the program), `inspector` (the controls and the tree of game data) |
| `game_xml.cpp` | Walk the parsed XML tags into raw window/variable/object/state data (`RawValue`, `RawCommand`, `RawSprite`); a `<group>` is read here as one raw object per member |
| `game_expr.cpp` | exprtk symbol table and evaluation of raw values (expressions, `<random>`, `<equation>`, `<formula>`) into `Object`s and `State`s |
| `command.cpp` | Turn raw command tags into typed `Command`s; the table of arithmetic operations (`operationShape`) and a value printed as text (`valueText`) |
| `game.cpp` | Objects, state stack, per-frame update, collision pairs, conditions, resets, paths (`follow`, `applyPaths`) |
| `collision_detector.cpp` | Geometry only: box, circle and swept tests, and the pixel pass for type `pixel` |
| `builtin_font.cpp` | The 8x8 font stored in the program (`rasterizeText`), which draws text into a bitmap when a backend cannot load its font file |
| `bitmap.cpp` | Draws a sprite's `<line>`s (`rasterizeLines`) or `<bitmap>` rows (`rasterizeRows`) into an RGBA bitmap, the pixels both the window backends and pixel collisions use, and flips (`flipBitmap`) and turns (`turnBitmap`, `rasterizeTurned`) them |
| `svg.cpp` | Draws a part of an SVG file (`rasterizeSvg`) into an RGBA bitmap, with lunasvg; the only file that includes it |
| `command_executor.cpp` | What each command does |
| `sound.cpp` | Pitch names, wave names, and `synthesize()`: a sound's notes made into 16-bit samples, the same for every audio backend |
| `audio.cpp`, `audio_*.cpp` | `AudioFactory` and the sound backends (SFML 3, raylib, SDL2; `NullAudio` is in `audio.h`) |
| `engine.cpp` | Frame loop (`loop()`, or `step()` and `render()` for a front end that owns the event loop), key handling, and playing the sounds each frame asks for |
| `object.h`, `states.h`, `color.cpp`, `keycode.cpp` | Data model, named colors, key names |
| `window_*.cpp`, `xml_*.cpp`, `xsd_lite.cpp` | Backends and the weak validator |
| `tests/` | Catch2 tests (opt-in with `BUILD_TESTING`): collision geometry and swept collision, command parsing, conditions, input resolution, `stick()`, collision rules, lockstep bounce, size expressions, engine key handling, object variables, the new verbs (`dec`, `hop`, `wrap`, `carry`, `unless`, `atmost`, colors), the tag format, its rejections and the names commands use (`test_xml_format`), arithmetic as tags (`test_equations`: the three spellings agreeing, each operation in both forms, chains, nesting, step names, every rejection with its message, a divisor of 0 at load and while running, both schema checkers, and Pong's and Breakout's titles), groups (`test_group`: expansion, overrides, names, lockstep, columns and rows with what a row, column or cell changes, gaps, picks, clashes, errors, both schema checkers), lines, pixel collisions, acceleration, thrust and the speed filters (`test_lines_and_pixels`), bitmaps, animations and Space Invaders as written with them, with both schema checkers (`test_bitmap_sprites`; `invaders_fixture.h` keeps a copy of the first, plain Space Invaders for the tests that are about a block of cells and not about that game), the built-in font, the command line and the data folder search, sound (`test_sound`: pitch names, the synthesizer, `<sounds>` and its rejections, Pong asking for its sounds, `Engine` and a recording `Audio`), the one path of drawn pictures and `<flip>` (`test_pictures`), timers, facing, jumps, `<reverse />`, looks, `<reveal>`, conditions that count, key sets, `<chase>` and `<aim>` (`test_gameplay_verbs`), `Engine::pump()` and `isWindowOpen()` (`test_engine_input`), and Frogger, Space Race, Kaboom, Freeway, Depth Charge, Astrosmash, Lunar Lander and Asteroids (`test_asteroids`: headings, turning, thrust and drag, the pool of shots, `release`, wrapping, losing ships, winning) played frame by frame , Demon Attack (`test_ai_games`: played from the title to the end screen, with the demons firing back), Berserk (`test_berserk`: the man's four looks and shots, robots patrolling and firing at the man, the exits and the room bonus, Otto chasing him, extra men, pausing and game over), paths and Galaxian (`test_galaxian`: steps, home, a start, a stagger, a step's commands, what ends a path, the mistakes, the fleet flying in, a dive that wraps and comes home, a new fleet), Frostbite (`test_frostbite`: jumps, rows turning blue, the igloo, the cold, drowning, the geese and the fish), Donkey Kong (`test_donkeykong`: standing on girders, walking off an open end, a leap's height and fixed arc, ladders up and down, a barrel's way down to the oil drum, a barrel jumped and one not, saving Pauline), and the five Atari games: Pitfall! (`test_pitfall`: screen to screen and round, a log, the tar pit leapt and fallen into, the crocodiles and their jaws, crossing the pond, the ladder, the treasures, the clock), Missile Command (`test_missilecommand`: missiles chasing cities and turning when one goes, a counter-missile bursting at the sight, a burst taking a missile, a city lost, the waves), Combat (`test_combat`: driving, backing and turning along the heading, a shell from the gun, a hit, a wall), Air-Sea Battle (`test_airseabattle`: the lanes going round, a gun swung and fired at an angle, a plane shot down for the right player and coming back, the clock) and Megamania (`test_megamania`: a wave wrapping, bombs, a shot, the energy, the cookies bouncing, the waves won) |

## Known limitations

- Object-object collision knows only the four edges of the other object (and, for a pixel hit, the side the motion came in through); there are no verbs beyond the table above (the jumps are a step (`hop`), a straight line (`<jump>`) and a fixed arc with no air control (`<leap>`); no shooting patterns or AI strategies beyond `<chase>` and `<aim>`).
- A path is straight steps, not curves: a loop is a few steps round it, and the steps are flown at one speed, with no easing. A path cannot aim (a dive weaves the same way whatever the ship does) and its numbers are fixed when the game loads. Home is the object's place in the game file; a formation that moves (Galaxian's sways from side to side) would need home to move with it. A follower that is stopped or pushed on the way (a `<stick />`) still counts the step as flown.
- Enemies fire on timers from wherever they are: an invader behind others fires through them (in the arcade game only the bottom one of a column fires). `<chase>` goes straight at its target and `<aim>` straight at where it is, through walls and with no lead; a Berserk robot walks its corridor whether or not the man is near. Firing needs a pool of hidden projectiles to take from; with all of them in flight a timer that goes off does nothing.
- A condition's `<atleast>`, `<atmost>` and `<remaining>`, and the amount of an `<inc>` or `<dec>`, are values worked out once when the game loads, so they cannot follow a variable (`<atleast>player.nextlife</atleast>` is a constant, and a condition written to move its own goalposts fires every frame). A timer's `<every>` and `<after>`, and the ends of a `<random>` in them, are worked out again each round and can; a count that has to start again is kept in a variable of its own object, which `<reset object>` of that object puts back (Berserk's tally of the robots shot in a room, and its score since the last extra man).
- Timers count frames, like everything else, so a machine that cannot keep up slows the clock with the game. There is no way to show how long a timer has left, other than a timer that counts a variable down.
- A look is a whole picture; there is no way to recolour one. An object with a `<heading>` or an `<animation>` has no looks, and looks do not combine with an animation (an animated object that changes look).
- `<facing>` is one of four directions; a picture does not turn with it (use `<heading>` for that).
- Sound is short effects and little tunes, not music: one wave at a time per sound (no chords inside one sound; two sounds at once do play together), no looping background tune, no stopping a sound from the game file, no volume envelope beyond the click-free fade (a note does not decay like a plucked string), and no stereo. A sound is made when the window opens, so its notes cannot change with a variable while the game runs. It starts on the frame it is asked for, so it is as late as the sound library's buffer (a few hundredths of a second).
- An object that is not visible is not moved and does not collide, whatever its velocity; that is how a bullet waits, unseen and still, to be fired. There are no hidden objects that still move and collide (an invisible trigger zone, an off-screen enemy on its way in); a game that wants one will need visibility and "in play" to be separate things.
- An object's velocity and collisions belong to the object, not to a state: any state that shows it lets it move. There is no way to show the Space Invaders aliens standing still behind the menu and have them march only in `playing`; they start marching as soon as they are shown. See [designs/04](designs/04-states-conditions-and-input.md).
- An object's own `<variable>` named `width` or `height` shadows its measured size (`objectName.width` then reads the variable). A non-colliding spelling is under consideration; see [designs/02](designs/02-values-variables-and-names.md).
- A text or image placed with `objectName.width` / `objectName.height` is re-placed only when the size of an object it names changes, so it is not re-centered after it has moved on its own, and other expressions (velocity, variables) see those sizes as 0 for unmeasured text and images.
- Text is drawn with `assets/tuffy.ttf`, found relative to the directory the program is run from. If that file cannot be found, each backend prints `error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead` once, and draws every text with the font stored in the program: 8 by 8 pixel glyphs for printable ASCII (anything else is a `?`), scaled to about `<size>` / 16 in blocks tall (so a letter is 8 times that tall) and half as wide, so it is chunky and monospaced and its width differs from the real font's ([design 07](designs/07-pictures-and-text.md)).
- Movement is in pixels per frame with no time step. Acceleration is constant per object (`<acceleration>`, `<accelerate>`); an object can face a heading and be pushed along it (`<heading>`, `<turn>`, `<thrust>`), and `<drag>` is the only thing that slows an object by itself apart from a rule or an opposing thrust.
- A `pixel` collision is only as exact as half a pixel of motion: a pair is looked at every half pixel along the longer way it moves in a frame, and two one-pixel lines can cross at a slant without sharing a pixel at all, so lines meant to be tested should be at least 2 pixels thick. It knows nothing about which way a surface faces, so there is no bounce off a slope. Text and images cannot be `pixel` yet (their pixels are only known to a window backend), so a fighting game with image sprites will need the backends to hand their pixels back. The bitmap of a sprite of lines is kept for every object that has one, four bytes a pixel, so one big drawing costs what its bounding box does (Lunar Lander's 800 by 182 moon is about 580 KB).
- A sprite of lines is measured from 0, 0 and fixed when the game loads: it cannot be moved or scaled, and it turns only on an object with a `<heading>`, in whole degrees (one picture at a time, always the same square size). Lines are straight, one color each, and have no fill.
- A `<bitmap>` is fixed when the game loads too, and has one color (an asterisk is that color, a period is clear), so a sprite in two colors is two objects on top of each other. A turning object keeps the original and the one picture it shows, so the memory it costs is about double, and a turn is done again only when the heading moves by a whole degree. The sprite pictures are separate bitmaps, so a game with many large animated sprites keeps them all in memory: four bytes a pixel for each frame, once per object (the cells of a grid share theirs).
- An `<svg>` is drawn once, when the game loads, at one scale: to show one at two sizes, make two sprites. A pixel collision counts any partly transparent edge pixel as solid. An animated object cannot have looks, so an animated alien cannot change picture on a hit; an explosion would be a pool released where it died. The banking, hit-flash, charged-bolt, explosion and saucer pictures on the Space Invaders 2 sheet are not used yet (see [design 07](designs/07-pictures-and-text.md)).
- An animation has one interval for all its frames, and counts frames of the game, not time (see [Animation](#animation)). There is no way yet to show a picture that depends on something other than time (a variable, the direction of travel, a hit), to run an animation once and stop, or to start it from a collision.
- Each cell of a group is its own object with its own count of frames. The cells stay in step because they start together (at load, and again at every reset) and only count while shown.
- A `<random>` is drawn once, when the game loads. A piece that falls, is caught and is reset falls again at the same speed from the same place, so Astrosmash's rhythm is different on every launch but repeats within one (Kaboom avoids this: its bomber's timers draw a new `<random>` wait every round). There is no way yet to draw again on `<reset />`, and nothing in the language creates a new object while the game runs, so things that fall are a fixed set that cycles, or a pool that is fired from, released or revealed. A `<timer>`'s own interval is drawn again every round.
- A hop is a jump, not a slide: the object is simply one step away, with no animation between and nothing touched on the way (a step is meant to be one lane). The arcing jump is `<leap>`.
- `carry()` lends velocity one frame at a time and only while touching. It is not attached: a carried object that meets a screen edge is handled by that edge's rule like any other (Frogger's frog loses a life), and nothing pushes it into a wall.
- A pair in which nothing moves is not looked at (see the collision rules above), so a rule against something that stands still runs only when the object moves, is carried, hops or lands from a jump.
- `unless` names a class, not a single object, and only exists on object-against-object rules.
- `wrap()` assumes what it wraps is spaced with its own size in mind (a lane of things that are `size` wide repeats every window width plus `size`), and the members of a lane (a `<group>`) are placed by hand, one `<x>` each, or evenly spaced as one row of `<columns>` with a `<padding>`: a group's cells are spaced by sizes and gaps, not spread across a width.
- Only the object-against-object test is swept. A screen edge is still checked by position before the move, so an object that moves more than a whole window's width in one frame is not caught by it; rotation is not modeled. A shape that is not a rectangle or circle (text, image) is treated as its bounding box, unless it is a sprite of lines tested by `pixel`.
- Expressions inside a value are not checked by the schema (they are text); a typo in one is found when the game loads (the load stops with a message naming where), not by validation. Structure, tag names, attributes and command verbs are checked by the schema; the names a command uses are checked by the engine when it loads.
- The weak validator (`xsd_lite`) covers only the XSD subset this project uses; Xerces is the full check. It follows a named type that contains itself (a `<formula>`'s operands), but not a group that does.
- `<equation>` and `<formula>` have four operations and no more: no `min`, `max`, `negate`, `abs`, `clamp`, `sign` or `pick` (the exprtk text has them). An operand is a name or a number, so a sum that is needed in two places is a named `<equation>` step; an `<equation>` has no `<random>` step (a `<formula>` operand can be one). Only Pong and Breakout use the tags; every other game still writes its arithmetic as text.
- A `<divide>` by something not known while the game loads (another object's variable, an unmeasured text's size) works out to 0 at load, and only a position or a timer's interval is worked out again afterwards, so an object `<variable>` made from such a division keeps the 0.
- The window's `width`, `height` and `framerate` are plain numbers, not values, and are read once, before the window opens. The size cannot be changed while a game runs, in `xgegui` either.
- `--generate windows-cpp` covers `pong.xml`, `pong_min.xml`, `spacerace.xml`, `freeway.xml`, `breakout.xml` and `depthcharge.xml` and little more: `<state>`s with `<push>`, `<pop>` and `<reset />` on keys and in conditions; circles and rectangles (white when no `<color>` is given), texts (`<content>` or the `<number>` of an object variable), pictures (`<flip>` too), `<bitmap>`s and `<line>`s (drawn when the program starts by its module `pictures.h`, pixel for pixel as the engine draws them, the rows written out in `main.cpp` so they look like themselves) and `<svg>`s (drawn by xgecli as the engine draws them, into `assets/drawn/<name>.png`; their numbers must be numbers or game variables that are), one look each; `<group>`s of circles, rectangles or pictures (one kind to a group, one sprite each; a `<member>` gives its `<sprite>`, whole or only what changes, `<position>` and `<velocity>`; a group in `<columns>` and `<rows>` takes `<row>`s, `<column>`s and `<cell>`s that change the sprite, the `<velocity>` and the `<padding>` of what they pick, laid out as the engine lays them out; no `<variables>` or `<actions>`, the group's or a row's; only `<wrap />`, `<die />`, `<play>`, `<inc>` and `<dec>` in their own rules, and `<bounce />` off the sides for a group in `<lockstep>`, which turns the whole block); game-wide `<variables>` (no `<random>` in them), an object's own `<variables>`, an object's `width` and `height`, and the window's names in expressions; edge rules with `bounce`, `stick`, `wrap` (on its own in its rule), `reset` and `die`, object rules with `bounce`, `deflect`, `reset` and `die` (a flag for each thing that can die, so it is no longer drawn, moved or touched), and `<play>`, `<inc>` and `<dec>` (of an object variable, `paddle1.score`) in either; `<input>`s that either `<trigger>` an action of `<move>`s (held), on an object with no velocity of its own, or do the rest, a `<hop>` or a `<fire>` among them (pressed; a projectile is an alive flag that `<fire>` sets, placed at the middle of the shooter's top with its own velocity, one at a time); `<condition>`s with `<atleast>` or `<atmost>` on an object variable, or `<remaining>` (a whole number) of an object, group or class; and `<sounds>`. Its rules are simpler than the engine's: an object moves, then is checked against the edges and the objects it can touch where it lands (no sweep, so a fast small object can pass through a thin one), and a touch is told apart by which way the two overlap least. Pong's ball, at most 7 pixels a frame against paddles 30 wide, does not show the difference. When two things each have a rule about the other, both rules run in one touch, as in the engine, and so do all the rules about one edge. Something with no velocity and no keys never meets an edge, so its edge rules are left out. A rule about a class is written once for each group or object in it (Space Race's rockets look at each of the nine lanes in turn). Every other game stops at its first tag: `<hidden>` (Space Invaders, Space Invaders 2, Frostbite), `<heading>` (Asteroids, Combat), `<facing>` (Berserk, Kaboom), a collision `<type>` (Lunar Lander), a `<reset />` of one member of a group (Astrosmash), `<reset object>` (Megamania), an object's `<timers>` (Pitfall!), `<acceleration>` (Donkey Kong), `<wrap />` on an object moved by keys (Demon Attack), `<paths>` (Galaxian), a `<collision>` with `unless=` or `sprite=` (Frogger), and an object named like a game variable, which in C++ would be two things of one name (Missile Command's `ground`, Air-Sea Battle's `sea`).
