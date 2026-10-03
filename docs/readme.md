# XMLGameEngine: current design

This document describes what the engine does **today**, as read from the source on 2026-10-01 (before that, games were written as function-call attribute strings; see [designs/03](designs/03-expression-syntax.md)). For the reasoning behind these choices, the alternatives that were considered, and ideas that are not built yet, see [designs/00-designs.md](designs/00-designs.md).

XMLGameEngine is a video game description language (VGDL) written in XML, plus a C++ engine that loads a game description and runs it. A game is one `.xml` file. The description is declarative: there are no loops and no `if` statements in it, and no function calls either. Behavior comes from a fixed vocabulary of verbs (the tags `<bounce />`, `<stick />`, `<die />`, ...) that the engine knows how to carry out. In this document, and in the design notes, a verb is sometimes written as `bounce()` for short; in a game file it is always the tag.

## Running a game

```
XGECLI                       # loads "pong"
XGECLI breakout              # a bare name gets ".xml" appended
XGECLI pong.xml              # a name with an extension is used as given
XGECLI -g pong -w sdl2 -x tinyxml2
XGECLI --game pong --window raylib --xml pugixml
XGECLI pong -a sdl2          # sounds played by SDL2 (or -a none for silence)
```

The game is a bare argument or `-g` / `--game`; both are looked for the same way: as given (a path, or a name in the current directory), then its file name in the working directory, then its file name in `games/` of the data folder (the first of the working directory, the program's folder and the folder above it that has both `games/` and `assets/`; the program then runs from there, so it can be started from anywhere). If it is not found the program prints an error and exits. `-w` / `--window` picks the window library (`sfml3`, `raylib`, `sdl2`, `opengl`; default `sfml3`) `-x` / `--xml` the XML library (`xerces`, `tinyxml2`, `pugixml`, `rapidxml`; default `xerces`) and `-a` / `--audio` the sound library (`sfml3`, `raylib`, `sdl2`, `none`; default `sfml3`), not case sensitive. Any sound library goes with any window library. A short option takes its value attached or after a space (`-gpong`, `-g pong`); a long option needs the space (`--game pong`, not `--game=pong`). Each option can be given once, the game only once (bare or with `-g`), and `-h` / `--help` prints the usage. The program starts by printing the file, window library, XML library and sound library it chose, one to a line. See [design 37](designs/37-command-line.md). Shipped games: `games/pong.xml`, `games/breakout.xml`, `games/spaceinvaders.xml`, `games/frogger.xml`, `games/spacerace.xml` (two players, W/S and Up/Down, first to two points), `games/kaboom.xml` (A/D or Left/Right; catch bombs in three waves, three missed bombs end the game, 60 points win), `games/freeway.xml` (two players, W/S and Up/Down, first to five crossings), `games/depthcharge.xml` (A/D or Left/Right to move, Space to drop, Space to start; sink all nine submarines before eight charges are wasted) `games/astrosmash.xml` (A/D or Left/Right to move, Space to fire, Space to start; shoot 20 rocks before five land) and `games/lunarlander.xml` (Up or W for the main thruster, Left/Right for the side ones, Space to start; set the lander down on the green pad slower than the safe speed, with fuel to spare, and do not touch anything else), `games/berserk.xml`, `games/demonattack.xml` and `games/frostbite.xml` (A/D or Left/Right, W/S and Up/Down where the game goes up and down, Space to start and to fire; three small games written by another AI from the schema alone, see [design 44](designs/44-games-written-by-another-ai.md)).

The XML, window and sound libraries are chosen in C++ with `Game(file, XmlBackend)` and `Engine(game, WindowBackend, AudioBackend)`; `XGECLI` passes what `-x`, `-w` and `-a` named, Xerces, SFML3 and SFML3 when they are not given. `XGEGUI` (the Qt application) takes the game the same way, `XGEGUI pong`, or opens a file dialog in `games/` when none is named; its Options dialog picks the video library, the XML parser and the sound library. See [Backends](#backends).

A game file that is wrong (it does not match the schema, an expression will not evaluate, a command names a state or object the game does not have) stops the load with a message saying where; `XGECLI` prints it and exits, `XGEGUI` shows it and carries on.

## Game file layout

A game file has one `<game>` root with four children, and an optional fifth (`<sounds>`), in this order, as enforced by [assets/xmlgameengine.xsd](../assets/xmlgameengine.xsd):

```xml
<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
      xsi:noNamespaceSchemaLocation="../assets/xmlgameengine.xsd">
  <window name="..."> ... </window>
  <variables> <variable name="...">...</variable> ... </variables>
  <sounds>    <sound name="..."> ... </sound> ... </sounds>   (optional)
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

Wherever the content of an element is a number (a position, a velocity, a size, a variable's value, a threshold, a step) it is a **value**, and a value is one of two things:

- **An expression written out as text.** Ordinary arithmetic, with the names below: `window.width.center - title.width / 2`, `margin`, `ball.radius * 2`, `30`. It is evaluated by exprtk at load time.
- **One value tag**, which makes the number. Today there is one: `<random min="-7" max="7" />`, a random number from `min` to `max` (in either order; both ends are themselves expressions). Any place that takes a value takes it:

```xml
<velocity>
  <x><random min="-7" max="7" /></x>
  <y><random min="-3" max="3" /></y>
</velocity>
```

A value holds text or a tag, not both, and not two tags (`<x>5 <random .../></x>` is an error naming the element). A value tag that is evaluated at load gives one number for the whole run: a `reset()` puts an object back to the position and velocity it started with, not a new draw.

The names an expression can use:

| Name | Value |
|---|---|
| `window.top`, `window.left` | 0 |
| `window.bottom`, `window.right` | window height, window width |
| `window.width.center`, `window.height.center` | half the width, half the height |
| any global `<variable>` | its value. They are worked out in the order written, so a variable can use the ones above it |
| any `objectName.variableName` | that object's variable (available regardless of the order objects appear in the file) |
| `objectName.width`, `objectName.height` | the width and height of any object (its sprite's footprint); an object refers to itself by its own name, like any other field. Meant for `<position>`. An object's own `<variable>` named `width` or `height` wins over its size. A circle's or rectangle's size is known from its sprite, so the position is exact at load. A text's or image's size is only known once the window has measured it, so its position is finished then, and worked out again whenever the size changes (a score gaining a digit); until then `printGame` shows it as unknown. |

The other exprtk math functions (`min`, `max`, `sqrt`, ...) exist because exprtk brings them, but the game files do not use them and nothing about the format depends on them.

### `<window>`

`<window name="Pong">` (the title), then in this order:

| Element | Meaning |
|---|---|
| `<width>`, `<height>` | Size in pixels. Plain whole numbers: the expressions in the rest of the file are worked out against the window's size, so these cannot be expressions themselves |
| `<background>` | A color name, e.g. `color.black` |
| `<fullscreen>` | `true` or `false` |
| `<framerate>` | Frames per second (0-255) |

### `<variables>`

Global named numbers. Each `<variable name="margin">30</variable>` becomes a constant that any expression in the file can use (for example `margin`, `ball.radius`). Names may contain dots. The content is a value, so `<variable name="b">a * 2</variable>` and `<variable name="start"><random min="1" max="3" /></variable>` both work. Declaring a name twice keeps the later value.

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

Every sound is made into samples (16 bit, one channel, 44100 a second) by the engine when the window opens, the same for every sound library ([design 45](designs/45-sound.md)). Each note fades in and out over a couple of milliseconds so it does not click. Each sound has one voice, like a channel of a sound chip: playing a sound that is still playing starts it again from the beginning, and different sounds play over each other. A sound asked for more than once in one frame is played once.

A wave, pitch or length the engine cannot use, a `<volume>` outside 0 to 1 or after a note, a sound with no notes, two sounds of one name, and a `<play>` naming no sound all stop the game loading with a message saying where (`sound 'wall' > <note> 1: pitch="H2" is not a pitch; ...`).

### `<object>`

An object is anything that can be drawn, moved, or collided with: a ball, a paddle, a score readout, a title. Attributes: `name` (required, how other elements refer to it) and `class` (optional, a group label that collision rules and conditions can match, like a CSS class).

Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<sprite>` | yes | What the object looks like: one shape (see [Sprites](#sprites)), or a `<grid>` of them. An object with an `<animation>` has several, each with a `name` |
| `<animation>` | no | Which of the object's sprites are shown, in what order, and for how many seconds each: see [Animation](#animation). Objects, and groups and their members |
| `<position>` | yes | Starting position, in pixels from the top-left: `<x>` and `<y>`, each a value. May use an object's own size by its name, `title.width` and `title.height`, to place it by its size, for example a text called `title` centered: `<x>window.width.center - title.width / 2</x>` |
| `<velocity>` | yes | Starting velocity, in pixels per frame: `<x>` and `<y>`, each a value |
| `<acceleration>` | no | A constant pull: `<x>` and `<y>`, each a value, added to the velocity once every frame before anything moves, for as long as the object is shown. Gravity is an acceleration with only a `<y>`. `<stop />` takes it away and a reset gives it back. Objects only, not groups |
| `<heading>` | no | The way the object faces, in degrees clockwise from straight up (0 is up, 90 is right). It lets `<turn>` and `<thrust>` work, and the sprite (of lines or a `<bitmap>`) is then drawn turned to the heading, to the nearest whole degree, so a `pixel` collision follows it. A collision `<reset />` puts it back. Objects only |
| `<drag>` | no | A value from 0 up to (not including) 1: the fraction of its velocity the object loses every frame, after its thrust is added. Objects only |
| `<hidden>` | no | `true`: the object starts out of play (not drawn, no collisions, not counted by a condition's `remaining`) until a `<release>` or a `<fire>` brings it in. Objects, and groups (after `<velocity>`) |
| `<collisions>` | yes | `<enabled>` (`true`/`false`), an optional `<lockstep>` (`true`), an optional `<type>` (`box`, the default, or `pixel`), then zero or more `<collision>` rules. See [Collisions](#collisions) |
| `<actions>` | no | Named actions the object can perform, each `<action name="up"><move direction="up">step</move></action>`: the commands are what it does. States bind keys to these names |
| `<variables>` | no | Variables owned by this object, each `<variable name="score">0</variable>`. Other expressions refer to them as `objectName.variableName`, e.g. `paddle1.score` |

An object with `class="projectile"` starts invisible (used for bullets).

### `<group>`

A `<group>` is several objects that share a description, written once: a lane of logs, a row of debris, the pads along the top of Frogger. It sits beside `<object>` under `<objects>`. It holds the parts its members have in common, in the same order as an object (`<sprite>`s, an `<animation>`, `<position>`, `<velocity>`, `<collisions>`, `<actions>`, `<variables>`, each optional except `<collisions>`), then one or more `<member>`s. Attributes: `name` (required) and `class` (optional, and every member's class).

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

A member is an ordinary object: it is loaded as one object of its own (`logrow3.1`, `logrow3.2`, `logrow3.3`: the group's name, a dot, and its number counting from 1 in the order written, unless the member says `name="..."`), it is drawn, moved and collided with on its own, and each `<wrap />` or `<die />` happens to that one member. A group only shares what is written. Members are drawn in the order written, at the place in the file where the group stands. The group's name means every member wherever the file names an object (`<show object="logrow3" />`, `object="pads"` in a rule or condition, `<reset object="pads" />`), and a member can be named on its own (`object="logrow3.2"`). A group with `<lockstep>true</lockstep>` in its `<collisions>` moves as one block, like the cells of a `<grid>`.

A `<grid>` and a `<group>` differ in what they repeat: a `<grid>` makes identical cells on a regular pattern from one sprite, a `<group>` lists members that can each differ in place, shape or speed and share everything else.

### Sprites

A `<sprite>` holds one shape. Colors are named (`color.red`, below); a `<color>` left out is `color.white`. A sprite may be given a `name`, which an object needs only when it has several sprites for an [`<animation>`](#animation) to choose between.

| Shape | Contents | Draws |
|---|---|---|
| `<circle>` | `<radius>`, `<color>` | A filled circle |
| `<rectangle>` | `<width>`, `<height>`, `<color>` | A filled rectangle |
| `<text>` | `<content>` (a fixed label) **or** `<number>` (a value), then `<size>`, `<color>` | Text. `<content>PONG</content>` is written as is. `<number>paddle1.score</number>` shows a number: when the value is exactly one `owner.variable` it is live and redraws when that variable changes; any other value (`paddle1.score + 1`, a `<random>`) is worked out once |
| `<image>` | `<path>`, then `<flip>` (`horizontal` or `vertical`, optional) | An image file |
| `<line>` (one or more) | `<from>` and `<to>` (each an `<x>` and a `<y>`), then `<color>` and `<thickness>` (both optional) | Straight lines, all in one sprite. See [Lines](#lines) |
| `<bitmap>` | one or more `<row>`s of `.` and `*`, then `<scale>` and `<color>` (both optional) | A picture written as rows of text. See [Bitmaps](#bitmaps) |
| `<svg>` | `<path>`, then `<x>`, `<y>`, `<width>`, `<height>` (all four or none), `<scale>` and any number of `<hide>` (all optional) | A drawing, or a part of one, from an SVG file. See [SVG pictures](#svg-pictures) |

### Lines

A sprite of one or more `<line>`s is a drawing. Each `<line>` goes from a point to a point, in pixels from the **top left of the sprite** (so the coordinates are never negative), and may have a `<color>` (default `color.white`) and a `<thickness>` in pixels (a value, at least 1, default 1):

```xml
<sprite>
  <line><from><x>10</x><y>2</y></from><to><x>20</x><y>2</y></to></line>
  <line><from><x>20</x><y>2</y></from><to><x>25</x><y>7</y></to><color>color.red</color><thickness>2</thickness></line>
</sprite>
```

The lines are drawn once, when the game loads, in the order written (a later one covers an earlier one where they meet), into a bitmap the size of what was drawn: as far right and down as the furthest endpoint plus the thickness, from 0, 0. Endpoints are rounded to whole pixels and a line is drawn with no gaps, stamping a square of its thickness along it. Every pixel nothing was drawn on is transparent. That bitmap is the sprite: the window backends show it as a picture, its size is the object's size (so `name.width` and `name.height` work as for any shape, with no window needed), and a collision of [type pixel](#collisions) looks at the same pixels, so what is drawn is exactly what is tested. A `<random>` in a coordinate is drawn once, so the picture and its size agree.

A sprite of lines is not repeated by a `<grid>`, and cannot be mixed with another shape. One object can be a whole drawing: Lunar Lander's moon is one object of fifteen lines, its lander another, its pad a single thick line.

A `<grid>` repeats a shape as a grid of separate objects (Breakout bricks, Space Invaders): `<columns>`, `<rows>`, an optional `<padding>` (with `<x>` and `<y>`), then the shape, which may be a `<circle>`, `<rectangle>`, `<text>`, `<image>` or `<bitmap>`.

```xml
<sprite>
  <grid>
    <columns>11</columns>
    <rows>5</rows>
    <padding><x>15</x><y>15</y></padding>
    <rectangle>
      <width>width</width>
      <height>height</height>
    </rectangle>
  </grid>
</sprite>
```

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

That picture is 45 pixels wide and 20 tall. The rows are measured from the top left, must all be the same length and may hold nothing but `.` and `*`: a space or any other character, an empty row, or rows of different lengths stop the load with a message that names the row and the character (`row 3 of a bitmap has 'o' as character 2`). A bitmap is drawn once, when the game loads, into the same kind of bitmap a sprite of [lines](#lines) is, so everything said there holds for it: its size is the object's size (`name.width` and `name.height` work with no window), every window backend shows it as a picture, and a collision of [type pixel](#collisions) tests exactly the solid pixels. Unlike a sprite of lines a `<bitmap>` can be repeated by a `<grid>`, and all the cells share the one picture. It has one color. On an object with a `<heading>` the picture is drawn once like this and then that finished picture is turned to the heading the object faces, to the nearest whole degree (360 headings, 0 and 360 being the same): each pixel of the turned picture takes the one pixel of the original that lies under it, so chunky pixels stay chunky and nothing is blended. The object keeps the original and the one picture it shows, and draws a new one only when its heading moves to another whole degree. Every heading comes out as the same square, big enough for the picture at any angle (for a bitmap 11 by 8 characters at `<scale>` 5 that is a square of 68), and that square is the object's size, as with [lines](#lines).

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

The picture keeps the drawing's own colors, antialiased, with soft edges coming out as partly transparent pixels, so there is no `<color>`. Everything after the drawing is the same as for a `<bitmap>`: the picture is made once, its size is the object's size (`name.width` and `name.height` work with no window), every window backend shows it as a picture (the engine draws it itself, with a library used only by `svg.cpp`, so no backend loads an SVG: SFML and raylib could not, SDL2 and OpenGL could), a `<grid>` repeats it and its cells share the one picture, it can be a frame of an [animation](#animation), and a collision of [type pixel](#collisions) tests its pixels, a pixel counting as solid if anything at all was drawn on it, so a faint edge counts. A picture cut close round what is drawn also has a close-fitting box for the collisions that are not `pixel`. A missing or unreadable file, a part with no size or entirely outside the drawing, and a scale of 0 or less stop the load with a message that names the object and the file.

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

The frames must all be pictures (a `<bitmap>`, an `<svg>` or `<line>`s, not a circle, rectangle, text or image) of the same size, and where one is repeated by a `<grid>` all of them are, the same way. Every sprite the object has must be shown by its animation, and several sprites without an animation are an error, since nothing would say when each is shown. An object with a `<heading>` can be animated too: whichever frame is showing is drawn at the heading the object faces, and the frames must come out as the same square when turned (equal sized bitmaps always do).

Only an object that is shown by the current state and in play (not dead, not hidden) moves on, so a pause or a menu holds the picture where it was, and a `<reset />` puts it back on the first picture, from the start of its time. Every cell of a `<grid>` has a count of its own, and they all start together, so a block of aliens changes picture as one. A collision of type pixel tests the picture that is showing. In a `<group>`, a member that gives sprites of its own has those instead of the group's, and one that gives an `<animation>` has that instead of the group's; the names in an animation are looked up among the sprites the member ends up with. Space Invaders is the example: its three kinds of alien are the members of one group, each a `<grid>` of two named bitmaps with an animation, so the block marches, bounces and steps down together while each kind flaps on its own sprites.

### Commands

Commands are tags, and where they are meaningful is what the table says. Any list of them (inside a `<collision>`, an `<action>`, an `<input>` or a `<condition>`) is run in the order written, for example `<inc variable="paddle2.score" />` then `<reset />`.

| Command | Where it is meaningful | Effect |
|---|---|---|
| `<bounce />` | collision | Reverses velocity away from the touched edge |
| `<stick />` | collision | Clamps the object inside the screen edge it touched and stops only the velocity heading into that edge; the other axis keeps going, so an object pressed against the bottom wall still slides left or right. Re-applied after the frame's move, so a stuck object never ends a frame outside the screen |
| `<wrap />` | collision (screen edge) | Once the object has gone completely off the screen through that edge and is still heading that way, puts it back in from the opposite edge, one window width (or height) plus its own size along, so anything spaced along a lane keeps its spacing. While any of it is still in view nothing happens |
| `<carry />` | collision (another object) | Lends the object the touched object's velocity for that frame, on top of its own: a frog on a log rides along with it. Worked out again every frame from whatever is still being touched, so an object that steps off is at rest |
| `<die />` | collision | Disables the object's collisions and hides it; it stops moving and being drawn until something brings it back (`<fire>` re-launching a bullet, `<reset />`) |
| `<reset />` | collision (screen edge or another object) | Puts the object back at its starting position |
| `<reset />` | state input or condition | Full game reset: every object's position, velocity, variables, visibility and collisions go back to how they started (a bullet in flight is put away, a dead alien is back), and the state stack collapses to the first state |
| `<reset object="name" />` | state input or condition | Resets that one object (or every cell of a grid, for its grid name) the same way |
| `<inc variable="owner.variable" />` or `<inc variable="owner.variable">amount</inc>` | collision (screen edge or another object) | Adds 1, or the amount (a value, so an expression works), to that variable and refreshes any text bound to it |
| `<dec variable="owner.variable" />` or `<dec variable="owner.variable">amount</dec>` | collision (screen edge or another object) | Takes 1, or the amount, off that variable and refreshes any text bound to it. The variable may go below zero; a condition with `<atmost>` is what notices it has run out |
| `<move direction="up">step</move>` (also `down`, `left`, `right`) | collision, or an object `<action>` | In a collision: shifts the object (or everything in lockstep with it) once. In an object action: sets a held-key velocity (see [Input](#input)). The content is a value |
| `<hop direction="up">distance</hop>` (also `down`, `left`, `right`) | object `<action>` | A one-shot jump of `distance` pixels for each press of the key (see [Input](#input)). The content is a value |
| `<accelerate direction="up" burn="fuel">amount</accelerate>` (also `down`, `left`, `right`) | object `<action>` | A thruster: while the key is held, the object's velocity changes by `amount` (a value) every frame in that direction, where `<move>` would set the velocity itself. The optional `burn` names one of the object's own `<variable>`s; 1 is taken off it every frame the thrust is on (and a text bound to it follows), and while it is 0 or below the thrust does nothing. See [Input](#input) |
| `<stop />` | collision (screen edge or another object) | The object comes to rest where it is and stays there: its velocity goes to 0, and it is no longer pulled by its `<acceleration>` or pushed by a held `<accelerate>` or `<move>`, until a reset gives the acceleration back. A landing |
| `<turn direction="left">degrees</turn>` (or `right`) | object `<action>` | While the key is held, the object's heading changes by that many degrees every frame (left is counterclockwise). Needs a `<heading>` on the object |
| `<thrust burn="fuel">amount</thrust>` | object `<action>` | Like `<accelerate>`, but along the way the object faces instead of along an axis; `burn` works the same. Needs a `<heading>` on the object |
| `<release object="name">count</release>` | collision (screen edge or another object) | Puts the first `count` (default 1) out-of-play objects of that name or group back in play, centered on the object running the rule, at their own starting velocity. Fewer left in the pool gives what is there. This is how a rock breaks into smaller ones |
| `<push state="name" />` / `<pop />` | state input or condition | Push a state / pop back. The state named must be one of the game's. `<pop />` with only the first state left does nothing |
| `<trigger object="name" action="up" />` | state input | Runs one of that object's named `<action>`s. The object and the action must exist |
| `<play sound="name" />` | anywhere: collision (screen edge or another object), object `<action>`, state input or condition | Plays one of the game's [sounds](#sounds). In an action or an input it plays on the press, not on the release. The game only asks for the sound; the engine plays everything asked for once the frame's keys, collisions and conditions have run |
| `<fire object="projectile" />` | object action | Launches the named projectile object from the shooter's top-center (the middle of the projectile over the middle of the shooter's top edge), moving with the projectile's own `<velocity>`. A projectile is not drawn, moved or collided with until it is fired, and is put away again by `<die />` (hitting a target, or `edge="all"`). The name may be a `<group>`: the first member that is out of play is the one launched, so a group of four is four shots in flight. A shooter with a `<heading>` fires from its nose, along the heading, at the speed of the projectile's `<velocity>`; with none, one projectile name can only be in flight once |

A command the engine does not know, or one missing an attribute it needs, stops the game loading with a message that says where (`object 'ball' > <collisions> > <collision>: unknown command <explode>`). So does a command that names something the game does not have: a state (`<push state="pasued" />`), an object or one of its actions (`<trigger>`), a projectile (`<fire>`), an object to reset (`<reset object="...">`) or a sound (`<play>`). These are checked once every object and state has been built, so a name used before the thing it names appears in the file is fine.

**Colors:** `color.black`, `color.white`, `color.red`, `color.green`, `color.blue`, `color.yellow`, `color.magenta`, `color.cyan`, and the muted `color.grey`, `color.darkgrey`, `color.lightgrey`, `color.brown`, `color.orange`, `color.purple`, `color.darkblue`, `color.darkgreen`, `color.forestgreen`. Any other name is fully transparent.

### `<state>`

A state is one screen: a menu, the playfield, a pause screen, a game-over screen. Attribute: `name`. Children, in this order:

| Element | Required | What it does |
|---|---|---|
| `<shows>` | yes | A list of `<show object="name" />`. Only these objects are drawn and updated while the state is current |
| `<inputs>` | yes | A list of `<input button="key">`, each holding the commands that key runs, e.g. `<push state="playing" />` or `<trigger object="paddle1" action="up" />` |
| `<conditions>` | no | A list of `<condition>`, checked every frame. See [Conditions](#conditions) |

The first state in the file is the starting state. States form a **stack**: `<push state="name" />` pushes a state, and `<pop />` pops back to the previous one. The starting state is never popped: a `<pop />` with only it left does nothing.

## Input

An `<input button="w"><trigger object="paddle1" action="up" /></input>` names a key and the commands it runs. Objects never mention keys, and states never mention what an action does, so remapping a key means editing one attribute in one state.

The shipped games share one convention: Space starts, pauses and unpauses, and plays again after a game over (in games where Space fires, such as Space Invaders, Astrosmash and Depth Charge, pausing is P or Escape, and Space still unpauses); player one plays on W, A, S and D (the arrow keys are a second way in the one-player games, and player two's keys in the two-player ones). The games written after Pong and Breakout also pause on P, and most of them on Escape; in Pong and Breakout Escape leaves the settings screen (S on the main menu).

Key names are lowercase: `a`-`z`, `num0`-`num9`, `numpad0`-`numpad9`, `f1`-`f15`, `space`, `enter`, `escape`, `backspace`, `tab`, `left`, `right`, `up`, `down`, `home`, `end`, `pageup`, `pagedown`, `insert`, `delete`, `pause`, `lshift`, `rshift`, `lcontrol`, `rcontrol`, `lalt`, `ralt`, and a few more listed in `lib/source/keycode.cpp`.

While a key bound to a `<move>` in an object action is held, that direction's step is recorded. Velocity is recomputed from all four directions on every key change, so holding Down and tapping Up cancels out and releasing Up resumes Down. Left/right and up/down are independent axes, so two keys can make a diagonal.

The current state decides what a held key means. When the state changes, keys that are still down stop driving whatever the old state bound them to, and start driving what the new state binds them to. In Breakout, holding Left in `playing` and pressing Space stops the paddle, because `paused` binds only Space; unpausing while Left is still down moves it again with no re-press, and a Left first pressed during the pause starts moving it on unpause. Letting go of a key always stops what it was driving. Only continuous bindings (an action's `<move>`) resume this way; state changes and one-shot commands such as `<fire>` run only when the key is actually pressed, so holding Space through the main menu does not pause the game. Alternatives that were considered are in [designs/10](designs/10-input-and-actions.md#held-keys-across-state-changes).

**Accelerate.** `<accelerate direction="up">amount</accelerate>` in an object `<action>` is held like a `<move>`, and is resumed the same way after a state change, but it adds to the velocity instead of setting it: each frame the key is down, every direction held contributes its `amount` (so opposite thrusters cancel, and two directions make a diagonal push), on top of the object's own `<acceleration>`, before the frame's move. What was gained stays: let go of the key and the object drifts on at the speed it reached. `<thrust>` is the same along the object's `<heading>`, and `<turn>` changes the heading by a fixed amount a frame while held; an object's `<drag>` then takes a fraction of the speed away each frame, so thrust has a top speed. With `burn="variable"` each thruster that is on takes 1 off that variable of the object's every frame, and does nothing once it is gone.

**Hop.** `<hop direction="up">distance</hop>` (and `down`, `left`, `right`) in an object `<action>` is a one-shot jump, not a held move. Pressing the key queues a hop of `distance` pixels; the next frame's move makes it before collisions are worked out, so the object is judged where it lands, and it is refused (the object stays where it is) if it would leave the window. Holding the key does nothing more and releasing it does nothing, and a state change never repeats it: only continuous bindings are resumed, so a key held through a pause has to be pressed again to hop. Two hops asked for in one frame keep the later one, so a hop is always one step in one direction.

## Collisions

Each object has `<collision>` rules. A rule's content is a list of command tags, run in order. There are two kinds:

- **Screen edge:** `<collision edge="left">`, `"right"`, `"top"`, `"bottom"`, plus the groupings `"vertical"` (top and bottom), `"horizontal"` (left and right) and `"all"`. Several rules that touch the same edge all run.
- **Object against object:** name what the rule applies to with `class="..."` (a kind of other object, matched against that object's `class` attribute) and/or `object="..."` (one named object), for example `<collision class="bricks"><die /></collision>`. A `<collision>` with no selector at all applies to anything. A `<collision>` with `edge` is always a screen-edge rule.

Detection (pure geometry, `CollisionDetector`) is kept apart from response (`CommandExecutor`). Object-against-object collisions are **swept**: each object moves along its own path for the frame, and the detector finds the moment two of them first touch (rectangles as boxes, a circle against the other object's box with rounded corners), so a small or fast object cannot jump over a thin one between frames. The earliest touch in the whole frame is handled first: everything moves up to that moment, the pair's rules run, and the rest of the frame is played with whatever velocities they left, so a bounce spends the remaining part of the frame heading away. Each pair reacts at most once per frame. The detector reports which edge of the other object was touched, and each side of the pair then sees the edge from its own point of view. Screen edges are still checked by position, before the move.

**Type.** `<type>` in `<collisions>` says how the object's shape is tested: `box` (the default: a rectangle as a rectangle, a circle as a circle, everything else as its bounding box) or `pixel`. A pair in which either object is `pixel` is first swept as boxes, exactly as above, and then looked at more closely: from the moment the boxes touch, the pair is walked along its path half a pixel at a time until some pixel drawn by one lies on a pixel drawn by the other, and that moment (found to a fraction of a pixel) is the hit. If the pixels never meet, there is no hit, however long the boxes overlapped. Pixels are asked about at their centres, at the positions the objects really are at. A `pixel` object is its sprite's bitmap (a sprite of [lines](#lines)); an object of any other type in the pair counts as solid all over its shape. A circle or a rectangle can be `pixel` too, which is solid as itself; text and images cannot (a load error), since what they look like is only known to a window backend. Each object says its own type, so a game with a pixel lander and pixel terrain gives both `<type>pixel</type>`. The same test decides `unless=` (is it touching something of that class right now).

The edge reported for a pixel hit is the one the motion came in through (the side of the other object that the two were moving toward each other across), or the boxes' edge when they were already touching; a pixel touch knows where pixels met, not which way a surface faces, so `<bounce />` on a pixel hit reflects along the axis of the relative motion.

Rules that apply:

- Only objects that are **shown** in the current state and have `enabled="true"` take part.
- **What a rule can do.** Against a screen edge: `<bounce />`, `<stick />`, `<reset />`, `<die />`, `<stop />`, `<move>`, `<inc>`, `<dec>` and `<wrap />`. Against another object: `<bounce />`, `<die />`, `<stop />`, `<reset />`, `<move>`, `<inc>`, `<dec>` and `<carry />`. `<stick />` and `<wrap />` are about a screen edge and do nothing in a rule about another object, and `<carry />` is about another object.
- **`<slower>` and `<faster>`.** A rule about another object may begin with `<slower>N</slower>` and/or `<faster>N</faster>` (values), before its commands: it only runs while the object's own speed (the length of its velocity, with what it is carried at) is **under** N (`slower`) or **N or more** (`faster`) at the moment of the touch. The speed is taken once, before any of the object's rules for this touch run, so a rule that `<stop />`s it does not change which rules after it are run. Two rules about the pad with the same number, one `slower` and one `faster`, are a landing and a crash: exactly N counts as fast. A screen-edge rule has no speed filter (a load error).
- **`unless`.** An object-against-object rule can carry `unless="class"`: it is passed over while the object is, at that same moment, touching something in play of that class. Frogger's river is `<collision class="water" unless="logs"><dec variable="frog.lives" /><reset /></collision>`: water costs the frog a life, unless it is also on a log. It looks at where things are right now, so it does not matter which of the two touches was handled first.
- A pair is skipped unless at least one of the two is moving, and unless one of them has a rule that answers to the other (its `class`/`object`, or a rule with no selector). An object counts as moving if its velocity is not zero, if it is being carried, or if it has just hopped, so an object that lands somewhere by hopping is judged there even though nothing else in the pair moves.
- `<lockstep>true</lockstep>` puts all cells of a `<grid>` object, or all members of a `<group>`, in lockstep: they share a lockstep number. Cells in lockstep never collide with each other, `<move>` in a collision moves all of them, and one hitting the left or right screen edge with `<bounce />` moves the whole block (this is how the invaders march). Being in lockstep changes none of the geometry: every cell is swept on its own, so a bullet only ever meets the cells that are still alive, and there is no bounding box around the block.
- Every cell of a `<grid>` is its own object, named after the grid with its column and row counted from 1: a grid called `aliens` has `aliens.1.1`, `aliens.2.1`, ... `aliens.11.5`. Names of the whole grid still work where the whole set is meant: `<show object="aliens"/>`, a rule or condition `object="aliens"`, `<reset object="aliens" />`; a cell can be named on its own (`object="aliens.3.2"`). A `<group>`'s members are named the same way (`logrow3.2`) and the group's name means all of them; see [`<group>`](#group).

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

`<remaining>` counts objects instead of reading a variable. It fires when no more than that many of the matching objects are still in play, that is still visible (`<die />` hides an object). `0` means they are all gone, which is how Space Invaders is won: `class="aliens"` covers every cell of the grid, and `object="aliens.3.2"` would watch a single one. `<atmost>` reads a variable from above: `<condition object="frog" variable="lives"><atmost>0</atmost>...</condition>` fires when the variable has fallen to that value or below, which is how a lives counter that goes down with `<dec>` ends the game (Frogger). A condition uses exactly one of `<atleast>`, `<atmost>` or `<remaining>` (the schema enforces this). If the filter matches no object at all, the game warns when it loads, since the condition would fire at once. Ending the game with a `gameover` state and starting over with a bare `<reset />` on that screen works as in Pong; `<reset />` also brings the dead aliens back.

## What happens when a game runs

1. **Parse and validate.** The XML backend loads the file. If the file names a schema, it is validated: Xerces does full XSD validation ("strong"); the other three backends use a small built-in validator for the subset of XSD this project uses ("weak", `xsd_lite`). `printGame()` reports which one ran.
2. **Evaluate.** exprtk evaluates every value once (an expression text, or a value tag such as `<random>`, which is drawn here). Objects, their variables, `<grid>` cells, states and sounds are built. Nothing here needs a window or a sound device.
3. **Open the window and the sound.** `Engine` creates the window backend and the audio backend (if the sound library will not start, it prints a warning and the game plays silently), makes every sound into samples and hands them to the audio backend, measures each object's real size for drawing and collisions, finishes the position of any text or image that uses `objectName.width` or `objectName.height`, then pushes the first state. The program prints the game twice, once before this step (sizes and size-dependent positions shown as unknown) and once after.
4. **Loop.** Each frame: read key changes and run the current state's bindings for them; count the frame towards the next picture of every shown object that has an [animation](#animation); change every shown object's velocity by its acceleration and held thrust; run the screen-edge rules; make any queued hops; move every shown object by its velocity, and by what it is being carried at, running the object-against-object rules at each touch on the way (per frame, not scaled by time); check conditions; play the sounds the frame asked for; clear; draw shown objects; present.

## Backends

| Job | Interface | Implementations |
|---|---|---|
| Read XML | `XmlDocument` / `XmlNode` (`xml_document.h`) | Xerces (default), TinyXML2, PugiXML, RapidXML |
| Window, drawing, keyboard | `Window` (`window.h`) | SFML3 (default), Raylib, SDL2, OpenGL (GLFW) |
| Sound | `Audio` (`audio.h`) | SFML3 (default), Raylib, SDL2, None (silent) |

One more library is used by the engine itself and is not a backend: lunasvg draws the SVG files an `<svg>` sprite names, into the same kind of bitmap a `<bitmap>` makes, so no window backend knows about SVG ([design 43](designs/43-svg-sprites.md)). Only `svg.cpp` includes it.

`Game` and `Engine` only ever see the interfaces. Each interface has a factory that is the single place that knows every implementation. Build-time dependency selection is in `scripts/cmake/` (see the `FORCE_LOCAL_*` options in `options.cmake`).


`Game` and `Engine` only ever see the interfaces. Each interface has a factory that is the single place that knows every implementation (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`).

The sound library is chosen apart from the window library, and any goes with any (OpenGL, which has no sound of its own, included). `Game` never makes a noise: `<play>` only asks for a sound (`Game::requestSound`), and `Engine` hands what a frame asked for to its `Audio`, so a game runs and is tested with no sound device. The engine makes every sound's samples itself (`synthesize()`, `sound.h`), so a sound is the same whichever library plays it; a backend only hands the samples to its library: an `sf::SoundBuffer` and `sf::Sound` (SFML 3), a `Sound` made with `LoadSoundFromWave` (raylib, which opens its sound device apart from its window), or a small mixer of its own in an SDL audio callback (SDL2, which has no mixer without SDL_mixer). The SDL2 window and the SDL2 sound each start and stop only their own part of SDL, so either can go first. `Engine::replaceAudio()` swaps the sound library of a running game, which is how the Options dialog changes it, and `Engine::silence()` stops what is playing, which `XGEGUI` does when the game is paused. `NullAudio` plays nothing: the tests use it, and so does `-a none`. Build-time dependency selection is in `scripts/cmake/` (see the `FORCE_LOCAL_*` options in `options.cmake`).

Every window backend opens a window of its own, which is what `XGECLI` uses. `XGEGUI` shows the game in one of two layouts ([design 40](designs/40-split-windows-in-xgegui.md)): in one window, drawn by a renderer of its own that draws with Qt (`QtWindow`, the default), or in two, where the main window holds only the controls and the tree and the game is in a window of its own, opened by the chosen library (SFML3, SDL2, raylib or OpenGL) or by the Qt renderer. `Engine::replaceWindow()` swaps the window of a running game for another without touching the game, which is how the Options dialog changes the video library. A front end that has paused the game but keeps its window calls `Engine::pump()` so the window can still be moved and closed, and `Engine::isWindowOpen()` tells it when the user closed it.

Nothing in `XGEGUI` uses OpenGL through Qt, so a library's OpenGL context is the only one on the thread. (The game view was a `QOpenGLWidget` once, and the two kinds of context, which Qt tracks by its own record, drew into each other; see [design 40](designs/40-split-windows-in-xgegui.md).)

## Building: library and programs

The engine (everything in `lib/source/` and `lib/include/`) is a library, the `XGELIB` CMake target. Two programs use it: `XGECLI` (`cli/`) and `XGEGUI` (`gui/`, the Qt application, built only when Qt 6 is found), and so do the tests (`XGETEST`, in `tests/`). Every project has the same layout, a folder with `source/` and `include/` in it; `XGEDATA` is the target that copies `games/` and `assets/` next to the programs. Programs, games and assets go in `output/<config>` at the top of the repository (`output/Debug`, `output/Release`), and the DLLs (`.so` files on Linux) and Qt's plug-ins in `output/<config>/libraries`, set up in `scripts/cmake/output.cmake` (on Windows the programs find them through a manifest, see [design 18](designs/18-build-system.md)); the build directory keeps only the compiler's and linker's own files. `output/` is ignored by git. The library knows nothing about the command line, so a different front end only needs its own `main()`.

The library is static by default: each program has the engine's code copied into it, so `XGECLI` is one self-contained file. `-DXGE_BUILD_SHARED=ON` builds it as a shared library instead (`libXGELIB.so`, or `XGELIB.dll` on Windows), which each program loads when it starts; a shared build needs the library file to be found next to the program or on the system's library path. On Windows the DLL exports every class in the headers (`WINDOWS_EXPORT_ALL_SYMBOLS`) rather than each being marked by hand. The third-party libraries are `PUBLIC` dependencies of the library, because the engine's own headers include theirs. See [design 35](designs/35-library-and-front-ends.md).

## Source map

| File | Responsibility |
|---|---|
| `cli/source/main.cpp`, `cli/source/cli.cpp` | XGECLI, the command line program: read the options, find the game file, build `Game` and `Engine` with the chosen backends. The only code outside the engine library |
| `gui/source/*.cpp` | XGEGUI, the Qt application ([design 38](designs/38-qt-front-end.md), [39](designs/39-opengl-backend-and-options.md), [40](designs/40-split-windows-in-xgegui.md)): `main_window` (the window, the File and View menus, the question about two windows), `game_session` (a loaded game and its engine, run from a timer; play, pause, step, reset, and changing the libraries), `game_stage` (where the Qt renderer's picture is: the left pane, or a window of its own), `game_view` (the widget a picture is shown in), `key_queue` (the keyboard, read by Qt), `qt_window` (the `Window` that draws with QPainter), `options_dialog` and `session_options` (the video library, XML parser and sound library choice), `app_settings` (`xgegui.ini`, next to the program), `inspector` (the controls and the tree of game data) |
| `game_xml.cpp` | Walk the parsed XML tags into raw window/variable/object/state data (`RawValue`, `RawCommand`, `RawSprite`); a `<group>` is read here as one raw object per member |
| `game_expr.cpp` | exprtk symbol table and evaluation of raw values into `Object`s and `State`s |
| `command.cpp` | Turn raw command tags into typed `Command`s |
| `game.cpp` | Objects, state stack, per-frame update, collision pairs, conditions, resets |
| `collision_detector.cpp` | Geometry only: box, circle and swept tests, and the pixel pass for type `pixel` |
| `builtin_font.cpp` | The 8x8 font stored in the program (`rasterizeText`), which draws text into a bitmap when a backend cannot load its font file |
| `bitmap.cpp` | Draws a sprite's `<line>`s (`rasterizeLines`) or `<bitmap>` rows (`rasterizeRows`) into an RGBA bitmap: the pixels both the window backends and pixel collisions use |
| `svg.cpp` | Draws a part of an SVG file (`rasterizeSvg`) into an RGBA bitmap, with lunasvg; the only file that includes it |
| `command_executor.cpp` | What each command does |
| `sound.cpp` | Pitch names, wave names, and `synthesize()`: a sound's notes made into 16-bit samples, the same for every audio backend |
| `audio.cpp`, `audio_*.cpp` | `AudioFactory` and the sound backends (SFML 3, raylib, SDL2; `NullAudio` is in `audio.h`) |
| `engine.cpp` | Frame loop (`loop()`, or `step()` and `render()` for a front end that owns the event loop), key handling, and playing the sounds each frame asks for |
| `object.h`, `states.h`, `color.cpp`, `keycode.cpp` | Data model, named colors, key names |
| `window_*.cpp`, `xml_*.cpp`, `xsd_lite.cpp` | Backends and the weak validator |
| `tests/` | Catch2 tests (opt-in with `BUILD_TESTING`): collision geometry and swept collision, command parsing, conditions, input resolution, `stick()`, collision rules, lockstep bounce, size expressions, engine key handling, object variables, the new verbs (`dec`, `hop`, `wrap`, `carry`, `unless`, `atmost`, colors), the tag format, its rejections and the names commands use (`test_xml_format`), groups (`test_group`: expansion, overrides, names, lockstep, errors, both schema checkers), lines, pixel collisions, acceleration, thrust and the speed filters (`test_lines_and_pixels`), bitmaps, animations and Space Invaders as written with them, with both schema checkers (`test_bitmap_sprites`; `invaders_fixture.h` keeps a copy of the first, plain Space Invaders for the tests that are about grids and not about that game), the built-in font, the command line and the data folder search, sound (`test_sound`: pitch names, the synthesizer, `<sounds>` and its rejections, Pong asking for its sounds, `Engine` and a recording `Audio`), `Engine::pump()` and `isWindowOpen()` (`test_engine_input`), and Frogger, Space Race, Kaboom, Freeway, Depth Charge, Astrosmash, Lunar Lander and Asteroids (`test_asteroids`: headings, turning, thrust and drag, the pool of shots, `release`, wrapping, losing ships, winning) played frame by frame , and Berserk, Demon Attack and Frostbite (`test_ai_games`: each played from the title to the end screen) |

## Known limitations

- Object-object collision knows only the four edges of the other object (and, for a pixel hit, the side the motion came in through); there are no verbs beyond the table above (no arcing jump (`hop` is a single step), no shooting patterns, AI).
- Sound is short effects and little tunes, not music: one wave at a time per sound (no chords inside one sound; two sounds at once do play together), no looping background tune, no stopping a sound from the game file, no volume envelope beyond the click-free fade (a note does not decay like a plucked string), and no stereo. A sound is made when the window opens, so its notes cannot change with a variable while the game runs. It starts on the frame it is asked for, so it is as late as the sound library's buffer (a few hundredths of a second).
- An object that is not visible is not moved and does not collide, whatever its velocity; that is how a bullet waits, unseen and still, to be fired. There are no hidden objects that still move and collide (an invisible trigger zone, an off-screen enemy on its way in); a game that wants one will need visibility and "in play" to be separate things.
- An object's velocity and collisions belong to the object, not to a state: any state that shows it lets it move. There is no way to show the Space Invaders aliens standing still behind the menu and have them march only in `playing`; they start marching as soon as they are shown. See [designs/09](designs/09-states-and-screens.md).
- An object's own `<variable>` named `width` or `height` shadows its measured size (`objectName.width` then reads the variable). A non-colliding spelling is under consideration; see design note 07.
- A text or image placed with `objectName.width` / `objectName.height` is re-placed only when the size of an object it names changes, so it is not re-centered after it has moved on its own, and other expressions (velocity, variables) see those sizes as 0 for unmeasured text and images.
- Text is drawn with `assets/tuffy.ttf`, found relative to the directory the program is run from. If that file cannot be found, each backend prints `error: failed to load font: assets/tuffy.ttf - drawing text with the built-in 8x8 font instead` once, and draws every text with the font stored in the program: 8 by 8 pixel glyphs for printable ASCII (anything else is a `?`), scaled to about `<size>` / 16 in blocks tall (so a letter is 8 times that tall) and half as wide, so it is chunky and monospaced and its width differs from the real font's ([design 36](designs/36-builtin-font.md)).
- Movement is in pixels per frame with no time step. Acceleration is constant per object (`<acceleration>`, `<accelerate>`); an object can face a heading and be pushed along it (`<heading>`, `<turn>`, `<thrust>`), and `<drag>` is the only thing that slows an object by itself apart from a rule or an opposing thrust.
- A `pixel` collision is only as exact as half a pixel of motion: a pair is looked at every half pixel along the longer way it moves in a frame, and two one-pixel lines can cross at a slant without sharing a pixel at all, so lines meant to be tested should be at least 2 pixels thick. It knows nothing about which way a surface faces, so there is no bounce off a slope. Text and images cannot be `pixel` yet (their pixels are only known to a window backend), so a fighting game with image sprites will need the backends to hand their pixels back. The bitmap of a sprite of lines is kept for every object that has one, four bytes a pixel, so one big drawing costs what its bounding box does (Lunar Lander's 800 by 182 moon is about 580 KB).
- A sprite of lines is measured from 0, 0 and fixed when the game loads: it cannot be moved, scaled or repeated by a `<grid>`, and it turns only on an object with a `<heading>`, in whole degrees (one picture at a time, always the same square size). Lines are straight, one color each, and have no fill.
- A `<bitmap>` is fixed when the game loads too, and has one color (an asterisk is that color, a period is clear), so a sprite in two colors is two objects on top of each other. A turning object keeps the original and the one picture it shows, so the memory it costs is about double, and a turn is done again only when the heading moves by a whole degree. The sprite pictures are separate bitmaps, so a game with many large animated sprites keeps them all in memory: four bytes a pixel for each frame, once per object (the cells of a grid share theirs).
- An `<svg>` is drawn once, when the game loads, at one scale: to show one at two sizes, make two sprites. A pixel collision counts any partly transparent edge pixel as solid, and the picture an animation or a `<grid>` shows cannot depend on a key, a hit or a death yet, so the banking, hit-flash, enemy-bolt, explosion and saucer pictures on the Space Invaders 2 sprite sheet are unused (see [design 43](designs/43-svg-sprites.md)).
- An animation has one interval for all its frames, and counts frames of the game, not time (see [Animation](#animation)). There is no way yet to show a picture that depends on something other than time (a variable, the direction of travel, a hit), to run an animation once and stop, or to start it from a collision.
- Each cell of a grid is its own object with its own count of frames. The cells stay in step because they start together (at load, and again at every reset) and only count while shown.
- A `<random>` is drawn once, when the game loads. A piece that falls, is caught and is reset falls again at the same speed from the same place, so Kaboom's and Astrosmash's rhythm is different on every launch but repeats within one. There is no way yet to draw again on `<reset />`, and nothing in the language creates a new object while the game runs, so things that fall are a fixed set that cycles.
- A hop is a jump, not a slide: the object is simply one step away, with no animation between and nothing touched on the way (a step is meant to be one lane). It is not the arcing jump planned in [designs/13](designs/13-verb-vocabulary.md).
- `carry()` lends velocity one frame at a time and only while touching. It is not attached: a carried object that meets a screen edge is handled by that edge's rule like any other (Frogger's frog loses a life), and nothing pushes it into a wall.
- A pair in which nothing moves is not looked at (see the collision rules above), so a rule against something that stands still runs only when the object moves, is carried, or hops.
- `unless` names a class, not a single object, and only exists on object-against-object rules.
- `wrap()` assumes what it wraps is spaced with its own size in mind (a lane of things that are `size` wide repeats every window width plus `size`), and the members of a lane (a `<group>`) are placed by hand, one `<x>` each: there is no way yet to say "this many, evenly spaced". A `<grid>` gives one velocity and one spacing to all its cells.
- Only the object-against-object test is swept. A screen edge is still checked by position before the move, so an object that moves more than a whole window's width in one frame is not caught by it; rotation is not modeled. A shape that is not a rectangle or circle (text, image) is treated as its bounding box, unless it is a sprite of lines tested by `pixel`.
- Expressions inside a value are not checked by the schema (they are text); a typo in one is found when the game loads (the load stops with a message naming where), not by validation. Structure, tag names, attributes and command verbs are checked by the schema; the names a command uses are checked by the engine when it loads.
- The weak validator (`xsd_lite`) covers only the XSD subset this project uses; Xerces is the full check.
- The window's `width`, `height` and `framerate` are plain numbers, not values, and are read once, before the window opens. The size cannot be changed while a game runs, in `XGEGUI` either.
